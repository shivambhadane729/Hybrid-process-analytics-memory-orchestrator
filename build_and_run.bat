@echo off
setlocal

:: --- Configuration ---
set "QT_PATH=C:\Qt\6.11.0\mingw_64"
set "MINGW_PATH=C:\Qt\Tools\mingw1120_64"
set "EXE_NAME=analyzer_gui.exe"

:: Setup PATH
set "PATH=%QT_PATH%\bin;%MINGW_PATH%\bin;%PATH%"

echo ========================================
echo   Building Adaptive Process Analyzer...
echo ========================================

:: Check for qmake
where qmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] qmake not found. Please check QT_PATH in this script.
    pause
    exit /b
)

:: Run qmake if Makefile doesn't exist
if not exist "Makefile" (
    echo [INFO] Generating Makefile...
    qmake analyzer_gui.pro
)

:: Run make
echo [INFO] Compiling...
mingw32-make -j8
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b
)

echo [SUCCESS] Build complete. Starting...
if exist "release\%EXE_NAME%" (
    start "" "release\%EXE_NAME%"
) else if exist "debug\%EXE_NAME%" (
    start "" "debug\%EXE_NAME%"
) else (
    start "" "%EXE_NAME%"
)
