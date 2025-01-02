@echo off
title ChunkBiomes GUI Build Script

echo ==================================
echo    ChunkBiomes GUI Build Script
echo ==================================
echo.

:: Check if required tools are available
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found! Please install CMake and add it to your PATH.
    pause
    exit /b 1
)

:: TODO: GENERALIZE
where mingw32-make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: MinGW32-Make not found! Please install MinGW and add it to your PATH.
    pause
    exit /b 1
)

echo Cleaning build directory...
:: if qprocess "chunkbiomesgui.exe" >nul (
::      taskkill /f /im chunkbiomesgui.exe
::      if %ERRORLEVEL% NEQ 0 (
::          echo Error: Failed to kill running executable!
::          pause
::          exit /b 1
::      )
::  )
if exist build (
    rmdir /s /q build
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to clean build directory!
        pause
        exit /b 1
    )
)

echo Creating build directory...
mkdir build
cd build

echo Configuring with CMake...
:: TODO: GENERALIZE
cmake.exe -G "MinGW Makefiles" ..
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed!
    cd ..
    pause
    exit /b 1
)

echo Building project...
:: TODO: GENERALIZE
mingw32-make.exe
if %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo.

if exist bin\chunkbiomesgui.exe (
    :: move bin\chunkbiomesgui.exe ..\chunkbiomesgui.exe
    echo [SUCCESS] GUI executable created successfully!
    echo   Location: build\bin\chunkbiomesgui.exe
    start "" bin\chunkbiomesgui.exe
) else (
    echo [ERROR] GUI executable not created!
)

cd ..
:: echo.
:: echo Press any key to exit...
:: pause >nul