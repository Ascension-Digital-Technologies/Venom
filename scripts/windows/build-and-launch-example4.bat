@echo off
set "VENOM_ROOT=%~dp0..\.."
if exist "%~dp0..\tools\build_and_launch_example.py" set "VENOM_ROOT=%~dp0.."
python "%VENOM_ROOT%\tools\build_and_launch_example.py" 4 %*
exit /b %ERRORLEVEL%
