REM ESP-IDF root
set IDF_PATH=D:\01_EU\Tools\esp-idf

REM Load ESP-IDF environment
call "%IDF_PATH%\export.bat"

REM Go to the ESP-IDF project root explicitly
cd /d "%~dp0\01-autosar-os-conformance"

REM Safety check (optional but recommended)
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found in project directory
    exit /b 1
)

REM Build + flash + monitor
idf.py build flash monitor
cmd /k idf.py monitor