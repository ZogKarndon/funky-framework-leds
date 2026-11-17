using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

namespace FunkyLedsTray;

/// <summary>
/// Talks to funky_leds firmware over USB CDC (115200). Control only — no frame streaming.
/// Discovers every matching COM port (dual Framework panels) and fans out commands to all.
/// </summary>
internal sealed class LedController : IDisposable
{
    private static readonly Regex StatusRegex = new(
        @"STATUS\s+power=(on|off)\s+brightness=(\d+)",
        RegexOptions.IgnoreCase | RegexOptions.Compiled);

    public const int Baud = 115200;
    public const int BrightnessStep = 1;
    public const int MinBrightness = 0;
    public const int MaxBrightness = 48;
    public const int DefaultBrightness = 16;

    private readonly object _gate = new();
    private readonly List<OpenPort> _ports = new();
    private readonly StringBuilder _rx = new();

    public string? PortName
    {
        get
        {
            lock (_gate)
            {
                return _ports.Count == 0
                    ? null
                    : string.Join(", ", _ports.Select(p => p.Name));
            }
        }
    }

    public string[] ConnectedPorts
    {
        get
        {
            lock (_gate)
            {
                return _ports.Select(p => p.Name).ToArray();
            }
        }
    }

    public int ConnectedCount
    {
        get { lock (_gate) return _ports.Count; }
    }

    public bool IsConnected => ConnectedCount > 0;

    public bool PowerOn { get; private set; } = true;
    public int Brightness { get; private set; } = DefaultBrightness;

    public event EventHandler? StateChanged;

    public static string[] ListCandidatePorts()
    {
        return SerialPort.GetPortNames().OrderBy(n => n, StringComparer.OrdinalIgnoreCase).ToArray();
    }

    /// <summary>
    /// Scan all COM ports, keep every port that answers as funky_leds (STATUS or boot banner),
    /// and drop anything else. Replaces any previous connections.
    /// </summary>
    public bool TryAutoConnect()
    {
        Disconnect(raise: false);

        var found = new List<OpenPort>();
        foreach (var name in ListCandidatePorts())
        {
            if (TryOpenIdentified(name, out var open) && open != null)
            {
                found.Add(open);
            }
        }

        lock (_gate)
        {
            _ports.Clear();
            _ports.AddRange(found);
            _rx.Clear();
        }

        if (found.Count == 0)
        {
            RaiseStateChanged();
            return false;
        }

        // Align every panel to the shared host brightness/power (default 16 / on).
        FanOut(PowerOn ? "ON" : "OFF");
        FanOut($"BRIGHTNESS {Brightness}");
        FanOut("STATUS");
        DrainAllFor(TimeSpan.FromMilliseconds(800));
        RaiseStateChanged();
        return true;
    }

    public void Disconnect() => Disconnect(raise: true);

    private void Disconnect(bool raise)
    {
        lock (_gate)
        {
            foreach (var open in _ports)
            {
                SafeClose(open.Port);
            }
            _ports.Clear();
            _rx.Clear();
        }
        if (raise)
        {
            RaiseStateChanged();
        }
    }

    public void SetPower(bool on)
    {
        if (!IsConnected)
        {
            return;
        }
        FanOut(on ? "ON" : "OFF");
        DrainAllFor(TimeSpan.FromMilliseconds(500));
        if (PowerOn != on)
        {
            PowerOn = on;
            RaiseStateChanged();
        }
    }

    public void AdjustBrightness(int delta)
    {
        if (!IsConnected)
        {
            return;
        }
        var next = Math.Clamp(Brightness + delta, MinBrightness, MaxBrightness);
        SetBrightness(next);
    }

    public void SetBrightness(int value)
    {
        if (!IsConnected)
        {
            return;
        }
        value = Math.Clamp(value, MinBrightness, MaxBrightness);
        FanOut($"BRIGHTNESS {value}");
        DrainAllFor(TimeSpan.FromMilliseconds(500));
        if (Brightness != value)
        {
            Brightness = value;
            RaiseStateChanged();
        }
    }

    public void RefreshStatus()
    {
        if (!IsConnected)
        {
            return;
        }
        FanOut("STATUS");
        DrainAllFor(TimeSpan.FromMilliseconds(500));
    }

    /// <summary>
    /// Open one COM port and confirm it speaks funky_leds (STATUS line or boot banner).
    /// Non-matching devices are closed immediately so dual-panel scan is safe on mixed COM lists.
    /// </summary>
    private static bool TryOpenIdentified(string portName, out OpenPort? open)
    {
        open = null;
        SerialPort? port = null;
        try
        {
            port = new SerialPort(portName, Baud)
            {
                NewLine = "\n",
                Encoding = Encoding.ASCII,
                ReadTimeout = 400,
                WriteTimeout = 400,
                DtrEnable = true,
                RtsEnable = true,
            };
            port.Open();
            Thread.Sleep(250);
            port.DiscardInBuffer();
            port.Write("STATUS\n");

            var buf = new StringBuilder();
            var deadline = DateTime.UtcNow + TimeSpan.FromMilliseconds(800);
            var identified = false;
            while (DateTime.UtcNow < deadline)
            {
                try
                {
                    if (port.BytesToRead > 0)
                    {
                        buf.Append(port.ReadExisting());
                    }
                }
                catch
                {
                    break;
                }

                var text = buf.ToString();
                if (StatusRegex.IsMatch(text)
                    || text.IndexOf("FUNKY FW16 RGB", StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    identified = true;
                    break;
                }
                Thread.Sleep(20);
            }

            if (!identified)
            {
                SafeClose(port);
                return false;
            }

            open = new OpenPort(portName, port);
            return true;
        }
        catch
        {
            if (port != null)
            {
                SafeClose(port);
            }
            return false;
        }
    }

    private void FanOut(string line)
    {
        var payload = line.Trim() + "\n";
        lock (_gate)
        {
            for (var i = _ports.Count - 1; i >= 0; i--)
            {
                var open = _ports[i];
                try
                {
                    if (!open.Port.IsOpen)
                    {
                        SafeClose(open.Port);
                        _ports.RemoveAt(i);
                        continue;
                    }
                    open.Port.Write(payload);
                }
                catch
                {
                    SafeClose(open.Port);
                    _ports.RemoveAt(i);
                }
            }
        }
    }

    private void DrainAllFor(TimeSpan budget)
    {
        var deadline = DateTime.UtcNow + budget;
        while (DateTime.UtcNow < deadline)
        {
            var anyData = false;
            lock (_gate)
            {
                for (var i = _ports.Count - 1; i >= 0; i--)
                {
                    var open = _ports[i];
                    try
                    {
                        if (!open.Port.IsOpen)
                        {
                            SafeClose(open.Port);
                            _ports.RemoveAt(i);
                            continue;
                        }
                        if (open.Port.BytesToRead <= 0)
                        {
                            continue;
                        }
                        _rx.Append(open.Port.ReadExisting());
                        anyData = true;
                    }
                    catch
                    {
                        SafeClose(open.Port);
                        _ports.RemoveAt(i);
                    }
                }
            }

            if (anyData)
            {
                ProcessBuffer();
            }
            else
            {
                Thread.Sleep(20);
            }
        }
    }

    private void ProcessBuffer()
    {
        while (true)
        {
            var text = _rx.ToString();
            var nl = text.IndexOf('\n');
            if (nl < 0)
            {
                break;
            }
            var line = text[..nl].Trim('\r', ' ', '\t');
            _rx.Clear();
            if (nl + 1 < text.Length)
            {
                _rx.Append(text[(nl + 1)..]);
            }
            if (line.Length == 0)
            {
                continue;
            }
            var m = StatusRegex.Match(line);
            if (!m.Success)
            {
                continue;
            }
            // Shared UI state: last STATUS wins (panels should match after fan-out).
            PowerOn = m.Groups[1].Value.Equals("on", StringComparison.OrdinalIgnoreCase);
            Brightness = Math.Clamp(int.Parse(m.Groups[2].Value), MinBrightness, MaxBrightness);
            RaiseStateChanged();
        }
    }

    private static void SafeClose(SerialPort port)
    {
        try
        {
            if (port.IsOpen)
            {
                port.Close();
            }
        }
        catch
        {
            // ignore
        }
        try
        {
            port.Dispose();
        }
        catch
        {
            // ignore
        }
    }

    private void RaiseStateChanged() => StateChanged?.Invoke(this, EventArgs.Empty);

    public void Dispose() => Disconnect();

    private sealed class OpenPort
    {
        public OpenPort(string name, SerialPort port)
        {
            Name = name;
            Port = port;
        }

        public string Name { get; }
        public SerialPort Port { get; }
    }
}
