#!/usr/bin/env python3
"""Tạo TT10 giả trong sandbox riêng; không bao giờ sửa hoặc xóa file nguồn thật."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from decimal import Decimal
import os
from pathlib import Path
import re
import sys
import tempfile
import time

VIETNAM = timezone(timedelta(hours=7))
MARKER_NAME = ".caongan-led-test-sandbox"
MARKER_TEXT = "Cao Ngan LED synthetic TT10 sandbox v1\n"
DEFAULT_ROOT = Path.home() / ".local" / "share" / "caongan-led-test"
MAX_FILES_PER_SOURCE = 1000
MAX_CYCLES = 120
NUMBER = re.compile(r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)\Z", re.ASCII)


@dataclass(frozen=True)
class Channel:
    key: str
    symbol: str
    unit: str
    default: str
    source: str


CHANNELS = (
    Channel("o2", "O2", "%V", "5.6", "khi-thai"),
    Channel("co", "CO", "mg/Nm3", "46.0", "khi-thai"),
    Channel("nox", "NOx", "mg/Nm3", "97.0", "khi-thai"),
    Channel("so2", "SO2", "mg/Nm3", "89.0", "khi-thai"),
    Channel("pm", "PM", "mg/Nm3", "27.0", "khi-thai"),
    Channel("temp", "Temp", "oC", "127.0", "khi-thai"),
    Channel("flow", "Flow", "m3/h", "538964", "khi-thai"),
    Channel("pressure", "P", "kPa", "0.92", "khi-thai"),
    Channel("ph", "pH", "-", "7.9", "nuoc-thai"),
    Channel("tss", "TSS", "mg/L", "26.0", "nuoc-thai"),
    Channel("cod", "COD", "mg/L", "37.0", "nuoc-thai"),
    Channel("nh4", "NH4+", "mg/L", "0.87", "nuoc-thai"),
)
STATIONS = {"khi-thai": "KHITO1", "nuoc-thai": "NUONT1"}
SCENARIOS = {
    "normal": "Bình thường",
    "alarm": "Giá trị cao — cần tự đặt ngưỡng thử",
    "calibration": "Hiệu chuẩn 01",
    "fault": "Lỗi thiết bị 02",
    "stale": "Dữ liệu cũ — ngừng cập nhật O₂ và pH",
    "missing-ph": "Thiếu pH — ngừng cập nhật pH",
    "wrong-unit": "Sai đơn vị PM",
    "conflict": "Xung đột SO₂ cùng mốc đo",
    "long-flow": "Flow 6 chữ số",
    "unknown-status": "Trạng thái không xác định",
}


def defaults() -> dict[str, str]:
    return {channel.key: channel.default for channel in CHANNELS}


def _require_directory(path: Path) -> None:
    if path.is_symlink():
        raise ValueError(f"Không dùng thư mục symlink: {path}")
    path.mkdir(exist_ok=True)
    if not path.is_dir():
        raise ValueError(f"Không phải thư mục: {path}")


def prepare_sandbox(root: Path) -> Path:
    """Chỉ tạo trong root mới/rỗng hoặc sandbox đã có marker của công cụ."""
    root = root.expanduser()
    if not root.is_absolute():
        raise ValueError("Sandbox phải là đường dẫn tuyệt đối")
    if root.is_symlink():
        raise ValueError(f"Không dùng sandbox symlink: {root}")
    if root.exists() and not root.is_dir():
        raise ValueError(f"Sandbox không phải thư mục: {root}")
    marker = root / MARKER_NAME
    if not marker.exists():
        if root.exists() and any(root.iterdir()):
            raise ValueError(f"Thư mục không rỗng và không có marker sandbox: {root}")
        root.mkdir(parents=True, exist_ok=True)
        with marker.open("x", encoding="utf-8", newline="\n") as file:
            file.write(MARKER_TEXT)
    if marker.is_symlink() or not marker.is_file() or marker.read_text(encoding="utf-8") != MARKER_TEXT:
        raise ValueError(f"Marker sandbox không hợp lệ: {marker}")
    inputs = root / "inputs"
    _require_directory(inputs)
    for name in STATIONS:
        _require_directory(inputs / name)
    return root


def _validate_values(values: dict[str, str]) -> dict[str, str]:
    expected = {channel.key for channel in CHANNELS}
    if set(values) != expected:
        raise ValueError("Cần đủ đúng 12 giá trị")
    result = {}
    for channel in CHANNELS:
        text = str(values[channel.key]).strip()
        if len(text) > 30 or not NUMBER.fullmatch(text) or not Decimal(text).is_finite():
            raise ValueError(f"{channel.symbol}: nhập số thập phân hữu hạn, dấu chấm, tối đa 30 ký tự")
        result[channel.key] = text
    return result


def _publish(folder: Path, stem: str, content: bytes) -> Path:
    """Ghi file tạm rồi hard-link atomic, không ghi đè bất kỳ file nào đã có."""
    temporary: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(prefix=".caongan-sim-", suffix=".part", dir=folder, delete=False) as file:
            temporary = Path(file.name)
            file.write(content)
            file.flush()
        for suffix in range(1000):
            name = f"{stem}{'_' + str(suffix) if suffix else ''}.txt"
            target = folder / name
            try:
                os.link(temporary, target)
                return target
            except FileExistsError:
                continue
        raise RuntimeError("Quá nhiều file có cùng timestamp trong sandbox")
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def emit_sample(root: Path, scenario: str, values: dict[str, str], now: datetime | None = None) -> list[Path]:
    if scenario not in SCENARIOS:
        raise ValueError(f"Không có cảnh {scenario}")
    root = prepare_sandbox(root)
    numbers = _validate_values(values)
    if scenario == "alarm":
        numbers["so2"], numbers["cod"] = "9999", "9999"
    elif scenario == "long-flow":
        numbers["flow"] = "123456"

    moment = now if now is not None else datetime.now(timezone.utc)
    if moment.tzinfo is None or moment.utcoffset() is None:
        raise ValueError("Thời gian mẫu phải có timezone")
    timestamp = moment.astimezone(VIETNAM).strftime("%Y%m%d%H%M%S")
    rows: dict[str, list[str]] = {source: [] for source in STATIONS}
    for channel in CHANNELS:
        if (scenario == "stale" and channel.key in {"o2", "ph"}) or (scenario == "missing-ph" and channel.key == "ph"):
            continue
        status = "00"
        if scenario == "calibration" and channel.key in {"so2", "ph"}:
            status = "01"
        elif scenario == "fault" and channel.key in {"co", "cod"}:
            status = "02"
        elif scenario == "unknown-status" and channel.key == "co":
            status = "03"
        unit = "mg/m3" if scenario == "wrong-unit" and channel.key == "pm" else channel.unit
        rows[channel.source].append("\t".join((channel.symbol, numbers[channel.key], unit, timestamp, status)))

    extra = 1 if scenario == "conflict" else 0
    for source in STATIONS:
        folder = root / "inputs" / source
        count = sum(1 for item in folder.glob("SIM_TEST_*.txt") if item.is_file())
        if count + 1 + (extra if source == "khi-thai" else 0) > MAX_FILES_PER_SOURCE:
            raise RuntimeError(f"Sandbox đã đủ {MAX_FILES_PER_SOURCE} file trong {folder}; dừng để tránh đầy đĩa")

    paths = []
    for source, station in STATIONS.items():
        content = ("\n".join(rows[source]) + "\n").encode("utf-8")
        stem = f"SIM_TEST_{station}_{timestamp}"
        paths.append(_publish(root / "inputs" / source, stem, content))
    if scenario == "conflict":
        changed = format(Decimal(numbers["so2"]) + Decimal(10), "f")
        line = f"SO2\t{changed}\tmg/Nm3\t{timestamp}\t00\n".encode("utf-8")
        paths.append(_publish(root / "inputs" / "khi-thai", f"SIM_TEST_KHITO1_{timestamp}", line))
    return paths


def run_gui(root: Path, interval: int) -> None:
    import tkinter as tk
    from tkinter import messagebox, ttk

    root = prepare_sandbox(root)
    try:
        window = tk.Tk()
    except tk.TclError as error:
        raise RuntimeError(f"Không mở được cửa sổ đồ họa: {error}") from error
    window.title("Cao Ngạn — SIMULATOR DỮ LIỆU GIẢ")
    window.geometry("760x670")
    window.minsize(690, 600)
    main = ttk.Frame(window, padding=14)
    main.pack(fill="both", expand=True)
    ttk.Label(main, text="DỮ LIỆU GIẢ LẬP — CHỈ DÙNG TRONG PREVIEW", foreground="#b00020", font=("", 13, "bold")).pack(anchor="w")
    ttk.Label(main, text=f"Sandbox: {root}", wraplength=700).pack(anchor="w", pady=(3, 10))

    form = ttk.LabelFrame(main, text="Giá trị đầu vào", padding=10)
    form.pack(fill="x")
    variables: dict[str, tk.StringVar] = {}
    for index, channel in enumerate(CHANNELS):
        column, row = (0 if index < 6 else 3), index % 6
        var = tk.StringVar(value=channel.default)
        variables[channel.key] = var
        ttk.Label(form, text=channel.symbol, width=7).grid(row=row, column=column, sticky="w", pady=3)
        ttk.Entry(form, textvariable=var, width=14).grid(row=row, column=column + 1, sticky="w", padx=4)
        ttk.Label(form, text=channel.unit, width=10).grid(row=row, column=column + 2, sticky="w")

    settings = ttk.Frame(main)
    settings.pack(fill="x", pady=(12, 5))
    scenario = tk.StringVar(value=SCENARIOS["normal"])
    ttk.Label(settings, text="Tình huống:").grid(row=0, column=0, sticky="w")
    ttk.Combobox(settings, textvariable=scenario, values=list(SCENARIOS.values()), state="readonly", width=48).grid(row=0, column=1, padx=8, sticky="w")
    delay = tk.IntVar(value=interval)
    ttk.Label(settings, text="Chu kỳ (giây):").grid(row=1, column=0, sticky="w", pady=8)
    ttk.Spinbox(settings, from_=2, to=60, textvariable=delay, width=8).grid(row=1, column=1, sticky="w", padx=8)
    ttk.Label(main, text="Dữ liệu cũ/thiếu pH: ghi Bình thường trước, rồi chuyển cảnh và chờ quá thời gian stale.\nGiá trị cao chỉ báo vượt khi tự đặt ngưỡng thử trong cửa sổ vận hành.", wraplength=700).pack(anchor="w", pady=4)
    status = tk.StringVar(value="Chưa ghi file. Chọn tình huống rồi bấm Ghi ngay hoặc Bắt đầu.")
    ttk.Label(main, textvariable=status, wraplength=700).pack(anchor="w", pady=(8, 5))

    running = False
    callback: str | None = None
    cycles = 0
    last_emission = 0.0

    def write_sample() -> bool:
        nonlocal last_emission
        if time.monotonic() - last_emission < 1.0:
            status.set("Mốc TT10 có độ phân giải 1 giây; hãy chờ một chút trước lần ghi tiếp theo.")
            return True
        try:
            code = next(key for key, label in SCENARIOS.items() if label == scenario.get())
            files = emit_sample(root, code, {key: var.get() for key, var in variables.items()})
            last_emission = time.monotonic()
            status.set(f"Đã ghi {len(files)} file giả lúc {datetime.now().strftime('%H:%M:%S')} — {scenario.get()}")
            return True
        except (ValueError, OSError, RuntimeError, StopIteration) as error:
            status.set(f"Dừng: {error}")
            messagebox.showerror("Simulator", str(error), parent=window)
            return False

    def stop() -> None:
        nonlocal running, callback
        running = False
        if callback is not None:
            window.after_cancel(callback)
            callback = None
        start_button.configure(state="normal")
        stop_button.configure(state="disabled")

    def tick() -> None:
        nonlocal callback, cycles
        callback = None
        if not running:
            return
        if not write_sample():
            stop()
            return
        cycles += 1
        if cycles >= MAX_CYCLES:
            stop()
            status.set(f"Đã tự dừng sau {MAX_CYCLES} lần ghi. Bấm Bắt đầu nếu cần tiếp tục thử.")
            return
        try:
            seconds = delay.get()
            if not 2 <= seconds <= 60:
                raise ValueError("Chu kỳ phải từ 2 đến 60 giây")
        except (tk.TclError, ValueError) as error:
            stop()
            messagebox.showerror("Simulator", str(error), parent=window)
            return
        callback = window.after(seconds * 1000, tick)

    def start() -> None:
        nonlocal running, cycles
        if running:
            return
        running, cycles = True, 0
        start_button.configure(state="disabled")
        stop_button.configure(state="normal")
        tick()

    controls = ttk.Frame(main)
    controls.pack(fill="x", pady=8)
    ttk.Button(controls, text="Ghi ngay", command=write_sample).pack(side="left", padx=(0, 8))
    start_button = ttk.Button(controls, text="Bắt đầu", command=start)
    start_button.pack(side="left", padx=(0, 8))
    stop_button = ttk.Button(controls, text="Dừng", command=stop, state="disabled")
    stop_button.pack(side="left")

    def close() -> None:
        stop()
        window.destroy()

    window.protocol("WM_DELETE_WINDOW", close)
    window.mainloop()


def main() -> int:
    parser = argparse.ArgumentParser(description="Simulator file TT10 giả, chỉ ghi vào sandbox được đánh dấu.")
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help="Sandbox tuyệt đối (mặc định ~/.local/share/caongan-led-test)")
    parser.add_argument("--prepare", action="store_true", help="Chỉ tạo sandbox mới/rỗng và in đường dẫn")
    parser.add_argument("--once", action="store_true", help="Ghi một bộ file rồi thoát thay vì mở GUI")
    parser.add_argument("--scenario", choices=SCENARIOS, default="normal", help="Tình huống cho --once")
    parser.add_argument("--interval", type=int, default=5, help="Chu kỳ GUI ban đầu, 2–60 giây")
    args = parser.parse_args()
    try:
        if args.prepare and args.once:
            parser.error("--prepare và --once không dùng cùng nhau")
        if not 2 <= args.interval <= 60:
            parser.error("--interval phải từ 2 đến 60 giây")
        if args.prepare:
            root = prepare_sandbox(args.root)
            print(f"Sandbox thử: {root}")
            for source in STATIONS:
                print(f"{source}: {root / 'inputs' / source}")
        elif args.once:
            for path in emit_sample(args.root, args.scenario, defaults()):
                print(path)
        else:
            run_gui(args.root, args.interval)
        return 0
    except (OSError, ValueError, RuntimeError, ImportError) as error:
        print(f"Simulator: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
