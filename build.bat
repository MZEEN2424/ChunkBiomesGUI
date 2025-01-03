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
if exist bin\chunkbiomesgui.exe (
    echo GUI executable created successfully!
) else (
    echo Error: GUI executable not created!
)

cd ..
<<<<<<< HEAD
echo.
echo Press any key to exit...
pause >nul

=======
pause
>>>>>>> 92965f9 (.)
