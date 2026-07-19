@echo off
setlocal EnableExtensions

if "%VENOM_ROOT%"=="" (
  echo [ERROR]   VENOM_ROOT was not set by the launcher.
  set "VENOM_EXIT=2"
  goto finish
)
if "%VENOM_EXAMPLE%"=="" (
  echo [ERROR]   VENOM_EXAMPLE was not set by the launcher.
  set "VENOM_EXIT=2"
  goto finish
)
set "VENOM_LAUNCHER=%VENOM_ROOT%\tools\launch_example.py"
if not exist "%VENOM_LAUNCHER%" (
  echo [ERROR]   Example launcher not found: %VENOM_LAUNCHER%
  set "VENOM_EXIT=2"
  goto finish
)
where python.exe >nul 2>nul
if errorlevel 1 (
  echo [ERROR]   Python was not found in PATH.
  set "VENOM_EXIT=1"
  goto finish
)

python "%VENOM_LAUNCHER%" "%VENOM_EXAMPLE%" %*
set "VENOM_EXIT=%ERRORLEVEL%"

:finish
call "%~dp0finish-command.bat" "%VENOM_EXIT%"
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
