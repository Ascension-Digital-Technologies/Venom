@echo off
python "%~dp0..\tools\verify_compatibility_evidence.py" %*
exit /b %ERRORLEVEL%
