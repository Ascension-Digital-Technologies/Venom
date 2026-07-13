@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
pushd "%ROOT%" >nul || exit /b 1

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"
set "BUILD_DIR=%~2"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build"
set "FULL_QUICKJS=%~3"
set "WERROR=%~4"

where cmake >nul 2>nul
if errorlevel 1 (
  echo [venom] error: cmake was not found in PATH.
  popd >nul
  exit /b 1
)

echo [venom] configure: %BUILD_DIR% ^(%CONFIG%^) 
if /I "%FULL_QUICKJS%"=="fullquickjs" (
  if /I "%WERROR%"=="werror" (
    cmake -S "%ROOT%" -B "%ROOT%\%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CONFIG%" -DVENOM_ENABLE_FULL_QUICKJS=ON -DVENOM_ENABLE_WERROR=ON
  ) else (
    cmake -S "%ROOT%" -B "%ROOT%\%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CONFIG%" -DVENOM_ENABLE_FULL_QUICKJS=ON
  )
) else if /I "%FULL_QUICKJS%"=="full-qjs" (
  if /I "%WERROR%"=="werror" (
    cmake -S "%ROOT%" -B "%ROOT%\%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CONFIG%" -DVENOM_ENABLE_FULL_QUICKJS=ON -DVENOM_ENABLE_WERROR=ON
  ) else (
    cmake -S "%ROOT%" -B "%ROOT%\%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CONFIG%" -DVENOM_ENABLE_FULL_QUICKJS=ON
  )
) else if /I "%FULL_QUICKJS%"=="werror" (
  cmake -S "%ROOT%" -B "%ROOT%\%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CONFIG%" -DVENOM_ENABLE_WERROR=ON
) else (
  cmake -S "%ROOT%" -B "%ROOT%\%BUILD_DIR%" -DCMAKE_BUILD_TYPE="%CONFIG%"
)
if errorlevel 1 (
  popd >nul
  exit /b 1
)

echo [venom] build: %BUILD_DIR% ^(%CONFIG%^) 
cmake --build "%ROOT%\%BUILD_DIR%" --config "%CONFIG%" --parallel
if errorlevel 1 (
  popd >nul
  exit /b 1
)

call "%ROOT%\scripts\find-venom.bat" "%BUILD_DIR%" "%CONFIG%"
if errorlevel 1 (
  popd >nul
  exit /b 1
)

echo [venom] built: %VENOM_EXE%
popd >nul
endlocal
