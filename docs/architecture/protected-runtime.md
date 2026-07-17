# Protected Runtime

> **Applies to:** Venom 1.1.0

The protected runtime executes compiled QuickJS bytecode inside WebAssembly hosted by a dedicated worker. The browser page communicates through a narrow asynchronous API rather than receiving direct access to protected modules or runtime memory.

## Components

- **Verified loader** — resolves and validates the generated build assets.
- **Browser runtime** — coordinates route hydration, public API readiness, and browser-side execution.
- **Dedicated worker** — owns protected-call dispatch and session state.
- **Package WASM runtime** — receives package bytes, validates package structure, materializes sections, and resolves route data.
- **QuickJS/WASM runtime** — loads and executes protected QuickJS records.
- **Binary capability bridge** — transports bounded arguments, results, cancellation, and errors.

## Startup sequence

1. load and verify the build manifest and required assets;
2. initialize the package and route WASM runtime;
3. upload the VBC package in validated chunks;
4. initialize the protected worker and private message channel;
5. verify runtime ABI, capability registry, and integrity seals;
6. materialize the current route and protected scripts;
7. mark the public `venom` API ready.

## Protected call sequence

1. browser code invokes a generated protected export;
2. arguments are validated as bounded JSON-safe values;
3. the browser derives a session-bound, counter-bound capability lease;
4. a transferable binary frame is sent over the private `MessagePort`;
5. the worker validates generation, counter, operation, capability, size, and integrity;
6. the QuickJS execution domain validates the bytecode handoff and executes the export;
7. the result or sanitized error is returned in a bound response frame;
8. temporary bridge and runtime buffers are cleared.

## Failure behavior

Production runtime failures do not fall back to `eval`, `new Function`, readable source execution, or a browser-side replacement engine. Missing, stale, malformed, or mismatched runtime components stop boot or reject the protected operation.

## Observation boundary

Workers and WebAssembly improve separation and reduce convenient source-level hooks. They remain observable to a user who controls the browser. See [Trust boundaries](trust-boundaries.md) and [Security model](../security/security-model.md).
