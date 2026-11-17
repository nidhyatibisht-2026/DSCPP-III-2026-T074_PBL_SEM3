@echo off
setlocal
cd /d "%~dp0.."

set "MINGW_BIN=C:\msys64\ucrt64\bin"
if exist "%MINGW_BIN%" set "PATH=%MINGW_BIN%;%PATH%"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found. Install CMake and ensure it is on PATH.
    pause
    exit /b 1
)

if not exist "build-mingw" mkdir "build-mingw"
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="%MINGW_BIN%\g++.exe"
if errorlevel 1 goto build_error

cmake --build build-mingw --config Release
if errorlevel 1 goto build_error

py -3.13 -m pip install -r requirements.txt
if errorlevel 1 goto build_error

py -3.13 integration\app.py
exit /b 0

:build_error
echo.
echo [ERROR] FoodFlow could not be built or prepared. Review the message above.
pause
exit /b 1
