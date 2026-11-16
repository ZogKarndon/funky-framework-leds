#!/usr/bin/env python3
"""Minimal CLI for funky_leds control protocol (no GUI)."""

from __future__ import annotations

import argparse
import sys
import time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Install pyserial: pip install -r requirements.txt", file=sys.stderr)
    sys.exit(1)


def main() -> int:
    p = argparse.ArgumentParser(description="Send one control command to funky_leds")
    p.add_argument("--port", help="Serial port (COM5, /dev/ttyACM0, …)")
    p.add_argument("--list", action="store_true", help="List serial ports and exit")
    p.add_argument(
        "command",
        nargs="*",
        help="Command words, e.g. ON  |  BRIGHTNESS 64  |  STATUS",
    )
    args = p.parse_args()

    if args.list or not args.port:
        ports = list(list_ports.comports())
        if not ports:
            print("No serial ports found")
        for port in ports:
            print(f"{port.device}\t{port.description}")
        if args.list or not args.command:
            return 0
        if not args.port:
            print("--port is required to send a command", file=sys.stderr)
            return 2

    if not args.command:
        print("Provide a command, e.g. STATUS", file=sys.stderr)
        return 2

    line = " ".join(args.command)
    with serial.Serial(args.port, 115200, timeout=1) as s:
        time.sleep(0.3)
        s.reset_input_buffer()
        s.write((line + "\n").encode("ascii"))
        s.flush()
        # Read a few reply lines.
        end = time.time() + 1.5
        while time.time() < end:
            raw = s.readline()
            if not raw:
                continue
            print(raw.decode("utf-8", errors="replace").rstrip())
            if raw.upper().startswith(b"STATUS") or raw.upper().startswith(b"ERR"):
                break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
