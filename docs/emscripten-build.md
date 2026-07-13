# Emscripten build controller — v0.88.0

v0.88.0 adds a single controller for the full Emscripten path:

```text
setup/check emsdk
  -> strict QuickJS WASM preflight
  -> build quickjs-runtime.wasm with emcc
  -> inspect ABI12 exports
  -> embed generated artifact
  -> verify embedded real-engine provenance
  -> rebuild the native compiler against the new embedded header
  -> build and strictly verify a protected smoke distribution
```

The controller lives at:

```text
tools/build_emscripten.py
scripts/setup-emscripten.sh
scripts/setup-emscripten.ps1
scripts/setup-emscripten.bat
build-emscripten.bat
```

## Smoke/preflight mode

This mode never downloads Emscripten and is safe for CI machines without `emcc`:

```bash
scripts/setup-emscripten.sh --preflight-only --allow-missing
```

PowerShell:

```powershell
scripts\build-emscripten.ps1 -PreflightOnly -AllowMissing
```

It writes:

```text
build/emscripten-build/emscripten-build.txt
build/emscripten-build/emscripten-build.json
build/emscripten-build/setup/emscripten-setup.txt
build/emscripten-build/preflight/quickjs-wasm-preflight.txt
```

## Full build mode

On a machine that can download or already has `emsdk`:

```bash
scripts/setup-emscripten.sh
```

Windows:

```bat
build-emscripten.bat
```

The default mode can clone/install/activate emsdk, then runs the existing verified QuickJS WASM cutover path. If the generated WASM is not a real upstream QuickJS artifact with all ABI12 exports present, the build fails and the checked-in scaffold remains honest.

## Offline or preinstalled SDK mode

Use this when `emsdk` already exists or network access is blocked:

```bash
scripts/setup-emscripten.sh --skip-download --emsdk-dir build/emsdk
```

or point directly at `emcc`:

```bash
EMCC=/path/to/emcc scripts/setup-emscripten.sh --skip-setup
```

## Truth rule

This pass still does not mark the browser runtime as real unless the generated `.wasm` passes:

```text
artifact_kind=upstream-quickjs-wasm
required_exports_satisfied=true
real_engine_candidate=true
full_upstream_quickjs=true
finish_blocker=none
```

## v0.88.0 Windows error visibility

The root `.bat` wrappers now keep the console open on failures. Double-click the root wrapper, for example `build-emscripten.bat`, and if setup/build fails it will print the error and wait for a keypress before closing.

To disable that behavior in automation:

```bat
set VENOM_NO_PAUSE=1
build-emscripten.bat --skip-setup
```

## v0.88.0 runtime-value gate

The WASM cutover now checks more than export names. A module must also instantiate and return the expected ABI truth values:

```txt
venom_qjs_engine_abi == 12
venom_qjs_engine_version == current package version
venom_qjs_real_engine_candidate == 1
venom_qjs_fallback_required == 0
venom_qjs_upstream_quickjs_ready == 1
```

This catches stale artifacts that expose the right ABI names but were generated from an older runtime.

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


## v0.88.0 runtime-value probe import fix

v0.88.0 fixes the generated-WASM runtime value probe on Windows/Emscripten builds. The probe now synthesizes the imported `env`/WASI objects required to instantiate the module for metadata exports, and `build-quickjs-wasm.ps1` now checks `$LASTEXITCODE` after every native command so a failed cutover cannot continue with a false success message.

See `docs/emscripten-build.md` for the exact failure pattern and fix.

## v0.88.0 PowerShell external-command error fix

v0.88.0 fixes a Windows PowerShell parser failure in `scripts/build-quickjs-wasm.ps1`. PowerShell treats `$name:` as a scoped variable reference, so the previous error string `exit code $code:` could fail before the QuickJS WASM build started. The script now uses `${code}:` in the thrown error message and the workflow smoke test checks for that exact guarded form.

## v1.30.5 native binding closure

A successful WASM cutover changes `src/compiler/quickjs_runtime_wasm_blob.hpp`. The controller now rebuilds the native compiler automatically so an older `venom.exe` cannot continue packaging the previous runtime. It then builds `examples/protected-chess` and runs `verify-runtime --require-real-engine`.

Targeted toolchain runs may opt out with `--skip-native-rebuild` and `--skip-runtime-smoke`; release workflows should not use those switches.

## v1.30.6 export-surface compatibility

The browser worker requires the full Venom QuickJS release ABI and accepts only the known Emscripten helper exports `__indirect_function_table`, `_emscripten_stack_restore`, and `emscripten_stack_get_current`, with their expected WebAssembly kinds. Unknown exports remain rejected.


## v1.30.7 ABI fingerprint canonicalization

The protected package ABI fingerprint binds the canonical Venom release exports, not incidental Emscripten support exports. The worker and engine allow only `__indirect_function_table`, `_emscripten_stack_restore`, and `emscripten_stack_get_current` as additional toolchain exports. Unknown exports or incorrect WebAssembly kinds remain fatal.
