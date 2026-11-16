# Host control protocol

USB CDC-ACM serial, **115200** baud, 8N1. Commands are single ASCII lines ending in `\n` (optional `\r`). Replies are also newline-terminated ASCII.

This protocol is **control only**. The flowing LED animation runs entirely on the RP2040; the host does not stream frames.

## Commands

| Command | Effect |
| --- | --- |
| `ON` | Enable display / resume animation |
| `OFF` | Blank all LEDs and pause animation |
| `BRIGHTNESS <0-255>` | Set NeoPixel brightness (aliases: `BRIGHT <n>`) |
| `STATUS` | Report power + brightness (alias: `GET`) |
| `BOOT` | Reboot into UF2 bootloader (alias: `BOOTLOADER`) |
| `HELP` | List commands |

Commands are case-insensitive.

## Responses

- `OK` — command accepted
- `STATUS power=on|off brightness=<0-255>` — after `STATUS`, `ON`, `OFF`, or brightness changes; also printed once at boot
- `ERR <message>` — bad input
- Boot banner: `FUNKY FW16 RGB ready`

## Examples

```text
> ON
OK
STATUS power=on brightness=40

> BRIGHTNESS 80
OK
STATUS power=on brightness=80

> OFF
OK
STATUS power=off brightness=80
```

## Windows / Python host helpers

```python
import serial
s = serial.Serial("COM5", 115200, timeout=1)
s.write(b"BRIGHTNESS 64\n")
print(s.readline())
print(s.readline())
```

Use Device Manager (Windows) or `python -m serial.tools.list_ports` to find the COM port after flashing.
