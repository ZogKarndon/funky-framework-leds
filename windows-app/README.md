# Windows tray app (WinForms)

Native **.NET 8 WinForms** system-tray controller for funky_leds firmware.

Chosen stack: **WinForms + `NotifyIcon` + Win32 `RegisterHotKey`** — lightweight native Windows UI, no Tk/web runtime, straightforward global hotkeys and tray menus.

## Features

- Runs in the system tray (single-instance)
- Auto-connects to the module’s USB CDC COM port
- Tray menu: on / off / brightness / reconnect / exit
- Global hotkeys:
  - **Ctrl+Alt+Up** — brightness up (+8)
  - **Ctrl+Alt+Down** — brightness down (−8)
  - **Ctrl+Alt+Right** — LEDs on
  - **Ctrl+Alt+Left** — LEDs off

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
