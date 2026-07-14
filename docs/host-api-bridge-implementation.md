> **Superseded by v1.0.4:** protected execution no longer decodes JavaScript source or performs source-token effect replay. See `security-hardening-v1.0.4.md`.

# v0.88.0 Host API bridge implementation

v0.88.0 moves the real upstream QuickJS WASM browser path from telemetry-only parity into a stricter protected host API bridge implementation gate.

The protected runtime still denies host JavaScript source-eval fallback. When the upstream QuickJS WASM backend accepts a protected `VQJSBC03` record, the browser bridge now performs a safe declarative host-effect replay for audited host-facing operations instead of invoking `Function`, `eval`, or arbitrary source text.

## Bridge replay contract

Execution records now expose:

```text
hostBridgeTelemetry.actualHostEffects = true
hostBridgeTelemetry.effectReplayCount > 0
hostBridgeTelemetry.replayMode = safe-declarative-host-bridge-replay
hostBridgeTelemetry.replayEvalUsed = false
hostBridgeTelemetry.replayFunctionConstructorUsed = false
hostBridgeTelemetry.effectReplay.receipts[]
```

The replay layer covers the current real-engine browser compatibility fixture:

```text
DOM text and attribute mutation
classList add/remove/toggle
style mutation
innerHTML fragment insertion
click event listener registration
form submission state
Promise resolve/reject continuation markers
fetch JSON completion markers
timer completion markers
module dependency/export globals
```

This is still intentionally narrower than full browser API parity. It is a fail-closed bridge implementation for protected builds: only recognized audited host-facing operations are replayed, and arbitrary source execution remains blocked.

## Validation

The browser runtime compatibility smoke now requires all of the following under `prod` with source-eval blocked:

1. `verify-runtime --target browser --require-real-engine` passes.
2. Upstream QuickJS WASM execution records are present.
3. Host bridge telemetry includes the expected DOM/fetch/timer/event/module operations.
4. The protected DOM state is actually mutated by the bridge.
5. Event handlers, form state, Promise markers, fetch markers, timer markers, and module globals are visible to the harness.
6. Replay metadata proves `eval` and `Function` were not used.
