# v0.88.0 structural protected-runtime cutover

> **Historical implementation note:** This page records an earlier development stage. For current behavior and claims, use `README.md`, `docs/architecture.md`, `docs/runtime.md`, `docs/compatibility.md`, and `docs/security-model.md`.

Protected profiles no longer ship dormant host JavaScript execution implementations.

## Enforced invariants

- `protect`, `browser-protect`, `native-protect`, and `release` reject `--allow-host-fallback` and its legacy alias.
- Runtime generation omits inline-handler, engine-module, and legacy `Function` constructor paths.
- The protected QuickJS engine module omits its host execution fallback.
- Generated protected JavaScript is rejected if it contains `new Function`, `eval(`, disabled-eval aliases, or legacy host fallback markers.
- QuickJS/WASM failure remains fatal for protected script execution.

The debug profile retains an explicit compatibility fallback for development only.
