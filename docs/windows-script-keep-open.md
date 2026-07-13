# Windows scripts stay open — v0.88.0

v0.88.0 changes the Emscripten launchers so double-clicking the batch files is safe. The scripts now pause after completion and after failure by default, so setup errors such as missing Python, Git, CMake, or Emscripten remain visible.

## Recommended commands

From the project root:

```bat
build-emscripten.bat
```

For a persistent command window launcher:

```bat
build-emscripten-cmd.bat
```

For a log file you can share or inspect:

```bat
build-emscripten-log.bat
```

That writes:

```txt
build-emscripten.log
```

## Disable pauses in automation

```bat
set VENOM_NO_PAUSE=1
build-emscripten.bat
```

## Pause behavior controls

```bat
set VENOM_PAUSE_MODE=always
set VENOM_PAUSE_ON_ERROR=1
```

`VENOM_PAUSE_MODE=always` pauses on success and failure. `VENOM_PAUSE_ON_ERROR=1` pauses only on non-zero exit codes.

## v0.88.0 Emscripten environment resolution fix

v0.88.0 fixes the Windows case where `emsdk.bat install` and `emsdk.bat activate` succeed, but `build-emscripten.bat` still reports `missing-emcc`. The root cause is that `emsdk.bat activate` runs in a child `cmd.exe`; its PATH changes do not mutate the already-running Python controller process. The setup tool now resolves the installed SDK entrypoint directly, usually:

```txt
build\emsdk\upstream\emscripten\emcc.bat
```

and passes that resolved `EMCC` path into the preflight/build steps.

Run:

```bat
build-emscripten.bat
```

If the SDK was already downloaded by an older run, this should reuse it instead of recloning.

## v0.88.0 PowerShell external-command error fix

v0.88.0 fixes a Windows PowerShell parser failure in `scripts/build-quickjs-wasm.ps1`. PowerShell treats `$name:` as a scoped variable reference, so the previous error string `exit code $code:` could fail before the QuickJS WASM build started. The script now uses `${code}:` in the thrown error message and the workflow smoke test checks for that exact guarded form.
