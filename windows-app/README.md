# Windows tray app (WinForms)

Native **.NET 8 WinForms** system-tray controller for funky_leds firmware.

Chosen stack: **WinForms + `NotifyIcon` + Win32 `RegisterHotKey`** — lightweight native Windows UI, no Tk/web runtime, straightforward global hotkeys and tray menus.

## Features

- Runs in the system tray (single-instance)
- Auto-discovers **every** funky_leds USB CDC COM port (both Framework RGB panels) and fans out ON/OFF/brightness to all of them
- Tray menu: on / off / brightness / reconnect / exit
- Brightness range **0–48** (default **16**); hotkeys and menu clamp to that max; step **1**
- Global hotkeys:
  - **Ctrl+Alt+Up** — brightness up (+1)
  - **Ctrl+Alt+Down** — brightness down (−1)
  - **Ctrl+Alt+Right** — LEDs on
  - **Ctrl+Alt+Left** — LEDs off

### Dual-panel discovery

On connect / reconnect the tray app:

1. Lists all COM ports
2. Opens each briefly, sends `STATUS`, and keeps the port only if the reply matches funky_leds (`STATUS power=…` or the `FUNKY FW16 RGB` boot banner)
3. Holds **all** matching ports open (typically two panels → two COMs)
4. Fans out every control command (`ON` / `OFF` / `BRIGHTNESS` / `STATUS`) to every open port

Non-matching serial devices are closed immediately and ignored.

## Build & run (Windows)

Requirements: [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0) (or Visual Studio 2022+ with .NET desktop workload).

```bat
cd windows-app
dotnet build FunkyLedsTray\FunkyLedsTray.csproj -c Release
dotnet run --project FunkyLedsTray\FunkyLedsTray.csproj -c Release
```

Or open `FunkyLedsTray.sln` in Visual Studio and press F5.

The Release binary is at:

`FunkyLedsTray\bin\Release\net8.0-windows\FunkyLedsTray.exe`

Copy that folder (or publish) onto the Framework laptop and run the EXE. Leave it running in the tray while you use the hotkeys.

Publish a self-contained EXE (optional):

```bat
dotnet publish FunkyLedsTray\FunkyLedsTray.csproj -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -o publish
```

## Protocol

Uses the ASCII control protocol in [`docs/PROTOCOL.md`](../docs/PROTOCOL.md). The host never streams animation frames.
