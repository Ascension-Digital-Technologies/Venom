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
echo.
if "%VENOM_EXIT%"=="0" (
  echo [SUCCESS] Command completed successfully.
) else (
  echo [ERROR]   Command failed with exit code %VENOM_EXIT%.
)

set "VENOM_PAUSE=1"
if /I "%VENOM_NO_PAUSE%"=="1" set "VENOM_PAUSE=0"
if /I "%CI%"=="true" set "VENOM_PAUSE=0"
if /I "%GITHUB_ACTIONS%"=="true" set "VENOM_PAUSE=0"
if /I "%TF_BUILD%"=="True" set "VENOM_PAUSE=0"
if /I "%APPVEYOR%"=="True" set "VENOM_PAUSE=0"

if "%VENOM_PAUSE%"=="1" (
  echo [INFO]    Press any key to close this window...
  pause >nul
)

endlocal & exit /b %VENOM_EXIT%
