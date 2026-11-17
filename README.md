# funky-framework-leds

Stand-alone **horizontal soft-band** RGB animation for **Framework Laptop 16 RGB LED Matrix** modules (Joe Schroedl Rev 2 NeoPixel panels), plus a native Windows **system tray** app for power, brightness, and global hotkeys.

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

#### If `RPI-RP2` never appears (deeper checks)

Work these in order:

1. **Confirm DIP 2 orientation (photos)**  
   On the back near the connector: the raised bump on switch **2** must sit next to the printed **ON** label — fully to the **right**. Joe’s photos: [jschroedl.com/rgb-start](https://jschroedl.com/rgb-start/) (“Gently push switch 2 to the right”). If the bump is on the left, you are in normal run mode, not BOOTSEL.

2. **Switch 1 does not enter bootloader**  
   Leave switch **1** alone. On Framework-style modules, DIP1 is a user GPIO (official white matrix: GPIO25); only **DIP2 is BOOTSEL**. Flipping both switches is not required and can confuse the check.

3. **Bootloader vs firmware already running**  
   - DIP2 **ON** + power → expect **no app animation** and an **`RPI-RP2` / RP2 Boot** device.  
   - DIP2 **OFF** + power → expect normal firmware (stock Adalight often sits dark until OpenRGB; our firmware shows a flowing rainbow) and a **COM / USB Serial** port, **not** `RPI-RP2`.  
   If DIP2 is ON but LEDs still run and you only get a COM port, BOOTSEL is not actually asserted (wrong switch / not fully ON).

4. **Windows Device Manager (View → Devices by connection, then reinsert)**  
   Look for any of: **RP2 Boot**, **USB Mass Storage Device**, a new disk/volume **`RPI-RP2`**, USB VID **`2E8A`** (Raspberry Pi).  
   Also note the opposite case with DIP2 **OFF**: a new **USB Serial Device / COM port** means the deck USB path works and the MCU is alive — then the remaining issue is only BOOTSEL.  
   If **nothing** new appears for either DIP2 ON or OFF, the module is not enumerating on USB (bay/contact/power), not a PlatformIO problem.

   **Watch plug events while reseating** (Admin PowerShell):

   ```powershell
   # Live PnP add/remove stream — leave running, then reseat the module
   Register-WmiEvent -Class Win32_DeviceChangeEvent -SourceIdentifier ModPlug
   while ($true) {
     $e = Wait-Event -SourceIdentifier ModPlug
     "{0:HH:mm:ss} EventType={1}" -f $e.TimeGenerated, $e.SourceEventArgs.NewEvent.EventType
     Remove-Event -EventIdentifier $e.EventIdentifier
   }
   # EventType: 2 = Device Arrival, 3 = Device Removal (other values exist)
   # Ctrl+C when done, then: Unregister-Event ModPlug
   ```

   Snapshot helpers: `pnputil /enum-devices /connected` before/after reseat; Event Viewer → *Applications and Services Logs → Microsoft → Windows → Kernel-PnP → Configuration* (and DeviceSetupManager); optional GUI: **USBTreeView** / USBView.  
   **No arrival/removal at all** on reseat ⇒ Windows never saw a USB attach (deck power/USB path or dead module) — not a missing driver for `RPI-RP2`.

   **Identify what arrived** (run right after an Arrival, Admin PowerShell):

   ```powershell
   # Raspberry Pi Pico / RP2040 (bootloader or CDC) — VID 2E8A
   Get-PnpDevice -PresentOnly |
     Where-Object { $_.InstanceId -match 'VID_2E8A' } |
     Format-Table Status, Class, FriendlyName, InstanceId -Auto

   # Or broader: anything new-looking under USB / Ports / DiskDrive
   pnputil /enum-devices /connected /class USB
   pnputil /enum-devices /connected /class Ports
   pnputil /enum-devices /connected /class DiskDrive
   ```

   | What you see | Meaning | Next |
   | --- | --- | --- |
   | **RP2 Boot** / Disk **`RPI-RP2`** / `VID_2E8A&PID_0003` | BOOTSEL OK | `pio run -t upload` or drag `.uf2` onto `RPI-RP2` |
   | **USB Serial** / **COM** / `VID_2E8A&PID_000A` (or similar CDC) | App firmware running, not bootloader | DIP2 not ON at power-on — remove, DIP2 fully right/ON, reinsert |
   | Other VID/PID or unknown USB | Wrong/unexpected device | Paste **FriendlyName + InstanceId (VID/PID)** for help |
   | Arrival events but **no** `2E8A` device | USB attach without RP2040 enum | Reseat; try other bay; report InstanceIds from the commands above |

5. **Bay / USB path**  
   Reseat firmly; try the **other** half-size bay. The input deck must normally be **fully populated** for module power (BIOS can override “Input Module Power”). There is no separate flash cable in the laptop — the pogo connector *is* USB.

   **Dual-panel note (Framework 16):** On at least one FW16 setup, putting **only one** RGB panel into BOOTSEL (DIP2 ON) still enumerated as **COM** (run mode). Setting DIP2 ON for **both** input panels, then reseating, was what finally produced **`RPI-RP2`**. If a single module stubbornly shows COM despite DIP2 ON, put the sibling panel into BOOTSEL too (or ensure the deck is fully populated) and retry.

6. **RGB vs official white matrix**  
   - **RGB (Joe Schroedl):** color NeoPixels; can show full rainbows; this repo targets those.  
   - **Official Framework white:** monochrome white/amber LEDs via IS31FL3741 (9×34); same DIP2→BOOTSEL idea, but **this firmware is wrong** for them.  
   If unsure: with DIP2 OFF and stock firmware, OpenRGB/Adalight color control ⇒ RGB; Framework `inputmodule-control` / greyscale matrix tools ⇒ white.

7. **Standalone USB flash (if the deck path fails)**  
   Use a Framework **Input Module USB-C adapter** (or community external enclosure) so the module’s USB is a normal cable to the PC. DIP2 ON, then plug in power/USB — same `RPI-RP2` expectation. Use a **data** USB-C cable, not charge-only. Adapter docs: [Framework InputModules](https://github.com/FrameworkComputer/InputModules) / developer adapter program.

8. PlatformIO `pio run -t upload` (picotool) still needs BOOTSEL; it will not flash over a normal COM port for this project.

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
BRIGHTNESS 32
STATUS
BOOT
```

## Notes

- Default brightness is **16** (of 255); firmware and tray app hard-cap at **48**.
- Animation: soft full-height color bands travel **horizontally**, with width ≥2 px varying by row and a gentle sway (autonomous on-device).
- Tray brightness hotkeys step by **8** (clamped to 0–48).
- `BOOT` / `BOOTLOADER` jumps to the UF2 bootloader without flipping the DIP switch (handy for reflashing).

## Credits / references

- Hardware & stock firmware: [Joe Schroedl](https://jschroedl.com/store/), [adalight-multi-strip](https://github.com/corndog2000/adalight-multi-strip), [RGB-LED-Matrix-Input-Module](https://github.com/corndog2000/RGB-LED-Matrix-Input-Module)
- Framework input-module docs: [FrameworkComputer/InputModules](https://github.com/FrameworkComputer/InputModules)
- PlatformIO + Earle Philhower: [arduino-pico PlatformIO docs](https://arduino-pico.readthedocs.io/en/stable/platformio.html)
