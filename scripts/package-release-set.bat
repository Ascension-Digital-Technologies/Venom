@echo off
python "%~dp0..\tools\package_release_set.py" %*
exit /b %ERRORLEVEL%
