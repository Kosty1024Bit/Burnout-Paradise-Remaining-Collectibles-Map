param([int]$GameProcessId)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class SpeedometerGameWindow {
  public delegate bool EnumProc(IntPtr hwnd, IntPtr param);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc callback, IntPtr param);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
  [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hwnd, StringBuilder title, int length);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hwnd, int command);
}
'@
[SpeedometerGameWindow]::EnumWindows({param($hwnd,$unused)
    [uint32]$windowPid = 0
    [void][SpeedometerGameWindow]::GetWindowThreadProcessId($hwnd,[ref]$windowPid)
    if ($windowPid -eq $GameProcessId) {
        $title = [Text.StringBuilder]::new(512)
        [void][SpeedometerGameWindow]::GetWindowText($hwnd,$title,512)
        if ($title.ToString() -like '*Burnout*') {
            [void][SpeedometerGameWindow]::ShowWindow($hwnd,4)
            Write-Host "Shown game window $hwnd, $title (without keyboard/mouse input)"
        }
    }
    return $true
},[IntPtr]::Zero) | Out-Null
