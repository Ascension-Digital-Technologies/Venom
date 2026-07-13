@echo off
setlocal EnableExtensions
if /I not "%VENOM_NO_PAUSE%"=="1" if "%VENOM_PAUSE_MODE%"=="" set "VENOM_PAUSE_MODE=always"
set "ROOT=%~dp0.."
set "VENOM_PYTHON="
where py >nul 2>nul
if not errorlevel 1 set "VENOM_PYTHON=py -3"
if "%VENOM_PYTHON%"=="" (
  where python >nul 2>nul
  if not errorlevel 1 set "VENOM_PYTHON=python"
)
if "%VENOM_PYTHON%"=="" (
  echo [venom] error: Python 3 was not found in PATH.
  set "VENOM_EXIT=1"
  call "%~dp0pause-on-exit.bat" %VENOM_EXIT%
  exit /b %VENOM_EXIT%
)
%VENOM_PYTHON% "%ROOT%\tools\setup_emscripten.py" --repo-root "%ROOT%" %*
set "VENOM_EXIT=%ERRORLEVEL%"
call "%~dp0pause-on-exit.bat" %VENOM_EXIT%
exit /b %VENOM_EXIT%
