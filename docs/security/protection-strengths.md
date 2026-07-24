# Protection strengths and evidence

> **Audience:** evaluators, integrators, security reviewers, and runtime contributors  
> **Status:** Stable  
> **Applies to:** Venom 1.1.0

This page maps Venom's public protection claims to the concrete mechanisms implemented in the repository. It is intentionally precise about what is protected and what remains observable in a browser controlled by an analyst.

## Exact source recovery

Production distributions do not ship the original protected JavaScript source, source maps, comments, formatting, extraction reports, or the original module text. Protected code is represented as TurboJS bytecode inside build-specific package records. The exact source therefore cannot be reconstructed byte-for-byte from `dist/` alone.

That does not prevent behavioral reconstruction. An analyst can instrument the browser, capture inputs and outputs, inspect WebAssembly memory, hook the TurboJS boundary, or manually write equivalent source. Venom's goal is to increase the specialization, time, and per-build effort required.

## Browser-visible JavaScript hardening

Production JavaScript passes through embedded, pinned Terser 5.49.0 and `javascript-obfuscator` 5.4.7 bundles executed in-process by Venom’s linked TurboJS engine. Depending on asset role, the pipeline applies top-level mangling, multi-pass compression, encoded and shuffled string arrays, hexadecimal identifiers, split strings, control-flow transformation, dead-code injection, numeric expression transformation, transformed object keys, and self-defending output. Source-map emission is disabled.

The worker intentionally uses a more conservative transformation set where aggressive transformations would threaten runtime compatibility.

## Polymorphic build generation

Venom's `PolymorphicPlan` derives isolated deterministic streams from a 256-bit master seed. Production generation can vary:

- Route VM logical-to-physical opcode mappings;
- instruction word order and operand masks;
- DOM command identifiers and field layout;
- host-call class identifiers;
- route and string ordering;
- package section ordering, offsets, alignment, and padding;
- generated names, aliases, and release identifiers;
- bridge opcode and capability inputs; and
- stored TurboJS envelope representation.

A deterministic seed reproduces a build for qualification. A different seed or normal unseeded production build produces a different physical representation with equivalent intended behavior.

## TurboJS bytecode storage

The pinned TurboJS compiler emits canonical records, but production package sections store `VTJSE006` envelopes instead of directly exposing canonical `VTJSBC03` or `VTJSMB04` records. The envelope binds the record to the build salt, route, source identity, and execution order; applies a per-build 16-lane permutation and stream transform; and carries ABI and inner-record integrity checks.

Venom does not claim to renumber upstream TurboJS interpreter opcodes. The official engine ABI is preserved after validated reconstruction. The polymorphism applies to the stored record, while Venom's Route VM uses actual build-specific physical opcode mappings.

## Package decoding and memory lifecycle

Package bytes are uploaded to WASM-owned resident storage through an ordered chunk protocol. The JavaScript fetch buffer is cleared after successful residency. Sections are authenticated, decompressed, and materialized lazily. Transient package outputs, decoded records, bridge envelopes, TurboJS input ranges, copied results, route buffers, and retired package storage are explicitly overwritten at their earliest safe release point.

All JavaScript-accessible WASM memory operations use centralized bounds and overflow validation. Exported WebAssembly memory remains inspectable to a browser owner; the mechanism reduces exposure time and unsafe access, not observability to a privileged analyst.

## Binary bridge protocol

The public `venom.exports.*` API is backed by a private `MessageChannel`. Calls use transferable binary frames rather than readable message arrays. Frames include generation state, counters, opaque capabilities, bounded payload lengths, request identifiers, and an integrity tag.

Each worker session rotates invoke, cancel, result, and error operations. Request capabilities are derived as single-use leases from the base capability, worker generation, session material, and monotonic counter. The worker rejects stale counters, replayed frames, malformed ranges, invalid tags, incorrect generations, and unknown leases.

The inner value model remains a bounded JSON-safe contract. The protocol protects structure and integrity; it does not make call arguments secret from an analyst controlling both endpoints.

## Split trust domains and runtime integrity

The package runtime owns upload, package validation, section materialization, and retirement. The TurboJS execution runtime owns bytecode allocation and execution. A short-lived handoff binds decoded bytes to the route, source, order, length, content hash, producer, and consumer; the execution domain independently verifies it.

Compiler-generated integrity seals cover worker capability state, registry bytecode, bridge operations, release policy, TurboJS ABI identity, and route mapping state. Checks run at initialization and at relevant invocation or route-execution boundaries.

## Release enforcement

Production builds require the verified TurboJS/WASM runtime and deterministic JS hardener. Release inspection checks package polymorphism, asset binding, runtime provenance, ABI compatibility, raw record leakage, readable internal names, source/debug markers, source maps, and missing or mismatched runtime assets. Stable packaging is signed and fail-closed.

## Boundary statement

Venom is strongest as a cost-amplification system for valuable client-side implementation logic. It is not a substitute for server-side authority. Credentials, private keys, irreversible authorization, authoritative financial decisions, and secrets that must never reach the user remain server responsibilities.
