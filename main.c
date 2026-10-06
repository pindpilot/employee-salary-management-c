/*
 * Employee Salary Management System
 * Author: Gurleen Kaur
 *
 * Menu-driven console project in C. Uses a structure, functions, arrays
 * and text file handling. Salary rules (all on basic pay):
 *   HRA = 20%, DA = 10%, PF = 12%
 *   Gross = Basic + HRA + DA
 *   Net   = Gross - PF
 */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EMP 500
#define NAME_LEN 50
#define LINE_LEN 256
#define MAX_BASIC 10000000.0
#define HRA_RATE 0.20
#define DA_RATE 0.10
#define PF_RATE 0.12

#define DATA_FILE "employees.txt"
#define TMP_FILE "employees.tmp"
#define BAK_FILE "employees.bak"

typedef struct {
    int id;
    char name[NAME_LEN];
    int dept;           /* index into departments[] */
    char joined[11];    /* YYYY-MM-DD */
    double basic;
} Employee;

static const char *departments[6] = {
    "HR", "Finance", "IT", "Sales", "Operations", "Other"
};

static Employee employees[MAX_EMP];
static int empCount = 0;

/* ---------- input helpers (fgets instead of scanf) ---------- */

static void readLine(const char *prompt, char *buf, size_t size)
{
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (fgets(buf, (int)size, stdin) == NULL) {
            puts("\nInput closed. Exiting.");
            exit(0);
        }
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            return;
        }
        /* line was too long: throw away the rest and ask again */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
        puts("Input too long, try again.");
    }
}

static int readInt(const char *prompt, int min, int max)
{
    char line[LINE_LEN];
    for (;;) {
        readLine(prompt, line, sizeof line);
        char *end;
        errno = 0;
        long v = strtol(line, &end, 10);
        if (end == line) { puts("Enter a whole number."); continue; }
        while (isspace((unsigned char)*end)) end++;
        if (*end != '\0' || errno == ERANGE) { puts("Enter a whole number."); continue; }
        if (v < min || v > max) { printf("Enter a value from %d to %d.\n", min, max); continue; }
        return (int)v;
    }
}

static double readAmount(const char *prompt)
{
    char line[LINE_LEN];
    for (;;) {
        readLine(prompt, line, sizeof line);
        char *end;
        errno = 0;
        double v = strtod(line, &end);
        if (end == line) { puts("Enter a number."); continue; }
        while (isspace((unsigned char)*end)) end++;
        if (*end != '\0' || errno == ERANGE) { puts("Enter a plain number (no commas or letters)."); continue; }
        if (v <= 0) { puts("Amount must be more than zero."); continue; }
        if (v > MAX_BASIC) { puts("Amount is too large (max 1,00,00,000)."); continue; }
        return v;
    }
}

static void readName(const char *prompt, char *out)
{
    char line[LINE_LEN];
    for (;;) {
        readLine(prompt, line, sizeof line);
        size_t len = strlen(line);
        if (len == 0) { puts("Name cannot be empty."); continue; }
        if (len >= NAME_LEN) { puts("Name is too long."); continue; }
        int ok = 1;
        for (size_t i = 0; i < len; i++) {
            if (line[i] == '|') { ok = 0; break; }   /* '|' is our file separator */
        }
        if (!ok) { puts("The '|' character is not allowed."); continue; }
        strcpy(out, line);
        return;
    }
}

/* ---------- validation ---------- */

static int isLeap(int y)
{
    return (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
}

static int validDate(const char *s)
{
    if (strlen(s) != 10 || s[4] != '-' || s[7] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    int y = atoi(s);
    int m = atoi(s + 5);
    int d = atoi(s + 8);
    int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (y < 1900 || y > 2100 || m < 1 || m > 12) return 0;
    if (m == 2 && isLeap(y)) days[1] = 29;
    return d >= 1 && d <= days[m - 1];
}

static void readDate(const char *prompt, char *out)
{
    char line[LINE_LEN];
    for (;;) {
        readLine(prompt, line, sizeof line);
        if (validDate(line)) { strcpy(out, line); return; }
        puts("Invalid date. Use YYYY-MM-DD, e.g. 2024-02-29.");
    }
}

/* ---------- salary calculation ---------- */

static double calcHra(double basic)   { return basic * HRA_RATE; }
static double calcDa(double basic)    { return basic * DA_RATE; }
static double calcPf(double basic)    { return basic * PF_RATE; }
static double calcGross(double basic) { return basic + calcHra(basic) + calcDa(basic); }
static double calcNet(double basic)   { return calcGross(basic) - calcPf(basic); }

/* ---------- lookups ---------- */

static int findEmployee(int id)
{
    for (int i = 0; i < empCount; i++)
        if (employees[i].id == id) return i;
    return -1;
}

/* ---------- file handling ---------- */

static int saveEmployees(void)
{
    FILE *fp = fopen(TMP_FILE, "w");
    if (fp == NULL) { puts("Error: cannot create temporary file."); return 0; }
    for (int i = 0; i < empCount; i++) {
        Employee *e = &employees[i];
        if (fprintf(fp, "%d|%s|%d|%s|%.2f\n", e->id, e->name, e->dept, e->joined, e->basic) < 0) {
            fclose(fp);
            remove(TMP_FILE);
            puts("Error: write failed.");
            return 0;
        }
    }
    if (fclose(fp) != 0) { remove(TMP_FILE); puts("Error: could not close file."); return 0; }

    /* safe replace: old file -> .bak, tmp -> real file, then drop .bak */
    remove(BAK_FILE);
    int hadOld = (rename(DATA_FILE, BAK_FILE) == 0);
    if (rename(TMP_FILE, DATA_FILE) != 0) {
        if (hadOld) rename(BAK_FILE, DATA_FILE);   /* restore old data */
        remove(TMP_FILE);
        puts("Error: could not replace data file.");
        return 0;
    }
    if (hadOld) remove(BAK_FILE);
    return 1;
}

static int parseRecord(char *line, Employee *e)
{
    char *parts[5];
    int n = 0;
    char *p = line;
    while (n < 5) {
        parts[n++] = p;
        char *bar = strchr(p, '|');
        if (bar == NULL) break;
        *bar = '\0';
        p = bar + 1;
    }
    if (n != 5) return 0;

    char *end;
    long id = strtol(parts[0], &end, 10);
    if (*parts[0] == '\0' || *end != '\0' || id < 1 || id > 999999) return 0;
    if (parts[1][0] == '\0' || strlen(parts[1]) >= NAME_LEN) return 0;
    long dept = strtol(parts[2], &end, 10);
    if (*parts[2] == '\0' || *end != '\0' || dept < 0 || dept > 5) return 0;
    if (!validDate(parts[3])) return 0;
    double basic = strtod(parts[4], &end);
    if (*parts[4] == '\0' || *end != '\0' || basic <= 0 || basic > MAX_BASIC) return 0;

    e->id = (int)id;
    strcpy(e->name, parts[1]);
    e->dept = (int)dept;
    strcpy(e->joined, parts[3]);
    e->basic = basic;
    return 1;
}

static void loadEmployees(void)
{
    FILE *fp = fopen(DATA_FILE, "r");
    char line[LINE_LEN];
    int lineNo = 0, skipped = 0;
    if (fp == NULL) return;   /* first run: no file yet */
    while (fgets(line, sizeof line, fp) != NULL) {
        Employee e;
        lineNo++;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;
        if (!parseRecord(line, &e) || findEmployee(e.id) != -1 || empCount >= MAX_EMP) {
            printf("Warning: skipped bad record on line %d.\n", lineNo);
            skipped++;
            continue;
        }
        employees[empCount++] = e;
    }
    fclose(fp);
    printf("Loaded %d employee(s)", empCount);
    if (skipped) printf(", skipped %d", skipped);
    puts(".");
}

/* ---------- display ---------- */

static void printHeader(void)
{
    printf("%-6s %-20s %-11s %-10s %12s %12s\n",
           "ID", "Name", "Dept", "Joined", "Basic", "Net Pay");
    puts("-----------------------------------------------------------------------------");
}

static void printRow(const Employee *e)
{
    printf("%-6d %-20.20s %-11s %-10s %12.2f %12.2f\n",
           e->id, e->name, departments[e->dept], e->joined, e->basic, calcNet(e->basic));
}

static void printPayslip(const Employee *e)
{
    puts("\n========== PAYSLIP ==========");
    printf("ID         : %d\n", e->id);
    printf("Name       : %s\n", e->name);
    printf("Department : %s\n", departments[e->dept]);
    printf("Joined     : %s\n", e->joined);
    puts("-----------------------------");
    printf("Basic      : %12.2f\n", e->basic);
    printf("HRA (20%%)  : %12.2f\n", calcHra(e->basic));
    printf("DA (10%%)   : %12.2f\n", calcDa(e->basic));
    printf("Gross      : %12.2f\n", calcGross(e->basic));
    printf("PF (12%%)   : %12.2f\n", calcPf(e->basic));
    puts("-----------------------------");
    printf("Net Pay    : %12.2f\n", calcNet(e->basic));
    puts("=============================");
}

static void chooseDept(int *dept)
{
    puts("Departments:");
    for (int i = 0; i < 6; i++) printf("  %d. %s\n", i + 1, departments[i]);
    *dept = readInt("Choose department (1-6): ", 1, 6) - 1;   /* store 0-5 */
}

/* ---------- menu actions ---------- */

static void addEmployee(void)
{
    if (empCount >= MAX_EMP) { puts("Employee list is full."); return; }
    Employee e;
    e.id = readInt("Employee ID (1-999999): ", 1, 999999);
    if (findEmployee(e.id) != -1) { puts("This ID already exists."); return; }
    readName("Name: ", e.name);
    chooseDept(&e.dept);
    readDate("Joining date (YYYY-MM-DD): ", e.joined);
    e.basic = readAmount("Basic salary: ");

    employees[empCount++] = e;
    if (!saveEmployees()) {
        empCount--;                      /* rollback */
        puts("Employee not added.");
        return;
    }
    puts("Employee added.");
    printPayslip(&e);
}

static void viewEmployees(void)
{
    if (empCount == 0) { puts("No employees yet."); return; }
    printHeader();
    for (int i = 0; i < empCount; i++) printRow(&employees[i]);
    printf("Total employees: %d\n", empCount);
}

static void searchEmployee(void)
{
    int id = readInt("Employee ID to search: ", 1, 999999);
    int idx = findEmployee(id);
    if (idx < 0) { puts("Employee not found."); return; }
    printPayslip(&employees[idx]);
}

static void updateEmployee(void)
{
    int id = readInt("Employee ID to update: ", 1, 999999);
    int idx = findEmployee(id);
    if (idx < 0) { puts("Employee not found."); return; }

    Employee previous = employees[idx];   /* kept for rollback */
    Employee e = previous;
    puts("1. Name\n2. Department\n3. Basic salary\n4. Joining date");
    int ch = readInt("What to update (1-4): ", 1, 4);
    if (ch == 1) readName("New name: ", e.name);
    else if (ch == 2) chooseDept(&e.dept);
    else if (ch == 3) e.basic = readAmount("New basic salary: ");
    else readDate("New joining date (YYYY-MM-DD): ", e.joined);

    employees[idx] = e;
    if (!saveEmployees()) {
        employees[idx] = previous;
        puts("Update failed, old record restored.");
        return;
    }
    puts("Employee updated.");
    printPayslip(&e);
}

static void deleteEmployee(void)
{
    int id = readInt("Employee ID to delete: ", 1, 999999);
    int idx = findEmployee(id);
    if (idx < 0) { puts("Employee not found."); return; }

    char ans[LINE_LEN];
    printf("Delete %s? ", employees[idx].name);
    readLine("(y/n): ", ans, sizeof ans);
    if (tolower((unsigned char)ans[0]) != 'y' || ans[1] != '\0') { puts("Cancelled."); return; }

    Employee removed = employees[idx];
    for (int i = idx; i < empCount - 1; i++) employees[i] = employees[i + 1];
    empCount--;
    if (!saveEmployees()) {
        for (int i = empCount; i > idx; i--) employees[i] = employees[i - 1];
        employees[idx] = removed;        /* rollback */
        empCount++;
        puts("Delete failed, record restored.");
        return;
    }
    puts("Employee deleted.");
}

static void salaryReport(void)
{
    if (empCount == 0) { puts("No employees yet."); return; }
    double total[6] = {0};
    int count[6] = {0};
    double grandNet = 0, grandGross = 0;
    int top = 0;

    for (int i = 0; i < empCount; i++) {
        Employee *e = &employees[i];
        total[e->dept] += calcNet(e->basic);
        count[e->dept]++;
        grandNet += calcNet(e->basic);
        grandGross += calcGross(e->basic);
        if (e->basic > employees[top].basic) top = i;
    }
    puts("\n--- Department-wise Net Pay ---");
    printf("%-12s %6s %14s\n", "Department", "Staff", "Net Total");
    for (int d = 0; d < 6; d++)
        if (count[d] > 0) printf("%-12s %6d %14.2f\n", departments[d], count[d], total[d]);
    puts("-------------------------------");
    printf("Total gross payout : %.2f\n", grandGross);
    printf("Total net payout   : %.2f\n", grandNet);
    printf("Average net pay    : %.2f\n", grandNet / empCount);
    printf("Highest basic      : %s (%.2f)\n", employees[top].name, employees[top].basic);
}

static void showMenu(void)
{
    puts("\n===== Employee Salary Management System =====");
    puts("1. Add employee");
    puts("2. View all employees");
    puts("3. Search employee (payslip)");
    puts("4. Update employee");
    puts("5. Delete employee");
    puts("6. Salary report");
    puts("0. Exit");
}

int main(void)
{
    loadEmployees();
    for (;;) {
        showMenu();
        int choice = readInt("Enter choice: ", 0, 6);
        switch (choice) {
            case 1: addEmployee(); break;
            case 2: viewEmployees(); break;
            case 3: searchEmployee(); break;
            case 4: updateEmployee(); break;
            case 5: deleteEmployee(); break;
            case 6: salaryReport(); break;
            case 0: puts("Goodbye!"); return 0;
        }
    }
}
