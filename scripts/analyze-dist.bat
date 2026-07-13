@echo off
setlocal EnableExtensions
set "ROOT=%~dp0.."
python "%ROOT%\tools\analyze_dist.py" --repo-root "%ROOT%" %*
set "VENOM_EXIT=%ERRORLEVEL%"
call "%~dp0pause-on-error.bat" %VENOM_EXIT%
exit /b %VENOM_EXIT%
