# funky-framework-leds

Stand-alone flowing RGB animation for **Framework Laptop 16 RGB LED Matrix** modules (Joe Schroedl Rev 2 NeoPixel panels), plus a simple Windows control app for power and brightness.

The animation runs entirely on the module’s RP2040 — no desktop process is required to drive the LEDs. The host app only turns the display on/off and sets brightness.

> This repository started empty. Hardware pinout and board settings match the stock [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip) firmware used by these RGB modules.

## Hardware

| | |
| --- | --- |
| Module | Framework 16 RGB LED Matrix (third-party NeoPixel) |
| MCU | RP2040 |
| Layout | 8 strips × 36 LEDs (288 total) |
| Data pins | GPIO 15, 14, 13, 12, 11, 10, 9, 8 |
| Pixel order | GRB @ 800 kHz |
| Host link | USB CDC serial @ 115200 |

Official Framework **white** LED matrices use different hardware (IS31FL3741) and the `inputmodule-rs` protocol — this project targets the **RGB** NeoPixel modules.

## Repository layout

```
firmware/funky_leds/   Arduino sketch (autonomous animation + control protocol)
windows-app/           Python/Tk control UI for Windows
docs/PROTOCOL.md       Serial command reference
```

## Flash the firmware

### Requirements

- [Arduino IDE 2.x](https://www.arduino.cc/en/software)
- [Earle Philhower RP2040 core](https://github.com/earlephilhower/arduino-pico) (“Raspberry Pi Pico/RP2040” board package)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) library

### Board settings

Same as the stock RGB module firmware ([jschroedl.com/rgb-start](https://jschroedl.com/rgb-start/)):

- **Board:** Generic RP2040
- **Boot Stage 2:** W25Q16JVxQ QSPI /4
- **Flash Size:** 4MB (no FS)
- **USB Stack:** Pico SDK

### Steps

1. Remove the RGB LED module from the laptop.
2. Flip DIP switch **2** to **ON** (BOOTSEL).
3. Reinsert the module — an `RPI-RP2` drive should appear.
4. Open `firmware/funky_leds/funky_leds.ino` in Arduino IDE.
5. Install the Adafruit NeoPixel library if needed.
6. Select the board settings above, then **Sketch → Upload** (or copy the produced `.uf2` onto `RPI-RP2`).
7. Remove the module, set DIP switch **2** back to **OFF**, and reinsert.

On power-up the panel should immediately show a flowing rainbow. No host app is required.

To restore OpenRGB/Adalight behavior later, reflash [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip).

## Windows control app

Simple UI: pick the COM port, connect, toggle LEDs on/off, drag brightness.

### Run

```bat
cd windows-app
py -m pip install -r requirements.txt
py funky_leds_control.py
```

Optional: `py funky_leds_control.py --port COM5`

Or double-click `windows-app/run.bat`.

Headless one-shot commands (any OS with Python):

```bat
py cli.py --list
py cli.py --port COM5 STATUS
py cli.py --port COM5 BRIGHTNESS 64
```

The app sends the ASCII commands documented in [`docs/PROTOCOL.md`](docs/PROTOCOL.md) (`ON`, `OFF`, `BRIGHTNESS <0-255>`, `STATUS`).

### Serial from any OS

```text
ON
OFF
BRIGHTNESS 64
STATUS
BOOT
```

## Notes

- Default brightness is **40** (of 255) to stay near the input-module current budget. Raise carefully.
- `BOOT` / `BOOTLOADER` jumps to the UF2 bootloader without flipping the DIP switch (handy for reflashing).

## Credits / references

- Hardware & stock firmware: [Joe Schroedl](https://jschroedl.com/store/), [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip), [RGB-LED-Matrix-Input-Module](https://github.com/corndog2000/RGB-LED-Matrix-Input-Module)
- Framework input-module docs: [FrameworkComputer/InputModules](https://github.com/FrameworkComputer/InputModules)
