@echo off
python "%~dp0..\tools\package_compatibility_evidence.py" %*
exit /b %ERRORLEVEL%
