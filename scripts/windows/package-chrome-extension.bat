@echo off
setlocal
set "VENOM_ROOT=%~dp0..\.."
set "VENOM_DIST=%~1"
if "%VENOM_DIST%"=="" set "VENOM_DIST=%VENOM_ROOT%\examples\chrome-extension\dist-venom"
set "VENOM_OUT=%~2"
if "%VENOM_OUT%"=="" set "VENOM_OUT=%VENOM_ROOT%\release\chrome-extension"
python "%VENOM_ROOT%\tools\package_chrome_extension.py" "%VENOM_DIST%" --out-dir "%VENOM_OUT%"
set "VENOM_EXIT=%ERRORLEVEL%"
call "%VENOM_ROOT%\tools\windows-scripts\internal\finish-command.bat" "%VENOM_EXIT%"
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
