using System;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;

namespace FunkyLedsTray;

/// <summary>
/// Talks to funky_leds firmware over USB CDC (115200). Control only — no frame streaming.
/// </summary>
internal sealed class LedController : IDisposable
{
    private static readonly Regex StatusRegex = new(
        @"STATUS\s+power=(on|off)\s+brightness=(\d+)",
        RegexOptions.IgnoreCase | RegexOptions.Compiled);

    public const int Baud = 115200;
    public const int BrightnessStep = 8;
    public const int MinBrightness = 0;
    public const int MaxBrightness = 255;

    private readonly object _gate = new();
    private SerialPort? _port;
    private readonly StringBuilder _rx = new();

    public string? PortName { get; private set; }
    public bool IsConnected
    {
        get { lock (_gate) return _port is { IsOpen: true }; }
    }

    public bool PowerOn { get; private set; } = true;
    public int Brightness { get; private set; } = 40;

    public event EventHandler? StateChanged;

    public static string[] ListCandidatePorts()
    {
        return SerialPort.GetPortNames().OrderBy(n => n, StringComparer.OrdinalIgnoreCase).ToArray();
    }

    public bool TryAutoConnect()
    {
        foreach (var name in ListCandidatePorts())
        {
            if (TryConnect(name))
            {
                return true;
            }
        }
        return false;
    }

    public bool TryConnect(string portName)
    {
        Disconnect();
        try
        {
            var port = new SerialPort(portName, Baud)
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

            lock (_gate)
            {
                _port = port;
                PortName = portName;
                _rx.Clear();
            }

            Send("STATUS");
            DrainFor(TimeSpan.FromMilliseconds(800));
            RaiseStateChanged();
            return true;
        }
        catch
        {
            Disconnect();
            return false;
        }
    }

    public void Disconnect()
    {
        lock (_gate)
        {
            if (_port != null)
            {
                try
                {
                    if (_port.IsOpen)
                    {
                        _port.Close();
                    }
                }
                catch
                {
                    // ignore
                }
                _port.Dispose();
                _port = null;
            }
            PortName = null;
            _rx.Clear();
        }
        RaiseStateChanged();
    }

    public void SetPower(bool on)
    {
        if (!IsConnected)
        {
            return;
        }
        Send(on ? "ON" : "OFF");
        DrainFor(TimeSpan.FromMilliseconds(500));
        if (PowerOn != on)
        {
            // Firmware should echo STATUS; apply optimistically if not.
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
        Send($"BRIGHTNESS {value}");
        DrainFor(TimeSpan.FromMilliseconds(500));
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
        Send("STATUS");
        DrainFor(TimeSpan.FromMilliseconds(500));
    }

    private void Send(string line)
    {
        lock (_gate)
        {
            if (_port is not { IsOpen: true })
            {
                return;
            }
            _port.Write(line.Trim() + "\n");
        }
    }

    private void DrainFor(TimeSpan budget)
    {
        var deadline = DateTime.UtcNow + budget;
        while (DateTime.UtcNow < deadline)
        {
            string chunk;
            lock (_gate)
            {
                if (_port is not { IsOpen: true })
                {
                    return;
                }
                try
                {
                    chunk = _port.BytesToRead > 0 ? _port.ReadExisting() : string.Empty;
                }
                catch
                {
                    return;
                }
            }

            if (chunk.Length == 0)
            {
                Thread.Sleep(20);
                continue;
            }

            _rx.Append(chunk);
            ProcessBuffer();
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
            PowerOn = m.Groups[1].Value.Equals("on", StringComparison.OrdinalIgnoreCase);
            Brightness = int.Parse(m.Groups[2].Value);
            RaiseStateChanged();
        }
    }

    private void RaiseStateChanged() => StateChanged?.Invoke(this, EventArgs.Empty);

    public void Dispose() => Disconnect();
}
