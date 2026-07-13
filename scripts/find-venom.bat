@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build"
set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Release"

set "CANDIDATE=%ROOT%\%BUILD_DIR%\venom.exe"
if exist "%CANDIDATE%" goto found

set "CANDIDATE=%ROOT%\%BUILD_DIR%\%CONFIG%\venom.exe"
if exist "%CANDIDATE%" goto found

set "CANDIDATE=%ROOT%\%BUILD_DIR%\Debug\venom.exe"
if exist "%CANDIDATE%" goto found
set "CANDIDATE=%ROOT%\%BUILD_DIR%\Release\venom.exe"
if exist "%CANDIDATE%" goto found
set "CANDIDATE=%ROOT%\%BUILD_DIR%\RelWithDebInfo\venom.exe"
if exist "%CANDIDATE%" goto found
set "CANDIDATE=%ROOT%\%BUILD_DIR%\MinSizeRel\venom.exe"
if exist "%CANDIDATE%" goto found

echo [venom] error: venom.exe was not found under "%ROOT%\%BUILD_DIR%".
endlocal & exit /b 1

:found
endlocal & set "VENOM_EXE=%CANDIDATE%" & exit /b 0
