@echo off
set "VENOM_PS1=%~dp0package-release.ps1"
call "%~dp0internal\invoke-powershell.bat" %*
exit /b %ERRORLEVEL%
