@echo off
setlocal EnableExtensions
if "%VENOM_PS1%"=="" (
  echo [venom] error: VENOM_PS1 was not set by the launcher.
  exit /b 2
)
if not exist "%VENOM_PS1%" (
  echo [venom] error: script not found: %VENOM_PS1%
  set "VENOM_EXIT=2"
  goto finish
)
where powershell >nul 2>nul
if errorlevel 1 (
  echo [venom] error: PowerShell was not found in PATH.
  set "VENOM_EXIT=1"
  goto finish
)
powershell -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%VENOM_PS1%" %*
set "VENOM_EXIT=%ERRORLEVEL%"
:finish
if not "%VENOM_EXIT%"=="0" if /I not "%VENOM_NO_PAUSE%"=="1" (
  echo.
  echo [venom] command failed with exit code %VENOM_EXIT%.
  echo [venom] Press any key to close this window...
  pause >nul
)
exit /b %VENOM_EXIT%
