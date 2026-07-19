@echo off
setlocal
set "VENOM_ROOT=%~dp0..\.."
set "VENOM_BUILD=%VENOM_ROOT%\build-chrome-extension-compatibility"

echo [INFO] Configure Chrome extension compatibility tests...
cmake -S "%VENOM_ROOT%" -B "%VENOM_BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if errorlevel 1 goto :venom_error

echo [INFO] Build native compatibility matrix...
cmake --build "%VENOM_BUILD%" --target venom_chrome_extension_compatibility_matrix_test
if errorlevel 1 goto :venom_error

echo [INFO] Run native compatibility matrix...
"%VENOM_BUILD%\venom_chrome_extension_compatibility_matrix_test.exe"
if errorlevel 1 goto :venom_error

echo [INFO] Run real Chrome lifecycle matrix...
python "%VENOM_ROOT%\tests\browser\chrome-extension-compatibility.py"
if errorlevel 1 goto :venom_error

set "VENOM_EXIT=0"
goto :venom_finish

:venom_error
set "VENOM_EXIT=%ERRORLEVEL%"
if "%VENOM_EXIT%"=="0" set "VENOM_EXIT=1"

:venom_finish
call "%VENOM_ROOT%\tools\windows-scripts\internal\finish-command.bat" "%VENOM_EXIT%"
set "VENOM_EXIT=%ERRORLEVEL%"
endlocal & exit /b %VENOM_EXIT%
