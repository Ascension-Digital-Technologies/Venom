# WebAssembly Memory Hardening

> **Applies to:** Venom 1.65.2
> **Security goal:** Reduce convenient JavaScript-side memory interception and shorten plaintext lifetimes.

Venom's QuickJS and package runtimes still require exported WebAssembly memory for the current Emscripten ABI. A browser owner can therefore inspect memory with sufficient instrumentation. Venom does not claim otherwise.

Version 1.59 centralizes all bridge and script-buffer access behind validated range helpers. Every pointer/length pair is checked for overflow and bounds before a view is created. Inputs are copied through short-lived views, WASM input ranges are overwritten before release, bridge request buffers are erased, and copied result buffers are zeroed after parsing.

## Protected call lifecycle

```text
binary capability frame
→ validated worker request
→ short-lived encoded envelope
→ bounds-checked WASM input range
→ QuickJS invocation
→ copied result bytes
→ parse and validate result
→ erase copied result
→ overwrite WASM input range
→ release runtime allocation
```

## What this improves

- Removes scattered direct `memory.buffer` writes from protected-call code.
- Rejects integer overflow and out-of-range pointer/length pairs.
- Reduces the useful lifetime of request and result plaintext.
- Makes future memory-generation and allocator checks enforceable in one place.
- Prevents accidental retention of bridge envelopes after invocation.

## What it cannot do

A user controlling the browser can patch the helper, instrument the WebAssembly engine, or capture values during execution. Truly secret keys and authoritative decisions must remain on a trusted server.
