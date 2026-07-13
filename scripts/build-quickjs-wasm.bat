@echo off
setlocal EnableExtensions
if /I not "%VENOM_NO_PAUSE%"=="1" if "%VENOM_PAUSE_MODE%"=="" set "VENOM_PAUSE_MODE=always"
set "SCRIPT_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-quickjs-wasm.ps1" %*
set "VENOM_EXIT=%ERRORLEVEL%"
call "%SCRIPT_DIR%pause-on-exit.bat" %VENOM_EXIT%
exit /b %VENOM_EXIT%
