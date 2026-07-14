@echo off
call "%~dp0internal\invoke-powershell.bat" "%~dp0compatibility.ps1" %*
exit /b %ERRORLEVEL%
