@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-quickjs-wasm.ps1" %*
set "ec=%errorlevel%"
if not "%ec%"=="0" if not defined VENOM_NO_PAUSE pause
exit /b %ec%
