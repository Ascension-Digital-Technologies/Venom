# Architecture overview

Venom is a compiler, package system, loader, worker runtime, QuickJS execution engine, WebAssembly package decoder, route runtime, release verifier, and signed update toolchain.

## Build pipeline

```text
HTML / CSS / JavaScript
        │
        ├─ execution-runtime analysis
        ├─ route and asset compilation
        ├─ protected function/module extraction
        ├─ QuickJS bytecode compilation
        ├─ build-specific package and bytecode diversification
        ├─ runtime/loader generation and hardening
        └─ release verification
```

## Runtime pipeline

```text
verified loader
  → dedicated worker
  → WASM-owned package upload and decoding
  → context-bound bytecode handoff
  → QuickJS/WASM execution
  → leased binary capability bridge
  → browser-owned rendering and interaction
```

The browser continues to own the DOM, framework runtime, navigation, and browser APIs. Valuable logic can execute in the protected worker runtime and expose only narrow asynchronous JSON-safe contracts.

For implementation detail, continue with the [compiler pipeline](compiler-pipeline.md), [protected runtime](protected-runtime.md), and [trust boundaries](trust-boundaries.md).
