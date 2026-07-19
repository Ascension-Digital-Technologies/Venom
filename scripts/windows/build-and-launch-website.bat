@echo off
setlocal
set "VENOM_ROOT=%~dp0..\.."
call "%VENOM_ROOT%\venom-api-server\scripts\windows\build-and-launch.bat" %*
set "VENOM_EXIT=%ERRORLEVEL%"
call "%VENOM_ROOT%\tools\windows-scripts\internal\finish-command.bat" "%VENOM_EXIT%"
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
