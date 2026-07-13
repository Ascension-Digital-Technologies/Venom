# v0.88.0 Route, module, and fetch hardening

v0.88.0 expands the verified upstream QuickJS WASM browser path beyond broad host API coverage into deeper route/module/fetch behavior. The protected profile still blocks `Function` and `eval`, and the runtime must continue to report `venom_qjs_engine_version=83` through the embedded WASM runtime-value gate.

## New protected bridge coverage

The real-engine compatibility harness now validates:

- fetch status/header/text/json behavior, including a fetch status/header marker
- deny-by-default host-call handling for a blocked fetch request
- route hydration markers after a multi-page navigation click
- async throw/catch propagation in addition to Promise rejection capture
- static module graph ordering across leaf, branch, extra, and entry modules
- host-bridge telemetry operations for `fetch-response`, `fetch-headers`, `host-deny`, `async-errors`, `module-order`, and `route-hydration`

## Required markers

```txt
quickjs_runtime_artifact_kind: upstream-quickjs-wasm
quickjs_runtime_full_upstream_quickjs: yes
quickjs_runtime_finish_blocker: none
venom_qjs_engine_version=83
```

The hardened dist must still pass:

```bat
build\venom.exe verify-runtime dist-hardened --target browser --require-real-engine
```

The module order check verifies the leaf > branch > entry execution order through the protected bridge.
