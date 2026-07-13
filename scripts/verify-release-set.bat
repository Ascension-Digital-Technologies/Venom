@echo off
python "%~dp0..\tools\verify_release_set.py" %*
exit /b %ERRORLEVEL%
