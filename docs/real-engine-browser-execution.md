# Real-engine browser execution validation — v0.88.0

v0.88.0 is the first source package that ships a checked-in, verified upstream QuickJS WASM artifact instead of the older contract scaffold.

The embedded artifact is validated by the same strict gates used for generated Emscripten output:

```txt
artifact_kind=upstream-quickjs-wasm
runtime_implementation=quickjs-wasm-upstream-quickjs
runtime_claim=full-upstream-quickjs-wasm
real_engine_candidate=true
full_upstream_quickjs=true
required_exports_satisfied=true
runtime_values_satisfied=true
finish_blocker=none
```

## Runtime import-object fix

The browser/Node engine module now builds a minimal WebAssembly import object from `WebAssembly.Module.imports(...)` before instantiating `quickjs-runtime.wasm`. This fixes the real Emscripten module path where the runtime imports `env`/WASI-style host entries even though the metadata/value probe only needs inert host shims.

## Compatibility gate

`tests/package/browser-runtime-compat-smoke.py` now builds a protected multi-script browser fixture and requires:

```txt
verify-runtime --target browser --require-real-engine: PASS
quickjs_execution_backend: quickjs-wasm-real
quickjs_bytecode_boundary: wasm-owned
quickjs_host_js_fallback_allowed: no
quickjs_runtime_artifact_kind: upstream-quickjs-wasm
quickjs_runtime_full_upstream_quickjs: yes
```

The Node browser-like harness blocks `Function` and `eval` while booting the protected fixture, so passing this test proves the real WASM execution boundary is accepted without host source-eval fallback.

## Current limitation

The protected real-engine path proves WASM-owned bytecode execution and host-boundary records. Full browser semantic parity still needs more work in the QuickJS host-call bridge so DOM/fetch/timer/event effects can be driven entirely from upstream QuickJS without relying on the debug compatibility fallback path.
