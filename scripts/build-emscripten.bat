@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-emscripten.ps1" %*
exit /b %ERRORLEVEL%
