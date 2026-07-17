@echo off
setlocal
python "%~dp0\..\..\tools\build_and_launch_example.py" 7 %*
set "VENOM_EXIT=%ERRORLEVEL%"
if not "%VENOM_EXIT%"=="0" pause
exit /b %VENOM_EXIT%
