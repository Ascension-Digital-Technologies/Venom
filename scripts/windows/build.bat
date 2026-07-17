@echo off
set "VENOM_ROOT=%~dp0..\.."
if exist "%~dp0..\tools\windows-scripts\build.ps1" set "VENOM_ROOT=%~dp0.."
set "VENOM_PS1=%VENOM_ROOT%\tools\windows-scripts\build.ps1"
call "%VENOM_ROOT%\tools\windows-scripts\internal\invoke-powershell.bat" %*
exit /b %ERRORLEVEL%
