@echo off
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "SITE=%~1"
set "DIST=%~2"
set "CONFIG=%~3"
set "BUILD_DIR=build\windows-release"
if "%SITE%"=="" set "SITE=%ROOT%\examples\basic-site"
if "%DIST%"=="" set "DIST=dist"
if "%CONFIG%"=="" set "CONFIG=Release"
set "DIST_PATH=%DIST%"
if not "%DIST:~1,1%"==":" if not "%DIST:~0,1%"=="\" set "DIST_PATH=%ROOT%\%DIST%"
call "%ROOT%\scripts\build.bat" "%CONFIG%" "%BUILD_DIR%" fullquickjs
if errorlevel 1 exit /b 1
call "%ROOT%\scripts\find-venom.bat" "%BUILD_DIR%" "%CONFIG%"
if errorlevel 1 exit /b 1
echo [venom] production build %SITE% =^> %DIST_PATH%
"%VENOM_EXE%" build "%SITE%" --out "%DIST_PATH%"
if errorlevel 1 exit /b 1
"%VENOM_EXE%" verify-runtime "%DIST_PATH%" --target browser --require-real-engine
if errorlevel 1 exit /b 1
for %%F in ("%DIST_PATH%\assets\app\app.*.vbc") do (
  if exist "%%~F" (
    echo [venom] package: %%~F
    exit /b 0
  )
)
echo [venom] error: package was not emitted under %DIST_PATH%\assets\app
exit /b 1
