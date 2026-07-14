@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0release-closure.ps1" %*
exit /b %ERRORLEVEL%
