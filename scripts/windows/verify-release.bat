@echo off
setlocal
set "VENOM_ROOT=%~dp0..\.."
set "VENOM_PS1=%VENOM_ROOT%\tools\windows-scripts\verify-release.ps1"
call "%VENOM_ROOT%\tools\windows-scripts\internal\invoke-powershell.bat" %*
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
