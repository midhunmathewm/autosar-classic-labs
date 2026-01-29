@echo off

REM === ESP-IDF root (FIXED) ===
set IDF_PATH=D:\01_EU\Tools\Espressif\frameworks\esp-idf-v5.5.2

REM === Load ESP-IDF environment ===
call "%IDF_PATH%\export.bat"
if errorlevel 1 exit /b 1

REM === Go to project root ===
cd /d "%~dp0\01-autosar-os-conformance"

REM === Safety check ===
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in project directory
    exit /b 1
)

REM === Build + flash + monitor ===
idf.py build 

if errorlevel 1 exit /b 1

set ESPPORT=COM3
set ESPBAUD=115200

idf.py flash 
idf.py monitor
cmd /k idf.py monitor