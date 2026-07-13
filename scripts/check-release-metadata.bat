@echo off
setlocal EnableExtensions
if "%~1"=="" (
  echo Usage: scripts\check-release-metadata.bat path\to\app.vbc [forbidden-name ...]
  exit /b 2
)
set "ROOT=%~dp0.."
set "ART=%~1"
shift
set "ARGS="
:loop
if "%~1"=="" goto run
set "ARGS=%ARGS% --forbid "%~1""
shift
goto loop
:run
python "%ROOT%\tools\check_release_metadata.py" "%ART%" %ARGS%
exit /b %ERRORLEVEL%
