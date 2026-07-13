@echo off
if "%VENOM_PAUSE_MODE%"=="" set "VENOM_PAUSE_ON_ERROR=1"
call "%~dp0pause-on-exit.bat" %1
exit /b %ERRORLEVEL%
