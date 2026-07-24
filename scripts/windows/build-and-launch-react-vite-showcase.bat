@echo off
setlocal
set "VENOM_ROOT=%~dp0..\.."
set "VENOM_EXAMPLE=react-vite-showcase"
call "%VENOM_ROOT%\tools\windows-scripts\internal\invoke-example.bat" %*
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
