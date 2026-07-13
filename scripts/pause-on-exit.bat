@echo off
set "VENOM_PAUSE_EXIT=%~1"
if "%VENOM_PAUSE_EXIT%"=="" set "VENOM_PAUSE_EXIT=0"
if /I "%VENOM_NO_PAUSE%"=="1" exit /b %VENOM_PAUSE_EXIT%
if "%VENOM_ALREADY_PAUSED%"=="1" exit /b %VENOM_PAUSE_EXIT%

set "VENOM_SHOULD_PAUSE=0"
if /I "%VENOM_PAUSE_MODE%"=="always" set "VENOM_SHOULD_PAUSE=1"
if not "%VENOM_PAUSE_EXIT%"=="0" if /I "%VENOM_PAUSE_ON_ERROR%"=="1" set "VENOM_SHOULD_PAUSE=1"

if "%VENOM_SHOULD_PAUSE%"=="1" (
  set "VENOM_ALREADY_PAUSED=1"
  echo.
  if "%VENOM_PAUSE_EXIT%"=="0" (
    echo [venom] command completed successfully.
  ) else (
    echo [venom] command failed with exit code %VENOM_PAUSE_EXIT%.
    echo [venom] Review the error above before closing this window.
  )
  echo [venom] Press any key to close this window...
  pause >nul
)
exit /b %VENOM_PAUSE_EXIT%
