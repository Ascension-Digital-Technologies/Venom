@echo off
setlocal
set ROOT=%~dp0..
python "%ROOT%\tools\production_readiness.py" %*
exit /b %ERRORLEVEL%
