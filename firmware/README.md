# Firmware (PlatformIO)

Stand-alone flowing RGB animation + USB CDC power/brightness control for
Framework 16 RGB LED Matrix modules (8×36 NeoPixels on RP2040).

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
4. Put the module in BOOTSEL: remove it, set DIP switch **2** ON, reinsert (`RPI-RP2` appears).
5. **Upload:** PlatformIO → Upload, or `pio run -t upload`.
6. Remove the module, set DIP **2** OFF, reinsert. Animation starts immediately.

Artifacts land in `.pio/build/fw16_rgb/` (including a `.uf2` you can drag onto `RPI-RP2` if preferred).

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
