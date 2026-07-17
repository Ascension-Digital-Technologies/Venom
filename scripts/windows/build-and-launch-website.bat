@echo off
setlocal
call "%~dp0..\..\venom-api-server\scripts\windows\build-and-launch.bat" %*
exit /b %ERRORLEVEL%
