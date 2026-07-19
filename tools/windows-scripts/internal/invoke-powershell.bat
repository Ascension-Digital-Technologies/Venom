@echo off
setlocal EnableExtensions

if "%VENOM_PS1%"=="" (
  echo [ERROR]   VENOM_PS1 was not set by the launcher.
  set "VENOM_EXIT=2"
  goto finish
)
if not exist "%VENOM_PS1%" (
  echo [ERROR]   Script not found: %VENOM_PS1%
  set "VENOM_EXIT=2"
  goto finish
)
where powershell.exe >nul 2>nul
if errorlevel 1 (
  echo [ERROR]   PowerShell was not found in PATH.
  set "VENOM_EXIT=1"
  goto finish
)

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%VENOM_PS1%" %*
set "VENOM_EXIT=%ERRORLEVEL%"

:finish
call "%~dp0finish-command.bat" "%VENOM_EXIT%"
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
