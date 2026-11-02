"""Kiểm tra simulator -> file -> app -> snapshot, với dữ liệu hoàn toàn tạm."""

import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import time
from datetime import datetime, timedelta, timezone

PROJECT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT / "tools"))
import tt10_simulator as simulator


def snapshots(path):
    if not path.is_file():
        return {}
    try:
        db = sqlite3.connect(str(path), timeout=0.2)
        try:
            rows = db.execute(
                "SELECT channel_id, raw_value, device_status, validity, conflict FROM latest_reading"
            ).fetchall()
        finally:
            db.close()
        return {row[0]: row[1:] for row in rows}
    except sqlite3.Error:
        return {}


def wait_for(process, db, predicate, label, seconds=20):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if process.poll() is not None:
            raise AssertionError(f"Ứng dụng thoát sớm trong bước {label}, mã {process.returncode}")
        rows = snapshots(db)
        if predicate(rows):
            print(f"OK: {label}", flush=True)
            return
        time.sleep(0.2)
    raise AssertionError(f"Timeout {label}; snapshot cuối: {snapshots(db)}")


def main():
    exe = Path(sys.argv[1]).resolve()
    if not exe.is_file():
        raise SystemExit(f"Thiếu executable: {exe}")
    with tempfile.TemporaryDirectory(prefix="caongan-sim-test-") as tmp:
        base = Path(tmp)
        sandbox = base / "sandbox"
        app_data = base / "app-data"
        simulator.prepare_sandbox(sandbox)
        measured = datetime.now(timezone.utc).replace(microsecond=0) - timedelta(seconds=5)
        simulator.emit_sample(sandbox, "normal", simulator.defaults(), measured)

        config = json.loads((PROJECT / "sample-data" / "caongan-led.example.json").read_text(encoding="utf-8"))
        for source in config["sources"]:
            source["folder"] = str(sandbox / "inputs" / source["id"])
        config_path = base / "test-config.json"
        config_path.write_text(json.dumps(config, ensure_ascii=False), encoding="utf-8")
        xdg = base / "xdg"
        xdg.mkdir(mode=0o700)
        env = os.environ.copy()
        env.update(QT_QPA_PLATFORM="offscreen", QT_FATAL_WARNINGS="1",
                   XDG_RUNTIME_DIR=str(xdg), CAONGAN_LED_BASE_DIR=str(app_data))
        if sys.platform == "win32":
            # Qt offscreen không dùng Windows font database và Qt 6 không còn
            # đóng gói lib/fonts. Giữ fatal warnings để phát hiện lỗi thật.
            env["QT_QPA_FONTDIR"] = str(PROJECT / "resources" / "fonts" / "Inter")
        for name in ("CAONGAN_LED_DATA_DIR", "CAONGAN_LED_CONFIG_DIR", "CAONGAN_LED_LOG_DIR"):
            env.pop(name, None)
        db = app_data / "data" / "snapshot.db"
        log = base / "app.log"
        with log.open("w+b") as output:
            process = subprocess.Popen([str(exe), "--preview", "--config", str(config_path)],
                                       cwd=base, env=env, stdout=output, stderr=subprocess.STDOUT)
            try:
                wait_for(process, db, lambda rows: len(rows) == 12, "đủ 12 kênh giả lập")
                simulator.emit_sample(sandbox, "fault", simulator.defaults(), measured + timedelta(seconds=1))
                wait_for(process, db, lambda rows: rows.get("co", [None, None])[1] == "02"
                         and rows.get("cod", [None, None])[1] == "02", "trạng thái thiết bị 02")
                simulator.emit_sample(sandbox, "wrong-unit", simulator.defaults(), measured + timedelta(seconds=2))
                wait_for(process, db, lambda rows: rows.get("pm", [None, None, None])[2] == "unit_mismatch",
                         "PM sai đơn vị")
                simulator.emit_sample(sandbox, "conflict", simulator.defaults(), measured + timedelta(seconds=3))
                wait_for(process, db, lambda rows: rows.get("so2", [None, None, None, None])[3] == 1,
                         "xung đột cùng timestamp")
            except Exception:
                output.flush()
                print(log.read_text(encoding="utf-8", errors="replace"), file=sys.stderr)
                raise
            finally:
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)


if __name__ == "__main__":
    main()
