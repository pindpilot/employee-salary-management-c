# Employee Salary Management System (C)

**Author: Gurleen Kaur**

A menu-driven console project in C. I made it to manage employee records and calculate salaries (HRA, DA, PF, gross and net pay). Data is saved in a text file so it is still there after the program is closed.

## Features
- Add, view, search, update and delete employees
- Payslip with Basic, HRA, DA, Gross, PF and Net pay
- Salary report: department-wise net pay, totals, average, highest basic
- 6 departments: HR, Finance, IT, Sales, Operations, Other
- Data saved in `employees.txt` and loaded again on start
- Input validation (dates with leap year, positive amounts, duplicate IDs)

## Salary rules
All values are calculated on the basic salary:

| Part | Rule |
|------|------|
| HRA | 20% of basic |
| DA | 10% of basic |
| PF | 12% of basic |
| Gross | Basic + HRA + DA |
| Net | Gross - PF |

Example: Basic 50000 -> HRA 10000, DA 5000, Gross 65000, PF 6000, Net 59000.

## How to compile and run
```
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -o ems main.c
./ems
```
On Windows (MinGW): `gcc -o ems.exe main.c` then `ems.exe`.

Tests (optional, needs Python 3): `python3 tests/test_system.py`

## Files
- `main.c` - whole program
- `employees.txt` - created automatically, one employee per line: `id|name|dept|joined|basic`
- `tests/test_system.py` - automated tests

## 5-minute demo guide
1. Run the program. Menu appears (1 min to explain the options).
2. Add 2 employees (e.g. ID 101 Amit, IT, 2024-02-29, 50000 and ID 102 Neha, HR, 2023-07-01, 35000). Show the payslip.
3. Try wrong input: ID 101 again (duplicate), date 2023-02-29, salary -5. Show the error messages.
4. View all (option 2), search ID 101 (option 3), update Amit's basic (option 4).
5. Salary report (option 6).
6. Exit with 0, run again and show the employees are loaded back. Open `employees.txt` to show the file format.
7. Delete one employee (option 5) and show the list.

## Viva revision

**Why a structure?** One employee has id, name, department, date and salary. A struct keeps them together as one record: `Employee employees[500]`.

**Why is department stored as a number?** `departments[6]` holds the names, the struct stores index 0-5. It saves space and comparing is easy.

**What does menu-driven mean?** The program shows a menu inside an infinite loop with a `switch`. User picks a number, that function runs, and 0 exits.

**How is the file saved?** After every add/update/delete, `saveEmployees()` writes each employee as `id|name|dept|joined|basic` with `fprintf`.

**How is the file loaded?** `loadEmployees()` runs at start. It reads line by line with `fgets`, `parseRecord()` splits on `|` and validates each field. Bad or duplicate lines are skipped with a warning.

**Why text file, not binary?** I can open it in Notepad to check and fix data, and it works on every computer. Binary is faster but not readable.

**Why is `|` not allowed in a name?** It is the separator, so a `|` in the name would break the fields while loading.

**What is the safe save method?** Data is written to `employees.tmp`, the old file becomes `employees.bak`, then the tmp file is renamed to `employees.txt` and the bak is deleted. If something fails the old file is restored.

**What is rollback?** The record is added to the array first, then saved. If saving fails the change is undone (`empCount--`, old record restored) so the screen and file match.

**How are duplicate IDs stopped?** `findEmployee(id)` does a linear search of the array. If it finds the ID, the add is refused. Same check on file load.

**How is the date validated?** `validDate()` checks length 10, `-` at positions 5 and 8, digits elsewhere, month 1-12 and day within the month. February has 29 days only in a leap year.

**Leap year rule?** `year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)`. 2000 and 2024 are leap, 1900 is not.

**Why `fgets` and not `scanf`?** `scanf` gets stuck on wrong input like letters. `fgets` reads the full line, then `strtol` / `strtod` convert it and I can check for errors.

**How is salary calculated?** `calcHra`, `calcDa`, `calcPf`, `calcGross`, `calcNet` functions. Rates are `#define` constants so they are easy to change.

**What does the salary report do?** Loops over all employees, adds net pay in a department-wise array, and prints totals, average and the highest basic.

**Why double for money?** Simple and enough for a student project. Real payroll software would use integers (paise) or a decimal type because double can have tiny rounding errors.

**Limits of the project?** Max 500 employees (fixed array), one user, no login, search by ID only, fixed salary percentages, no tax slabs.

**Possible improvements?** Search by name, sorting, income tax slabs, overtime and bonus, dynamic memory (`malloc`), a database, CSV export.
