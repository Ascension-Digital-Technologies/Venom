# Binary Capability Bridge

> **Audience:** integrators and security reviewers  
> **Status:** production protocol  
> **Applies to:** Venom 1.1.0
Venom 1.57 replaces the readable array-based worker bridge with a private `MessagePort` carrying transferable binary frames. The public `venom.exports.*` API remains unchanged.

## Frame design

Each frame binds the protocol version, worker generation, monotonic sequence counter, opaque capability identifier, request identifier, payload length, payload bytes, and a session integrity tag. Frames are transferred as `ArrayBuffer` objects rather than ordinary JavaScript message arrays.

The worker rejects malformed lengths, unknown capabilities, stale counters, replayed requests, incorrect generations, and invalid integrity tags. Capability identifiers are derived from per-build bridge opcodes and export slots, so browser messages do not contain protected function names.

## Private transport

A `MessageChannel` is created during worker preparation. One port is transferred to the worker and the browser-side port remains inside the loader closure. Bridge traffic does not use the worker's public `onmessage` surface after preparation.

## Value contract

Arguments and results still use the `json-value-v1` semantic contract for compatibility and deterministic validation. JSON is encoded inside the binary frame; it is no longer exposed as the outer transport structure. Rich framework objects, DOM nodes, functions, cycles, and non-finite numbers remain rejected.

## Security boundary

This design raises interception and automation cost and provides replay/tamper detection. It does not make browser-delivered values secret from an analyst who controls the page, browser process, or JavaScript engine. The session key is an integrity mechanism, not a server-held secret. Truly sensitive authorization and long-lived secrets must remain server-side.

## Public bridge resource limits

Production bridge calls are bounded before they reach QuickJS and are validated again inside the worker trust domain.

- At most 32 public calls may be pending in the browser bridge.
- The worker executes at most 8 protected calls concurrently.
- Public payloads and protected results are limited to 1 MiB.
- JSON graphs are limited to depth 24 and 16,384 visited nodes.
- Only finite numbers, strings, booleans, null, arrays, and plain objects are accepted.
- `__proto__`, `prototype`, and `constructor` object keys are rejected.
- Non-plain objects, cyclic graphs, functions, symbols, BigInt values, and non-finite numbers are rejected.

These checks are intentionally duplicated across the browser and worker boundaries. Browser-side validation improves behavior for legitimate callers; worker-side validation is authoritative.

The CTest suite includes `protected_bridge_limits_smoke` and `bridge_frame_mutation_smoke`. The mutation test corrupts frame headers, session fields, lengths, payload bytes, integrity tags, counters, and truncation boundaries and requires every mutated frame to be rejected.
