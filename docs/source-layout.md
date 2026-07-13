# Source layout cleanup — v0.88.0

v0.88.0 makes the compiler-side runtime generation easier to maintain after the real upstream QuickJS/WASM cutover.

## Compiler JavaScript modules

The previous `src/compiler/js.cpp` mixed four separate jobs in one large file. It is now split into focused translation units:

- `js.cpp` — script discovery, JS bundle encoding, diagnostics, loader generation, and preview output.
- `runtime_js.cpp` — generated browser runtime module source.
- `wasm_runtime_js.cpp` — generated WASM runtime bridge source and embedded WASM accessors.
- `quickjs_engine_module.cpp` — generated QuickJS engine adapter module source.

This split is intentionally mechanical. The generated runtime text remains byte-for-byte behavior-compatible except for version/package markers.

## Generated blob policy

The embedded WASM headers remain generated artifacts:

- `quickjs_runtime_wasm_blob.hpp`
- `wasm_runtime_blob.hpp`

They are allowed to be large because they are binary payload containers. Production source cleanup should avoid hand-editing those headers; regenerate them from the relevant build/embed scripts instead.

## Runtime truth policy

This cleanup does not flip the real-engine gate. Browser packages still report the checked-in scaffold truth until `build-quickjs-wasm` embeds a verified upstream QuickJS WASM artifact with all ABI12 exports present.

The generated blob policy is enforced by the source-layout smoke test.

- `src/compiler/worker_runtime_js.cpp` owns the worker-side package/WASM attestation and QuickJS readiness probe.
