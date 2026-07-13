# Browser runtime

Venom's browser runtime coordinates two execution layers: the Route VM interpreter in generated JavaScript and a real QuickJS engine compiled to WebAssembly.

## Boot sequence

A protected browser build follows this high-level sequence:

```text
index.html
  -> load content-addressed loader
  -> start runtime/worker components
  -> fetch app.<hash>.vbc
  -> validate package structure and required metadata
  -> validate expected runtime asset consistency
  -> resolve the current route
  -> decode and execute Route VM bytecode
  -> construct deterministic DOM state
  -> load route QuickJS bytecode records
  -> execute them through QuickJS/WASM
  -> service supported browser effects through the host bridge
```

Strict production builds fail closed when required assets, metadata, ABIs, or the real QuickJS/WASM engine are unavailable or inconsistent.

## Route VM execution

The Route VM interprets a compact package-specific instruction stream. Current logical operations cover deterministic DOM construction behavior such as:

- creating an element;
- entering or leaving the current element context;
- setting attributes;
- appending text;
- appending child nodes;
- ending route execution.

Protected builds encode these operations with per-build opcode maps, masks, field ordering, and table ordering. The generated browser runtime decodes the active package's physical representation before dispatch.

The production Route VM interpreter is generated JavaScript. It should not be described as a separate WebAssembly VM or a native sandbox.

## QuickJS/WASM execution

At build time, Venom compiles scripts with embedded QuickJS in compile-only mode and serializes executable QuickJS objects. Protected output strips source and debug records.

At browser runtime, QuickJS compiled to WebAssembly:

- validates the Venom QuickJS record;
- reads serialized QuickJS objects;
- evaluates functions/modules;
- processes pending QuickJS jobs;
- reports console events, results, and exceptions through the runtime boundary.

The runtime accepts compiler-produced package bytecode. It is not intended as a public service for arbitrary untrusted QuickJS bytecode.

## Host bridge

QuickJS does not directly receive unrestricted browser globals. Supported operations are implemented by the Venom host bridge, including selected DOM, event, timer, fetch, storage, navigation, form, console, and browser-global behavior.

The bridge applies implemented validation and limits, but it is not a complete browser implementation. Missing or semantically different browser APIs are compatibility failures, not JavaScript parser failures.

## Navigation and route lifetime

The generated runtime can resolve compiled routes and perform same-origin packaged navigation. Route selection reconstructs the packaged page and executes scripts associated with that route.

Applications must test navigation, timer/event cleanup, async completion, storage behavior, and repeated route transitions. Documentation should not imply complete route isolation unless a specific behavior is covered by tests.

## Assets

Asset-bearing HTML and CSS references are rewritten to emitted distribution assets where supported. External URLs, data URLs, blobs, anchors, and other special schemes follow the runtime's explicit rewrite rules.

Static remote scripts can be vendored during compilation and pinned in `venom.lock`. Dynamically constructed network URLs and non-script resources remain application/runtime concerns.

## Failure policy

Production output denies silent fallback to readable browser-host JavaScript or decoded source execution. Failures should produce stable runtime errors rather than weaken the execution boundary.

Typical failure categories include:

- missing or stale QuickJS/WASM artifacts;
- runtime/package ABI mismatch;
- asset hash or binding mismatch;
- malformed package metadata;
- unsupported package protection provider;
- unsupported browser API behavior;
- resource limit violation.

## Security boundary

The runtime raises reverse-engineering and modification cost, but every browser component remains under the control of the client. Generated JavaScript, workers, package data, WebAssembly memory, host calls, and materialized values can be inspected by a determined operator.
