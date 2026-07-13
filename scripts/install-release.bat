@echo off
setlocal
set "ROOT=%~dp0.."
if defined PYTHON (set "PY=%PYTHON%") else (set "PY=python")
"%PY%" "%ROOT%\tools\install_release.py" %*
exit /b %ERRORLEVEL%
