@echo off
setlocal
rem Prepare Rage of the Dragons ROM files for the Windows build.
rem Usage: prepare-roms.bat rotd.zip neogeo.zip [OUT_DIR]
rem Default OUT_DIR: .\roms next to this script.

if "%~1"=="" goto usage
if "%~2"=="" goto usage

set "OUT=%~3"
if "%OUT%"=="" set "OUT=%~dp0roms"

if not exist "%~dp0romprep.exe" (
    echo prepare-roms: romprep.exe is missing next to this script.
    pause
    exit /b 1
)

"%~dp0romprep.exe" --game-zip "%~1" --bios-zip "%~2" --out "%OUT%"
if errorlevel 1 (
    echo.
    echo ROM preparation failed. Check that the zips are the MAME rotd and neogeo sets.
    pause
    exit /b 1
)

echo.
echo Prepared ROM files are in:
echo   %OUT%
echo.
echo Run:
echo   rotd.exe --rom-path "%OUT%"
echo.
echo Controls: arrows, Z/X/C/V = A/B/C/D, 5 = coin, 1 = start, Esc = quit.
echo Gamepad: D-pad/stick, A/B/X/Y, Start, Back = coin.
exit /b 0

:usage
echo usage: prepare-roms.bat rotd.zip neogeo.zip [OUT_DIR]
echo.
echo Supply your own legally obtained MAME sets. No ROMs are included.
pause
exit /b 2
