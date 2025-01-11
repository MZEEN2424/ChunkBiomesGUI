@echo off
echo Cleaning build directory...
if exist build rmdir /s /q build

echo Creating build directory...
mkdir build
cd build

echo Configuring with CMake...
cmake -G "MinGW Makefiles" ..

echo Building project...
mingw32-make

echo Build complete!
if exist chunkbiomesgui.exe (
    powershell -Command "& {Add-Type -AssemblyName System.Windows.Forms; Add-Type -AssemblyName System.Drawing; $notify = New-Object System.Windows.Forms.NotifyIcon; $notify.Icon = [System.Drawing.SystemIcons]::Information; $notify.Visible = $true; $notify.ShowBalloonTip(0, 'Build Complete', 'GUI executable created successfully!', [System.Windows.Forms.ToolTipIcon]::None)}"
) else (
    powershell -Command "& {Add-Type -AssemblyName System.Windows.Forms; Add-Type -AssemblyName System.Drawing; $notify = New-Object System.Windows.Forms.NotifyIcon; $notify.Icon = [System.Drawing.SystemIcons]::Error; $notify.Visible = $true; $notify.ShowBalloonTip(0, 'Build Failed', 'Error: GUI executable not created!', [System.Windows.Forms.ToolTipIcon]::Error)}"
)

cd ..
echo.
echo Press any key to exit...
pause >nul
