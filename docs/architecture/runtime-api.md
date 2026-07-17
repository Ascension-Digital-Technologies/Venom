# Stable runtime API

Venom 2.0.0 publishes `@venom-js/runtime` as the supported browser integration surface. Application code should not depend on `globalThis.__venomRuntime`, candidate IDs, worker opcodes, package internals, or QuickJS/WASM export names.

The contract is governed by `contracts/runtime-api.json`. The JavaScript SDK is SemVer-stable within a major release; QuickJS/WASM ABI and bytecode formats require exact compatibility; package schemas remain backward-compatible within a major release; generated clients must match the compiler/runtime version.

## Lifecycle

`initializeVenom()` is idempotent and shares concurrent initialization. Calls accept `AbortSignal` and timeout options. `getRuntimeStatus()` reports initialization, readiness, failure, and disposal. `disposeRuntime()` is idempotent and permanently rejects future work for the current page runtime.

## Error model

All SDK failures are exposed as `VenomRuntimeError` with a stable `code`, `phase`, `recoverable` flag, and optional structured details. Application code should branch on error codes rather than parsing messages.
