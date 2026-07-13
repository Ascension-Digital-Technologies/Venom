@echo off
python "%~dp0..\tools\browser_performance.py" %*
exit /b %ERRORLEVEL%
