@echo off
setlocal

:: --- Configuration ---
set "QT_PATH=C:\Qt\6.11.0\mingw_64"
set "EXE_NAME=analyzer_gui.exe"

:: Add Qt bin to PATH for DLLs
set "PATH=%QT_PATH%\bin;%PATH%"

echo ========================================
echo   Adaptive Process Analyzer - Runner
echo ========================================

:: Check for Release build first
if exist "release\%EXE_NAME%" (
    echo [INFO] Found Release build. Starting...
    start "" "release\%EXE_NAME%"
    exit /b
)

:: Check for Debug build
if exist "debug\%EXE_NAME%" (
    echo [INFO] Found Debug build. Starting...
    start "" "debug\%EXE_NAME%"
    exit /b
)

:: Check for local build
if exist "%EXE_NAME%" (
    echo [INFO] Found local build. Starting...
    start "" "%EXE_NAME%"
    exit /b
)

echo [ERROR] Could not find %EXE_NAME%.
echo Please build the project in Qt Creator or use mingw32-make.
echo.
pause
