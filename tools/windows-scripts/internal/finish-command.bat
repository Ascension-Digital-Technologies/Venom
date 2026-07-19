@echo off
setlocal EnableExtensions

set "VENOM_EXIT=%~1"
if "%VENOM_EXIT%"=="" set "VENOM_EXIT=1"

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
