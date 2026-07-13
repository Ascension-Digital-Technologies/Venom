# v0.88.0 Full host API expansion

v0.88.0 expands the protected upstream QuickJS WASM host bridge beyond the first implementation fixture. The browser compatibility site now exercises a wider group of real-world browser APIs while `Function` and `eval` remain blocked in the protected profile.

## Newly covered bridge behavior

The protected real-engine harness now validates:

- `dataset` reads/writes through `data-*` attributes
- `setAttribute` plus `removeAttribute` state transitions
- `document.createElement`, `appendChild`, and created-node lookup
- `querySelectorAll`, `closest`, and `matches` selector behavior
- JSON and text fetch response flows
- `async` / `await` Promise markers and rejected Promise capture
- route click handling through `history.pushState`
- a deeper static module graph with a second imported dependency

The bridge remains declarative and audited. Protected replay reports `safe-declarative-host-bridge-replay`, `sourceEvalUsed: false`, `replayEvalUsed: false`, and `replayFunctionConstructorUsed: false`.

## Required v0.88.0 truth markers

The embedded WASM runtime-value gate must report:

```txt
venom_qjs_engine_abi=12
venom_qjs_engine_version=83
venom_qjs_real_engine_candidate=1
venom_qjs_fallback_required=0
venom_qjs_upstream_quickjs_ready=1
```

The protected browser harness must observe host bridge telemetry for DOM, forms, routing, modules, fetch, timers, and promises, with at least 18 applied bridge effects.
