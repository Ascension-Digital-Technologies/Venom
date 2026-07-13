> **Superseded by v1.0.4:** protected execution no longer decodes JavaScript source or performs source-token effect replay. See `security-hardening-v1.0.4.md`.

# v0.88.0 Host-Call Bridge Parity

v0.88.0 strengthens the real upstream QuickJS WASM browser path with explicit host-call bridge telemetry.

The real-engine browser harness now requires protected browser builds to expose structured host-bridge records while `Function` and `eval` remain blocked. The telemetry proves that the protected runtime accepted opaque `VQJSBC03` QuickJS records through the WASM boundary and that browser-facing effects are represented by the host-call bridge instead of a silent source-eval fallback.

## Telemetry contract

Runtime execution records may include:

```text
hostBridgeTelemetry.boundary = upstream-quickjs-wasm-host-call-bridge
hostBridgeTelemetry.sourceKind = opaque-vqjsbc03-native-object
hostBridgeTelemetry.wasmAccepted = true
hostBridgeTelemetry.upstreamReady = true
hostBridgeTelemetry.fallbackRequired = false
hostBridgeTelemetry.hostCallCount > 0
```

The operation list is derived from the protected QuickJS record and must cover the browser bridge behavior exercised by the compatibility fixture:

```text
dom-query
dom-text
dom-class
dom-style
events
timers
fetch
promises
```

Additional operations such as console, routing, forms, modules, and DOM creation are recorded when present in the protected chunk.

## Validation

The browser runtime compatibility smoke now validates both layers:

1. `verify-runtime --target browser --require-real-engine` must pass.
2. The protected browser harness must run with host source-eval blocked.
3. At least one real-engine record must include host-call bridge telemetry.
4. The expected browser operations must appear in that telemetry.
5. The upstream QuickJS WASM runtime-value gate must report package version 83.

This does not claim full browser API parity yet. It establishes a hard regression gate that the real QuickJS WASM path is carrying browser host-call semantics through explicit bridge telemetry rather than relying on a debug fallback path.
