# WASM static script-arena lifecycle

The browser runtime treats the TurboJS/TurboJS script buffer as a fixed arena,
not as a heap allocation. The host retains the validated record length and
zeroes exactly that range after execution.

## Invariants

- Host code validates ranges with subtraction (`length <= memorySize - ptr`)
  to avoid integer-wraparound checks.
- Generated browser and engine code never calls the legacy
  `*_script_buffer_free` export after executing untrusted bytecode.
- The compatibility export performs no arena writes. The ABI-13 TurboJS
  artifact returns success without consulting mutable engine state.
- Context teardown and replacement allocation clamp every mutable length to
  the compile-time capacity before zeroization.
- Rebuilding or embedding the patched WASM must regenerate its digest and the
  corresponding embedded header; stale distribution output must not be reused.

These rules make cleanup independent of engine state modified during bytecode
execution and prevent a successful registry activation from failing during the
subsequent cleanup phase.
