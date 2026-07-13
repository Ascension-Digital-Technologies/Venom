@echo off
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "PORT=%~1"
set "DIST=%~2"
if "%PORT%"=="" set "PORT=8080"
if "%DIST%"=="" set "DIST=dist"
set "DIST_PATH=%DIST%"
if not "%DIST:~1,1%"==":" if not "%DIST:~0,1%"=="\" set "DIST_PATH=%ROOT%\%DIST%"
if not exist "%DIST_PATH%\index.html" (
  call "%ROOT%\scripts\build-site.bat" "%ROOT%\examples\basic-site" "%DIST_PATH%"
  if errorlevel 1 exit /b 1
)
echo [venom] serving %DIST_PATH% at http://127.0.0.1:%PORT%
python -m http.server %PORT% --directory "%DIST_PATH%"
