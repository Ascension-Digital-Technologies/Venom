@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0fuzz-package.ps1"
exit /b %errorlevel%
