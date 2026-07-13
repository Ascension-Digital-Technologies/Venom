# Emscripten Windows quoting fix

Venom v0.88.0 fixes a Windows-only command construction bug in the
Emscripten controller.

Older controllers ran the real QuickJS WASM build through a chained command like:

```bat
cmd.exe /d /s /c call "build\emsdk\emsdk_env.bat" && "scripts\build-quickjs-wasm.bat" "build\quickjs-wasm"
```

On some Windows shells this caused `cmd.exe` to treat the quote characters as part of the executable name, producing an error similar to:

```txt
'\"C:\...\emsdk_env.bat\"' is not recognized as an internal or external command
```

The controller no longer chains through `emsdk_env.bat` for the actual build.
`setup_emscripten.py` resolves the installed `emcc.exe` or `emcc.bat` path, the
controller sets `EMCC` to that exact path, and the QuickJS WASM build script is
invoked directly through PowerShell.

Recommended command on Windows:

```bat
build-emscripten.bat
```

If the Emscripten SDK is already installed in `build\emsdk`, the script reuses it.


## Empty `--emcc` argument hardening

The PowerShell QuickJS build now constructs lifecycle arguments as an array and
only appends `--emcc` when a non-empty compiler value exists. This avoids a
PowerShell native-command behavior where an empty string is omitted, leaving a
bare `--emcc` option for Python to reject.

The Emscripten controller also passes the resolved compiler path explicitly:

```powershell
build-quickjs-wasm.ps1 -OutDir <output> -Emcc <resolved-emcc.exe>
```

`EMCC` environment discovery remains supported as a fallback, and the lifecycle
resolver tolerates a legacy bare `--emcc` by continuing with normal discovery.
