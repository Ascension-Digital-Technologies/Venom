@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"
set "BUILD_DIR=%~2"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build"
set "FULL_QUICKJS=%~3"

call "%ROOT%\scripts\build.bat" "%CONFIG%" "%BUILD_DIR%" "%FULL_QUICKJS%"
if errorlevel 1 exit /b 1

echo [venom] test: %BUILD_DIR% ^(%CONFIG%^) 
ctest --test-dir "%ROOT%\%BUILD_DIR%" --build-config "%CONFIG%" --output-on-failure
if errorlevel 1 exit /b 1

endlocal
