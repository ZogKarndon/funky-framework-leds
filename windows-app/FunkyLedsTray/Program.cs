using System;
using System.Threading;
using System.Windows.Forms;

namespace FunkyLedsTray;

internal static class Program
{
    private const string MutexName = "Global\\FunkyFrameworkLedsTray";

    [STAThread]
    private static void Main()
    {
        using var mutex = new Mutex(true, MutexName, out bool createdNew);
        if (!createdNew)
        {
            MessageBox.Show(
                "Funky Framework LEDs is already running in the system tray.",
                "Funky Framework LEDs",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            return;
        }

        ApplicationConfiguration.Initialize();
        Application.Run(new TrayApplicationContext());
    }
}
