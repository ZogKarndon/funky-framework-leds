using System;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace FunkyLedsTray;

internal sealed class TrayApplicationContext : ApplicationContext
{
    private readonly LedController _leds = new();
    private readonly NotifyIcon _tray;
    private readonly HotkeyWindow _hotkeys;
    private readonly ToolStripMenuItem _statusItem;
    private readonly ToolStripMenuItem _portItem;
    private readonly ToolStripMenuItem _onItem;
    private readonly ToolStripMenuItem _offItem;
    private readonly System.Windows.Forms.Timer _reconnectTimer;

    public TrayApplicationContext()
    {
        _hotkeys = new HotkeyWindow();
        _hotkeys.HotkeyPressed += OnHotkey;
        if (!_hotkeys.RegisterDefaults())
        {
            MessageBox.Show(
                "Could not register global hotkeys (Ctrl+Alt+Arrows). Another app may already use them.",
                "Funky Framework LEDs",
                MessageBoxButtons.OK,
                MessageBoxIcon.Warning);
        }

        _statusItem = new ToolStripMenuItem("Status: connecting…") { Enabled = false };
        _portItem = new ToolStripMenuItem("Port: —") { Enabled = false };
        _onItem = new ToolStripMenuItem("LEDs On\tCtrl+Alt+Right", null, (_, _) => _leds.SetPower(true));
        _offItem = new ToolStripMenuItem("LEDs Off\tCtrl+Alt+Left", null, (_, _) => _leds.SetPower(false));

        var brighter = new ToolStripMenuItem("Brightness Up\tCtrl+Alt+Up", null,
            (_, _) => _leds.AdjustBrightness(LedController.BrightnessStep));
        var dimmer = new ToolStripMenuItem("Brightness Down\tCtrl+Alt+Down", null,
            (_, _) => _leds.AdjustBrightness(-LedController.BrightnessStep));
        var reconnect = new ToolStripMenuItem("Reconnect", null, (_, _) => TryConnect(showBalloon: true));
        var exit = new ToolStripMenuItem("Exit", null, (_, _) => ExitThread());

        var menu = new ContextMenuStrip();
        menu.Items.Add(_statusItem);
        menu.Items.Add(_portItem);
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add(_onItem);
        menu.Items.Add(_offItem);
        menu.Items.Add(brighter);
        menu.Items.Add(dimmer);
        menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add(reconnect);
        menu.Items.Add(new ToolStripMenuItem("Hotkeys help", null, (_, _) => ShowHotkeyHelp()));
        menu.Items.Add(exit);

        _tray = new NotifyIcon
        {
            Icon = LoadAppIcon(),
            Text = "Funky Framework LEDs",
            Visible = true,
            ContextMenuStrip = menu,
        };
        _tray.DoubleClick += (_, _) => ShowHotkeyHelp();

        _leds.StateChanged += (_, _) => UpdateTrayText();

        _reconnectTimer = new System.Windows.Forms.Timer { Interval = 5000 };
        _reconnectTimer.Tick += (_, _) =>
        {
            if (!_leds.IsConnected)
            {
                TryConnect(showBalloon: false);
            }
        };
        _reconnectTimer.Start();

        TryConnect(showBalloon: true);
        UpdateTrayText();
    }

    private void OnHotkey(int id)
    {
        if (!_leds.IsConnected)
        {
            TryConnect(showBalloon: false);
            if (!_leds.IsConnected)
            {
                _tray.ShowBalloonTip(1500, "Funky Framework LEDs", "Not connected to a module.", ToolTipIcon.Warning);
                return;
            }
        }

        switch (id)
        {
            case HotkeyWindow.IdBrightnessUp:
                _leds.AdjustBrightness(LedController.BrightnessStep);
                break;
            case HotkeyWindow.IdBrightnessDown:
                _leds.AdjustBrightness(-LedController.BrightnessStep);
                break;
            case HotkeyWindow.IdLedsOn:
                _leds.SetPower(true);
                break;
            case HotkeyWindow.IdLedsOff:
                _leds.SetPower(false);
                break;
        }
    }

    private void TryConnect(bool showBalloon)
    {
        var ok = _leds.TryAutoConnect();
        UpdateTrayText();
        if (showBalloon)
        {
            if (ok)
            {
                var n = _leds.ConnectedCount;
                _tray.ShowBalloonTip(
                    2000,
                    "Funky Framework LEDs",
                    n == 1
                        ? $"Connected to {_leds.PortName}"
                        : $"Connected to {n} panels: {_leds.PortName}",
                    ToolTipIcon.Info);
            }
            else
            {
                _tray.ShowBalloonTip(
                    2500,
                    "Funky Framework LEDs",
                    "No modules found. Insert panel(s) and choose Reconnect.",
                    ToolTipIcon.Warning);
            }
        }
    }

    private void UpdateTrayText()
    {
        void apply()
        {
            if (_leds.IsConnected)
            {
                var power = _leds.PowerOn ? "ON" : "OFF";
                _statusItem.Text = $"Status: {power}, brightness {_leds.Brightness}";
                var n = _leds.ConnectedCount;
                _portItem.Text = n == 1
                    ? $"Port: {_leds.PortName}"
                    : $"Ports ({n}): {_leds.PortName}";
                _tray.Text = n == 1
                    ? $"Funky LEDs · {power} · {_leds.Brightness}"
                    : $"Funky LEDs · {n} panels · {power} · {_leds.Brightness}";
                _onItem.Enabled = true;
                _offItem.Enabled = true;
            }
            else
            {
                _statusItem.Text = "Status: disconnected";
                _portItem.Text = "Port: —";
                _tray.Text = "Funky Framework LEDs · disconnected";
                _onItem.Enabled = false;
                _offItem.Enabled = false;
            }
        }

        if (_tray.ContextMenuStrip?.InvokeRequired == true)
        {
            _tray.ContextMenuStrip.BeginInvoke(apply);
        }
        else
        {
            apply();
        }
    }

    private static void ShowHotkeyHelp()
    {
        MessageBox.Show(
            "Global hotkeys:\n\n" +
            "Ctrl+Alt+Up — brightness up\n" +
            "Ctrl+Alt+Down — brightness down\n" +
            "Ctrl+Alt+Right — LEDs on\n" +
            "Ctrl+Alt+Left — LEDs off\n\n" +
            "Right-click the tray icon for the same actions.",
            "Funky Framework LEDs",
            MessageBoxButtons.OK,
            MessageBoxIcon.Information);
    }

    private static Icon LoadAppIcon()
    {
        var path = Path.Combine(AppContext.BaseDirectory, "Assets", "app.ico");
        if (File.Exists(path))
        {
            return new Icon(path);
        }
        return SystemIcons.Application;
    }

    protected override void ExitThreadCore()
    {
        _reconnectTimer.Stop();
        _reconnectTimer.Dispose();
        _hotkeys.Dispose();
        _leds.Dispose();
        _tray.Visible = false;
        _tray.Dispose();
        base.ExitThreadCore();
    }
}
