import subprocess
import sys
import tempfile
import unittest
from datetime import datetime, timezone
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import tt10_simulator as simulator


class SimulatorTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "sandbox"
        self.now = datetime(2026, 1, 1, 5, 0, 0, tzinfo=timezone.utc)

    def emit(self, scenario="normal", values=None):
        return simulator.emit_sample(self.root, scenario, values or simulator.defaults(), self.now)

    @staticmethod
    def lines(path):
        return [line.split("\t") for line in path.read_text(encoding="utf-8").splitlines()]

    def test_five_fields_timestamps_and_sources(self):
        gas, water = self.emit()
        self.assertEqual(gas.name, "SIM_TEST_KHITO1_20260101120000.txt")
        self.assertEqual(water.name, "SIM_TEST_NUONT1_20260101120000.txt")
        self.assertEqual(len(self.lines(gas)), 8)
        self.assertEqual(len(self.lines(water)), 4)
        for row in self.lines(gas) + self.lines(water):
            self.assertEqual(len(row), 5)
            self.assertEqual(row[3:], ["20260101120000", "00"])
        self.assertEqual((self.root / simulator.MARKER_NAME).read_text(), simulator.MARKER_TEXT)

    def test_calibration_and_fault_status(self):
        gas, water = self.emit("calibration")
        self.assertEqual(next(r for r in self.lines(gas) if r[0] == "SO2")[4], "01")
        self.assertEqual(next(r for r in self.lines(water) if r[0] == "pH")[4], "01")
        gas, water = self.emit("fault")
        self.assertEqual(next(r for r in self.lines(gas) if r[0] == "CO")[4], "02")
        self.assertEqual(next(r for r in self.lines(water) if r[0] == "COD")[4], "02")

    def test_wrong_unit_unknown_and_six_digit_flow(self):
        gas, _ = self.emit("wrong-unit")
        self.assertEqual(next(r for r in self.lines(gas) if r[0] == "PM")[2], "mg/m3")
        gas, _ = self.emit("unknown-status")
        self.assertEqual(next(r for r in self.lines(gas) if r[0] == "CO")[4], "03")
        gas, _ = self.emit("long-flow")
        self.assertEqual(next(r for r in self.lines(gas) if r[0] == "Flow")[1], "123456")

    def test_missing_and_stale_do_not_refresh_channels(self):
        gas, water = self.emit("stale")
        self.assertNotIn("O2", [r[0] for r in self.lines(gas)])
        self.assertNotIn("pH", [r[0] for r in self.lines(water)])
        gas, water = self.emit("missing-ph")
        self.assertIn("O2", [r[0] for r in self.lines(gas)])
        self.assertNotIn("pH", [r[0] for r in self.lines(water)])

    def test_conflict_same_timestamp_different_value(self):
        gas, _, conflict = self.emit("conflict")
        original = next(r for r in self.lines(gas) if r[0] == "SO2")
        changed = self.lines(conflict)[0]
        self.assertEqual(original[0], changed[0])
        self.assertEqual(original[2:], changed[2:])
        self.assertNotEqual(original[1], changed[1])
        self.assertTrue(conflict.name.endswith("_1.txt"))

    def test_never_overwrite_existing_files(self):
        gas, _ = self.emit()
        before = gas.read_bytes()
        values = simulator.defaults()
        values["co"] = "123"
        second, _ = self.emit(values=values)
        self.assertNotEqual(gas, second)
        self.assertEqual(gas.read_bytes(), before)
        self.assertTrue(second.name.endswith("_1.txt"))
        self.assertFalse(list(self.root.rglob("*.part")))

    def test_refuses_nonempty_unmarked_directory(self):
        self.root.mkdir()
        original = self.root / "real.txt"
        original.write_text("do not modify", encoding="utf-8")
        with self.assertRaises(ValueError):
            self.emit()
        self.assertEqual(original.read_text(encoding="utf-8"), "do not modify")
        self.assertEqual(list(self.root.iterdir()), [original])

    def test_refuses_bad_values_without_writing_samples(self):
        for bad in ("NaN", "1e5", "2,5", "3\t00", "9" * 31):
            values = simulator.defaults()
            values["co"] = bad
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.emit(values=values)
        self.assertFalse(list(self.root.rglob("*.txt")))

    def test_file_limit_does_not_delete(self):
        first = self.emit()
        saved = [path.read_bytes() for path in first]
        with mock.patch.object(simulator, "MAX_FILES_PER_SOURCE", 1):
            with self.assertRaises(RuntimeError):
                self.emit()
        self.assertEqual([path.read_bytes() for path in first], saved)

    def test_cli_once(self):
        result = subprocess.run(
            [sys.executable, str(Path(simulator.__file__)), "--root", str(self.root), "--once", "--scenario", "normal"],
            check=True, capture_output=True, text=True, encoding="utf-8",
        )
        self.assertEqual(len(result.stdout.splitlines()), 2)
        self.assertEqual(len(list(self.root.rglob("SIM_TEST_*.txt"))), 2)


if __name__ == "__main__":
    unittest.main()
