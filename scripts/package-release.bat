@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
where python >nul 2>nul
if %ERRORLEVEL%==0 (
  python "%REPO_ROOT%\tools\package_release.py" --repo-root "%REPO_ROOT%" %*
  exit /b %ERRORLEVEL%
)
where py >nul 2>nul
if %ERRORLEVEL%==0 (
  py -3 "%REPO_ROOT%\tools\package_release.py" --repo-root "%REPO_ROOT%" %*
  exit /b %ERRORLEVEL%
)
echo Python 3 is required to package a release.
exit /b 1
