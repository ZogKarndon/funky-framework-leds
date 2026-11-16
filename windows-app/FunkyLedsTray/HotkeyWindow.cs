using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace FunkyLedsTray;

/// <summary>
/// Message-only window that owns Win32 RegisterHotKey bindings.
/// </summary>
internal sealed class HotkeyWindow : NativeWindow, IDisposable
{
    private const int WmHotkey = 0x0312;
    private const uint ModAlt = 0x0001;
    private const uint ModControl = 0x0002;
    private const uint ModNorepeat = 0x4000;

    public const int IdBrightnessUp = 1;
    public const int IdBrightnessDown = 2;
    public const int IdLedsOn = 3;
    public const int IdLedsOff = 4;

    private const int VkLeft = 0x25;
    private const int VkUp = 0x26;
    private const int VkRight = 0x27;
    private const int VkDown = 0x28;

    public event Action<int>? HotkeyPressed;

    public HotkeyWindow()
    {
        CreateHandle(new CreateParams());
    }

    public bool RegisterDefaults()
    {
        var mods = ModControl | ModAlt | ModNorepeat;
        return RegisterHotKey(Handle, IdBrightnessUp, mods, VkUp)
            && RegisterHotKey(Handle, IdBrightnessDown, mods, VkDown)
            && RegisterHotKey(Handle, IdLedsOn, mods, VkRight)
            && RegisterHotKey(Handle, IdLedsOff, mods, VkLeft);
    }

    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WmHotkey)
        {
            HotkeyPressed?.Invoke(m.WParam.ToInt32());
        }
        base.WndProc(ref m);
    }

    public void Dispose()
    {
        if (Handle != IntPtr.Zero)
        {
            UnregisterHotKey(Handle, IdBrightnessUp);
            UnregisterHotKey(Handle, IdBrightnessDown);
            UnregisterHotKey(Handle, IdLedsOn);
            UnregisterHotKey(Handle, IdLedsOff);
            DestroyHandle();
        }
    }

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, int vk);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool UnregisterHotKey(IntPtr hWnd, int id);
}
