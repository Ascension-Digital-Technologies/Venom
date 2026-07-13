@echo off
setlocal EnableExtensions
set "ROOT=%~dp0.."

for %%D in (build build-debug build-release build-fullqjs build-werror dist dist-basic dist-basic-wasm dist-basic-protect dist-basic-release dist-release dist-protect dist-wasm) do (
  if exist "%ROOT%\%%D" (
    echo [venom] remove %%D
    rmdir /s /q "%ROOT%\%%D"
  )
)

endlocal
