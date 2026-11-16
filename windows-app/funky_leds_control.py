#!/usr/bin/env python3
"""
Framework 16 RGB LED Matrix — simple on/off + brightness control.

Talks to funky_leds firmware over USB CDC serial (115200 baud).
See docs/PROTOCOL.md for the command set.
"""

from __future__ import annotations

import argparse
import queue
import re
import sys
import threading
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing dependency: pyserial. Install with: pip install -r requirements.txt")
    sys.exit(1)


BAUD = 115200
STATUS_RE = re.compile(
    r"STATUS\s+power=(on|off)\s+brightness=(\d+)", re.IGNORECASE
)


def list_serial_ports() -> list[str]:
    ports = list(list_ports.comports())
    # Prefer RP2040 / Pico / Framework-looking devices first.
    def score(p) -> tuple:
        blob = f"{p.description} {p.manufacturer or ''} {p.hwid}".lower()
        prefer = any(
            k in blob
            for k in ("pico", "rp2040", "raspberry", "adafruit", "framework", "usb serial")
        )
        return (0 if prefer else 1, p.device)

    ports.sort(key=score)
    return [p.device for p in ports]


class DeviceLink:
    def __init__(self) -> None:
        self._ser: serial.Serial | None = None
        self._lock = threading.Lock()
        self.inbox: queue.Queue[str] = queue.Queue()
        self._reader: threading.Thread | None = None
        self._stop = threading.Event()

    @property
    def connected(self) -> bool:
        return self._ser is not None and self._ser.is_open

    def connect(self, port: str) -> None:
        self.disconnect()
        ser = serial.Serial(port, BAUD, timeout=0.2)
        self._ser = ser
        self._stop.clear()
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

    def disconnect(self) -> None:
        self._stop.set()
        if self._reader and self._reader.is_alive():
            self._reader.join(timeout=1.0)
        self._reader = None
        with self._lock:
            if self._ser is not None:
                try:
                    self._ser.close()
                except Exception:
                    pass
                self._ser = None

    def send(self, line: str) -> None:
        if not self.connected:
            raise RuntimeError("Not connected")
        data = (line.strip() + "\n").encode("ascii", errors="ignore")
        with self._lock:
            assert self._ser is not None
            self._ser.write(data)
            self._ser.flush()

    def _read_loop(self) -> None:
        buf = bytearray()
        while not self._stop.is_set():
            try:
                with self._lock:
                    ser = self._ser
                if ser is None:
                    break
                chunk = ser.read(256)
                if not chunk:
                    continue
                buf.extend(chunk)
                while True:
                    nl = buf.find(b"\n")
                    if nl < 0:
                        break
                    raw = bytes(buf[:nl]).decode("utf-8", errors="replace").strip("\r")
                    del buf[: nl + 1]
                    if raw:
                        self.inbox.put(raw)
            except Exception as exc:
                self.inbox.put(f"ERR reader: {exc}")
                break


class ControlApp(tk.Tk):
    def __init__(self, initial_port: str | None = None) -> None:
        super().__init__()
        self.title("Funky Framework LEDs")
        self.geometry("420x320")
        self.minsize(380, 280)
        self.configure(padx=16, pady=16)

        self.link = DeviceLink()
        self.power_var = tk.BooleanVar(value=True)
        self.brightness_var = tk.IntVar(value=40)
        self.status_var = tk.StringVar(value="Disconnected")
        self.port_var = tk.StringVar()

        self._build_ui()
        self._refresh_ports()
        if initial_port:
            self.port_var.set(initial_port)

        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self.after(100, self._drain_inbox)

    def _build_ui(self) -> None:
        header = ttk.Label(self, text="Framework 16 RGB LEDs", font=("Segoe UI", 14, "bold"))
        header.pack(anchor="w")

        sub = ttk.Label(
            self,
            text="On-device flowing animation · host only toggles power & brightness",
            wraplength=380,
        )
        sub.pack(anchor="w", pady=(0, 12))

        conn = ttk.LabelFrame(self, text="Connection", padding=10)
        conn.pack(fill="x")

        row = ttk.Frame(conn)
        row.pack(fill="x")
        ttk.Label(row, text="Port").pack(side="left")
        self.port_box = ttk.Combobox(row, textvariable=self.port_var, width=28, state="readonly")
        self.port_box.pack(side="left", padx=8)
        ttk.Button(row, text="Refresh", command=self._refresh_ports).pack(side="left")
        self.connect_btn = ttk.Button(row, text="Connect", command=self._toggle_connect)
        self.connect_btn.pack(side="left", padx=(8, 0))

        controls = ttk.LabelFrame(self, text="Display", padding=10)
        controls.pack(fill="x", pady=(12, 0))

        power_row = ttk.Frame(controls)
        power_row.pack(fill="x")
        self.power_check = ttk.Checkbutton(
            power_row,
            text="LEDs on",
            variable=self.power_var,
            command=self._on_power_toggle,
            state="disabled",
        )
        self.power_check.pack(side="left")

        bright_row = ttk.Frame(controls)
        bright_row.pack(fill="x", pady=(10, 0))
        ttk.Label(bright_row, text="Brightness").pack(side="left")
        self.bright_label = ttk.Label(bright_row, text="40", width=4)
        self.bright_label.pack(side="right")
        self.bright_scale = ttk.Scale(
            controls,
            from_=0,
            to=255,
            orient="horizontal",
            command=self._on_brightness_slide,
            state="disabled",
        )
        self.bright_scale.set(40)
        self.bright_scale.pack(fill="x", pady=(4, 0))
        self.bright_scale.bind("<ButtonRelease-1>", self._on_brightness_commit)

        status = ttk.Label(self, textvariable=self.status_var, foreground="#333")
        status.pack(anchor="w", pady=(14, 0))

        log_frame = ttk.LabelFrame(self, text="Device", padding=8)
        log_frame.pack(fill="both", expand=True, pady=(10, 0))
        self.log = tk.Text(log_frame, height=6, wrap="word", state="disabled")
        self.log.pack(fill="both", expand=True)

    def _append_log(self, line: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", line + "\n")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _refresh_ports(self) -> None:
        ports = list_serial_ports()
        self.port_box["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])
        elif self.port_var.get() and self.port_var.get() not in ports:
            self.port_var.set(ports[0] if ports else "")

    def _set_controls_enabled(self, enabled: bool) -> None:
        state = "normal" if enabled else "disabled"
        self.power_check.configure(state=state)
        self.bright_scale.configure(state=state)

    def _toggle_connect(self) -> None:
        if self.link.connected:
            self.link.disconnect()
            self.connect_btn.configure(text="Connect")
            self._set_controls_enabled(False)
            self.status_var.set("Disconnected")
            self._append_log("Disconnected")
            return

        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("No port", "Select a serial port first.")
            return
        try:
            self.link.connect(port)
            self.connect_btn.configure(text="Disconnect")
            self._set_controls_enabled(True)
            self.status_var.set(f"Connected to {port}")
            self._append_log(f"Connected to {port}")
            # Ask firmware for current state.
            self.link.send("STATUS")
        except Exception as exc:
            messagebox.showerror("Connect failed", str(exc))
            self.status_var.set("Connect failed")

    def _on_power_toggle(self) -> None:
        if not self.link.connected:
            return
        cmd = "ON" if self.power_var.get() else "OFF"
        try:
            self.link.send(cmd)
            self._append_log(f"> {cmd}")
        except Exception as exc:
            messagebox.showerror("Send failed", str(exc))

    def _on_brightness_slide(self, value: str) -> None:
        v = int(float(value))
        self.brightness_var.set(v)
        self.bright_label.configure(text=str(v))

    def _on_brightness_commit(self, _event=None) -> None:
        if not self.link.connected:
            return
        v = int(self.brightness_var.get())
        cmd = f"BRIGHTNESS {v}"
        try:
            self.link.send(cmd)
            self._append_log(f"> {cmd}")
        except Exception as exc:
            messagebox.showerror("Send failed", str(exc))

    def _drain_inbox(self) -> None:
        while True:
            try:
                line = self.link.inbox.get_nowait()
            except queue.Empty:
                break
            self._append_log(line)
            m = STATUS_RE.search(line)
            if m:
                on = m.group(1).lower() == "on"
                bright = int(m.group(2))
                self.power_var.set(on)
                self.brightness_var.set(bright)
                self.bright_scale.set(bright)
                self.bright_label.configure(text=str(bright))
                self.status_var.set(
                    f"{'On' if on else 'Off'} · brightness {bright}"
                )
        self.after(100, self._drain_inbox)

    def _on_close(self) -> None:
        self.link.disconnect()
        self.destroy()


def main() -> int:
    parser = argparse.ArgumentParser(description="Funky Framework LEDs control")
    parser.add_argument("--port", help="Serial port (e.g. COM5 or /dev/ttyACM0)")
    args = parser.parse_args()
    app = ControlApp(initial_port=args.port)
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
