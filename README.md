# funky-framework-leds

Stand-alone flowing RGB animation for **Framework Laptop 16 RGB LED Matrix** modules (Joe Schroedl Rev 2 NeoPixel panels), plus a native Windows **system tray** app for power, brightness, and global hotkeys.

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
firmware/                 PlatformIO project (VS Code primary workflow)
  platformio.ini
  include/config.h
  src/main.cpp
windows-app/              .NET 8 WinForms system-tray app
  FunkyLedsTray.sln
  FunkyLedsTray/
docs/PROTOCOL.md          Serial command reference
```

## Flash the firmware (PlatformIO + VS Code)

### Requirements

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension  
  (or PlatformIO Core CLI: `pip install platformio`)
- USB access to the module in BOOTSEL mode

### Build & upload

From the `firmware/` directory:

```bash
pio run                 # build
pio run -t upload       # flash (module must be in BOOTSEL)
pio device monitor      # 115200 serial monitor
```

In VS Code: open `firmware/`, pick env **`fw16_rgb`**, then use the PlatformIO Build / Upload buttons.

### BOOTSEL steps

The RP2040 only enters the UF2 bootloader when **DIP switch 2 is ON at the moment USB power is applied**. Flipping the switch while the module is already powered usually does nothing.

1. **Remove** the RGB LED module from the laptop (it must be unpowered).
2. Flip the module over. Near the main-board connector are **two** small DIP switches. Using tweezers, gently push switch **2** all the way to the **right**, toward the printed **ON** label. (Leave switch 1 alone.)
3. **Reinsert** the module into the Framework 16 input deck — that reinsertion is the power cycle.
4. Windows should show a mass-storage volume named **`RPI-RP2`** (File Explorer). In Device Manager look for an **RP2 Boot** device. Linux/macOS: a similar `RPI-RP2` mount.
5. Upload with PlatformIO (`pio run -t upload`), which uses **picotool** against the BOOTSEL USB device, **or** drag `.pio/build/fw16_rgb/firmware.uf2` onto the `RPI-RP2` drive. Either path needs BOOTSEL; drag-drop specifically needs the `RPI-RP2` volume.
6. After upload finishes, **remove** the module, slide switch **2** back **left** (away from ON), and reinsert. Animation should start immediately with no host app.

#### If `RPI-RP2` never appears

- Confirm you powered the module **after** setting switch 2 (remove → switch → reinsert). Do not flip the switch in-place and expect BOOTSEL.
- Confirm it is switch **2**, fully to the **right / ON**, not switch 1 and not a half-toggle.
- Confirm this is a **Joe Schroedl RGB** NeoPixel module (or another RP2040 RGB panel with the same DIP). Official Framework **white** LED matrices are different hardware; they also use DIP2 for BOOTSEL but this firmware is not for them.
- Try another input-deck bay / reseat firmly. The laptop’s deck USB connection is the cable — there is no separate USB wire for a normal flash.
- On Windows: Device Manager → look for **RP2 Boot**. If nothing new appears on insert, the MCU is not in bootloader (or not getting USB).
- PlatformIO upload (`picotool`) also requires BOOTSEL; it will not fall back to a normal COM-port serial flash for this project.

See also [Joe Schroedl’s RGB programming steps](https://jschroedl.com/rgb-start/).

`platformio.ini` encodes the stock module settings: Generic RP2040, Earle Philhower core, Boot2 **W25Q16JVxQ QSPI /4**, 4MB flash with no filesystem, Adafruit NeoPixel.

To restore OpenRGB/Adalight behavior later, reflash [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip).

## Windows tray app (.NET 8 WinForms)

Native system-tray controller (not Tk/web). Stack choice: **WinForms + tray icon + Win32 global hotkeys** — simple, native, and a good fit for a always-on control helper.

### Build & run (on Windows)

```bat
cd windows-app
dotnet build FunkyLedsTray\FunkyLedsTray.csproj -c Release
dotnet run --project FunkyLedsTray\FunkyLedsTray.csproj -c Release
```

Or open `windows-app/FunkyLedsTray.sln` in Visual Studio.

The app lives in the tray, auto-connects to the module COM port, and exposes:

| Hotkey | Action |
| --- | --- |
| **Ctrl+Alt+Up** | Brightness up |
| **Ctrl+Alt+Down** | Brightness down |
| **Ctrl+Alt+Right** | LEDs on |
| **Ctrl+Alt+Left** | LEDs off |

Right-click the tray icon for the same actions, reconnect, and exit. See [`windows-app/README.md`](windows-app/README.md).

### Protocol

ASCII lines over USB CDC @ 115200 — see [`docs/PROTOCOL.md`](docs/PROTOCOL.md):

```text
ON
OFF
BRIGHTNESS 64
STATUS
BOOT
```

## Notes

- Default brightness is **40** (of 255) to stay near the input-module current budget. Raise carefully.
- Tray brightness hotkeys step by **8**.
- `BOOT` / `BOOTLOADER` jumps to the UF2 bootloader without flipping the DIP switch (handy for reflashing).

## Credits / references

- Hardware & stock firmware: [Joe Schroedl](https://jschroedl.com/store/), [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip), [RGB-LED-Matrix-Input-Module](https://github.com/corndog2000/RGB-LED-Matrix-Input-Module)
- Framework input-module docs: [FrameworkComputer/InputModules](https://github.com/FrameworkComputer/InputModules)
- PlatformIO + Earle Philhower: [arduino-pico PlatformIO docs](https://arduino-pico.readthedocs.io/en/stable/platformio.html)
