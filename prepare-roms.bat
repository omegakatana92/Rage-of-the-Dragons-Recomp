@echo off
call "%~dp0packaging\windows\prepare-roms.bat" %*
exit /b %ERRORLEVEL%
