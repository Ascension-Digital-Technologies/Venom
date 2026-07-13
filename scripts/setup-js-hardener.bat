@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup-js-hardener.ps1" %*
exit /b %errorlevel%
