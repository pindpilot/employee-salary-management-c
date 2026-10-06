"""Integration tests: run the compiled program with scripted input."""
import os, subprocess, tempfile, unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "ems"


def run(inp, cwd):
    return subprocess.run([str(EXE)], input=inp, text=True, capture_output=True, cwd=cwd, timeout=10).stdout


ADD = "1\n{id}\n{name}\n3\n{date}\n{basic}\n"


class T(unittest.TestCase):
    def setUp(self):
        self.d = tempfile.TemporaryDirectory()
        self.cwd = self.d.name

    def tearDown(self):
        self.d.cleanup()

    def add(self, id=1, name="Amit", date="2024-02-29", basic="50000"):
        return ADD.format(id=id, name=name, date=date, basic=basic)

    def test_salary_values(self):
        out = run(self.add() + "0\n", self.cwd)
        for s in ("10000.00", "5000.00", "65000.00", "6000.00", "59000.00"):
            self.assertIn(s, out)

    def test_persistence(self):
        run(self.add() + "0\n", self.cwd)
        out = run("2\n0\n", self.cwd)
        self.assertIn("Loaded 1 employee", out)
        self.assertIn("Amit", out)
        self.assertEqual((Path(self.cwd) / "employees.txt").read_text(), "1|Amit|2|2024-02-29|50000.00\n")

    def test_duplicate_id(self):
        out = run(self.add() + self.add(name="Bob") + "0\n", self.cwd)
        self.assertIn("This ID already exists", out)
        self.assertEqual((Path(self.cwd) / "employees.txt").read_text().count("\n"), 1)

    def test_invalid_inputs(self):
        inp = "1\nabc\n0\n1\n7\n" + "x\n"  # bad id, then valid id 7
        inp += "Bob\n3\n2023-02-29\n2024-13-01\n2024-02-29\n-5\n0\nabc\n1e3\n0\n"
        out = run(inp, self.cwd)
        self.assertIn("Enter a whole number", out)
        self.assertIn("Invalid date", out)
        self.assertIn("more than zero", out)
        self.assertIn("Employee added", out)

    def test_pipe_in_name(self):
        out = run("1\n1\na|b\nAmit\n1\n2020-01-01\n100\n0\n", self.cwd)
        self.assertIn("'|' character is not allowed", out)

    def test_update_delete_search(self):
        run(self.add() + "0\n", self.cwd)
        out = run("4\n1\n3\n80000\n3\n1\n0\n", self.cwd)
        self.assertIn("Employee updated", out)
        self.assertIn("Net Pay    :     94400.00", out)
        out = run("5\n1\nn\n5\n1\ny\n3\n1\n0\n", self.cwd)
        self.assertIn("Cancelled", out)
        self.assertIn("Employee deleted", out)
        self.assertIn("Employee not found", out)
        self.assertEqual((Path(self.cwd) / "employees.txt").read_text(), "")

    def test_report(self):
        run(self.add() + self.add(id=2, name="Cat", basic="30000") + "0\n", self.cwd)
        out = run("6\n0\n", self.cwd)
        self.assertIn("Total net payout", out)
        self.assertIn("Highest basic      : Amit", out)

    def test_corrupt_line_skipped(self):
        (Path(self.cwd) / "employees.txt").write_text("1|Amit|2|2024-02-29|50000.00\nbad line\n1|Dup|1|2020-01-01|5\n")
        out = run("0\n", self.cwd)
        self.assertIn("Loaded 1 employee(s), skipped 2", out)

    def test_rollback_on_failed_save(self):
        # make employees.tmp a directory so saving cannot open it
        (Path(self.cwd) / "employees.tmp").mkdir()
        out = run(self.add() + "2\n0\n", self.cwd)
        self.assertIn("Employee not added", out)
        self.assertIn("No employees yet", out)

    def test_eof_exit(self):
        self.assertIn("Input closed", run("", self.cwd))


if __name__ == "__main__":
    unittest.main()
