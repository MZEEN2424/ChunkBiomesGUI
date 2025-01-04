@echo off
title ChunkBiomes GUI Build Script

echo ==================================
echo    ChunkBiomes GUI Build Script
echo ==================================
echo.

@REM Name of final executable
@REM From dbenham on Stack Overflow: https://stackoverflow.com/a/10552926
set "executableName=chunkbiomesgui.exe"
@REM Path to directory to use for building
set "buildPath=.\build"
@REM Path to final executable
@REM From podosta on Stack Overflow: https://stackoverflow.com/a/856592
set "executablePath=%buildPath%\bin\%executableName%"

@REM Check if CMake is available
where cmake > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found! Please install CMake and add it to your PATH.
    pause
    exit /b 1
)

@REM Check if MinGW-make is available
@REM TODO: GENERALIZE
where mingw32-make > nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: MinGW32-Make not found! Please install MinGW and add it to your PATH.
    pause
    exit /b 1
)

@REM Check if executable is running
@REM From Jason on Stack Overflow: https://stackoverflow.com/a/1329790
tasklist | find /i "%executableName%" > nul
@REM If executable was found, kill it
if %ERRORLEVEL% == 0 (
    echo Killing running executable...
    @REM From fmark on Stack Overflow: https://stackoverflow.com/a/2888874
    taskkill /f /im "%executableName%" > nul
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to kill running executable!
        pause
        exit /b 1
    )
    @REM Pause to allow any locks on the executable file to open
    @REM From STLDev on Stack Overflow: https://stackoverflow.com/a/47522011
    timeout /t 3 /nobreak > nul
@REM If command completed successfully but executable was not found, run dummy command to reset %ERRORLEVEL% to 0
) else if %ERRORLEVEL% == 1 (
    @REM From Ross Smith II on Stack Overflow: https://stackoverflow.com/a/2035363
    cmd /c "exit 0"
)

@REM If build directory already exists, delete it
echo Resetting build directory...
if exist "%buildPath%" (
    rmdir /s /q "%buildPath%"
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to clean build directory!
        pause
        exit /b 1
    )
)
@REM Then create and enter fresh build directory
mkdir "%buildPath%"
cd "%buildPath%"

@REM Run CMake
echo Configuring with CMake...
@REM TODO: GENERALIZE
cmake.exe -G "MinGW Makefiles" ..
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

@REM Run MinGW-make and exit build directory
echo Building project...
@REM TODO: GENERALIZE
mingw32-make.exe
if %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed!
    cd ..
    pause
    exit /b 1
)
cd ..

echo.
echo Build completed successfully!
echo.

@REM If executable exists, start it automatically
if exist "%executablePath%" (
    @REM move %executablePath% .\%executableName%
    echo [SUCCESS] GUI executable created successfully!
    echo   Location: %executablePath%
    @REM From rojo on Stack Overflow: https://stackoverflow.com/a/34725146
    start "" "%executablePath%"
) else (
    echo [ERROR] GUI executable not created!
)