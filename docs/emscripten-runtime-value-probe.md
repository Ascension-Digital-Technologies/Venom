# Emscripten runtime value probe fix — v0.88.0

v0.88.0 fixes the next Windows cutover failure after Emscripten setup and ABI export preflight succeed.

The failing log looked like this:

```text
runtime value error: instantiate: WebAssembly.instantiate(): Import #0 "env": module is not an object or function
runtime value checks: FAIL
...
strict real-engine verification failed:
artifact_kind: expected 'upstream-quickjs-wasm', got 'contract-scaffold'
```

Two issues were involved:

1. `wasm_exports.py` instantiated the generated Emscripten module with an empty import object. Emscripten-produced WASM can still import an `env` module even when the runtime-value exports are simple metadata functions. The probe now reads `WebAssembly.Module.imports(...)` and synthesizes inert imports for functions, memory, tables, and globals.
2. `build-quickjs-wasm.ps1` did not stop when an external command returned a non-zero exit code. PowerShell does not throw automatically for native process failures. The script now checks `$LASTEXITCODE` after every Python/emcc/cutover step and fails immediately instead of printing a false success message.

With this patch, a valid generated QuickJS WASM artifact should proceed to embed and then flip embedded provenance to:

```text
artifact_kind=upstream-quickjs-wasm
runtime_implementation=quickjs-wasm-upstream-quickjs
real_engine_candidate=true
full_upstream_quickjs=true
required_exports_satisfied=true
finish_blocker=none
```

If a runtime value still fails, the build stops at the exact failing command and the `.bat` launcher keeps the window open.

## v0.88.0 PowerShell external-command error fix

v0.88.0 fixes a Windows PowerShell parser failure in `scripts/build-quickjs-wasm.ps1`. PowerShell treats `$name:` as a scoped variable reference, so the previous error string `exit code $code:` could fail before the QuickJS WASM build started. The script now uses `${code}:` in the thrown error message and the workflow smoke test checks for that exact guarded form.

## v0.88.0 version-source guard

v0.88.0 adds a package/runtime version source smoke test so the generated
Emscripten WASM cannot be rejected because one C runtime source still returns an
older `venom_qjs_engine_version` while the Python verifier expects the current
package version. The guarded sources are:

- `src/quickjs/abi.hpp`
- `src/runtime/quickjs_runtime_scaffold.c`
- `src/runtime/quickjs_upstream_runtime.c`
- `tools/build_emscripten.py`
- `tools/quickjs_wasm_cutover.py`
- `tools/quickjs_wasm_preflight.py`
- `tools/setup_emscripten.py`
- `tools/analyze_dist.py`

The intended runtime-value result for this version is:

```txt
venom_qjs_engine_abi == 12
venom_qjs_engine_version == 78
venom_qjs_real_engine_candidate == 1
venom_qjs_fallback_required == 0
venom_qjs_upstream_quickjs_ready == 1
```
