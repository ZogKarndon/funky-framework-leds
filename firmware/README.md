# Firmware (PlatformIO)

Stand-alone flowing RGB animation + USB CDC power/brightness control for
Framework 16 RGB LED Matrix modules (9×32 NeoPixels on RP2040).

## Layout

```
firmware/
  platformio.ini
  include/config.h
  src/main.cpp
```

## Build & flash (VS Code + PlatformIO)

1. Install [VS Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension.
2. Open the `firmware/` folder (or this repo and select the `fw16_rgb` env).
3. **Build:** PlatformIO → Build, or `pio run` from `firmware/`.
4. Put the module in BOOTSEL: **remove** it (must be unpowered), set DIP switch **2** fully **right / ON** (on the back, near the connector; leave switch 1 alone), then **reinsert**. An `RPI-RP2` volume / “RP2 Boot” device should appear. Flipping DIP2 while already powered usually does nothing.
5. **Upload:** PlatformIO → Upload (`pio run -t upload` via picotool), or drag `.pio/build/fw16_rgb/firmware.uf2` onto `RPI-RP2`.
6. Remove the module, set DIP **2** back **left / OFF**, reinsert. Animation starts immediately.

If `RPI-RP2` never appears, see the troubleshooting notes in the root README.

Serial monitor: `pio device monitor` (115200 baud).

## Board settings encoded in `platformio.ini`

| Setting | Value |
| --- | --- |
| Board | Generic RP2040 (`board = generic`) |
| Core | Earle Philhower (`board_build.core = earlephilhower`) |
| Boot2 | `boot2_w25q16jvxq_4_padded_checksum.S` |
| Filesystem | none (`0m`) |
| Library | Adafruit NeoPixel |

Protocol: [`docs/PROTOCOL.md`](../docs/PROTOCOL.md).
