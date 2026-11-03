#!/usr/bin/env python3
"""Kiểm tra preview thường và preview mô phỏng dùng các vùng dữ liệu riêng."""

import os
from contextlib import closing
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile


def main() -> None:
    executable = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="caongan-preview-") as directory:
        root = Path(directory)
        runtime = root / "xdg"
        runtime.mkdir(mode=0o700)
        env = os.environ.copy()
        for key in ("CAONGAN_LED_DATA_DIR", "CAONGAN_LED_CONFIG_DIR", "CAONGAN_LED_LOG_DIR"):
            env.pop(key, None)
        # Qt offscreen trên Windows có thể cảnh báo thiếu font hệ thống;
        # smoke package dùng nền tảng Windows thật vẫn kiểm tra fatal warnings.
        env.pop("QT_FATAL_WARNINGS", None)
        env.update(QT_QPA_PLATFORM="offscreen", XDG_RUNTIME_DIR=str(runtime))

        def run_preview(name: str, *args: str) -> Path:
            base = root / name
            local_env = env | {"CAONGAN_LED_BASE_DIR": str(base)}
            result = subprocess.run(
                [executable, "--preview", *args, "--verify-preview-windows"],
                env=local_env, capture_output=True, text=True, encoding="utf-8",
                errors="replace", timeout=30, check=False,
            )
            output = result.stdout + result.stderr
            if result.returncode != 0 or "PREVIEW_WINDOWS_OK" not in output:
                raise AssertionError(f"Preview {name} thất bại ({result.returncode}):\n{output}")
            return base / "data" / "snapshot.db"

        snapshot = run_preview("normal")
        if not snapshot.is_file():
            raise AssertionError("Preview thường không khởi tạo snapshot.db")
        with closing(sqlite3.connect(snapshot)) as database:
            tables = {row[0] for row in database.execute("SELECT name FROM sqlite_master WHERE type='table'")}
            if "latest_reading" not in tables:
                raise AssertionError("Snapshot thiếu bảng latest_reading")

        simulated_snapshot = run_preview("simulation", "--simulate")
        if simulated_snapshot.exists():
            raise AssertionError("Preview mô phỏng không được tạo snapshot.db")


if __name__ == "__main__":
    main()
