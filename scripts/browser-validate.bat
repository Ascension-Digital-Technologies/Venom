@echo off
setlocal
python "%~dp0..\tools\browser_validation.py" %*
exit /b %errorlevel%
