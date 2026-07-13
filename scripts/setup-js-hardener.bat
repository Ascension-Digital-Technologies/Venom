@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%setup-js-hardener.ps1" %*
set "RC=%ERRORLEVEL%"
endlocal & exit /b %RC%
