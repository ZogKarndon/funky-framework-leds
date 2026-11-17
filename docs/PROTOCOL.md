# Host control protocol

USB CDC-ACM serial, **115200** baud, 8N1. Commands are single ASCII lines ending in `\n` (optional `\r`). Replies are also newline-terminated ASCII.

This protocol is **control only**. The flowing LED animation runs entirely on the RP2040; the host does not stream frames.

The Windows tray app (`windows-app/FunkyLedsTray`) uses this protocol. Global hotkeys map to `ON` / `OFF` / `BRIGHTNESS` with a local step of 8.

## Commands

| Command | Effect |
| --- | --- |
| `ON` | Enable display / resume animation |
| `OFF` | Blank all LEDs and pause animation |
| `BRIGHTNESS <0-48>` | Set NeoPixel brightness (aliases: `BRIGHT <n>`; hard-capped) |
| `STATUS` | Report power + brightness (alias: `GET`) |
| `BOOT` | Reboot into UF2 bootloader (alias: `BOOTLOADER`) |
| `HELP` | List commands |

Commands are case-insensitive.

## Responses

- `OK` — command accepted
- `STATUS power=on|off brightness=<0-48>` — after `STATUS`, `ON`, `OFF`, or brightness changes; also printed once at boot
- `ERR <message>` — bad input
- Boot banner: `FUNKY FW16 RGB ready`

## Examples

```text
> ON
OK
STATUS power=on brightness=16

> BRIGHTNESS 32
OK
STATUS power=on brightness=32

> OFF
OK
STATUS power=off brightness=32
```

## Host mapping (Windows tray)

| Hotkey / menu | Command sent |
| --- | --- |
| Ctrl+Alt+Right / LEDs On | `ON` |
| Ctrl+Alt+Left / LEDs Off | `OFF` |
| Ctrl+Alt+Up / Brightness Up | `BRIGHTNESS <current+8>` |
| Ctrl+Alt+Down / Brightness Down | `BRIGHTNESS <current-8>` |

## Manual serial example

```python
import serial
s = serial.Serial("COM5", 115200, timeout=1)
s.write(b"BRIGHTNESS 32\n")
print(s.readline())
print(s.readline())
```

Use Device Manager (Windows) or PlatformIO’s device list to find the COM port after flashing.
