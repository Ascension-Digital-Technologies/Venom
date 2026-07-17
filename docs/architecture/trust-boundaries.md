# Trust Boundaries

> **Applies to:** Venom 1.1.0

Venom uses multiple software boundaries to reduce exposure and make generic instrumentation less reusable. None of these boundaries should be treated as equivalent to trusted hardware or a server-controlled environment.

## Build boundary

The compiler, pinned toolchain inputs, embedded runtime sources, JavaScript hardener, and signing environment are trusted. Release gates verify completeness and consistency, but they cannot compensate for a compromised build host or stolen signing key.

## Distribution boundary

The production distribution is public and attacker-controlled after delivery. Package bytes, loader assets, workers, runtime JavaScript, and WebAssembly binaries may all be downloaded and modified. Hash binding and fail-closed checks detect many forms of mismatch; they do not make the files secret.

## Browser/runtime boundary

The page and protected worker use separate JavaScript runtimes and a private message channel. Browser code cannot directly call internal QuickJS objects, but a browser owner can instrument both runtimes.

## Decoder/executor boundary

Package materialization and QuickJS execution are separated. Decoded records carry a content- and context-bound handoff that the execution domain verifies independently. This increases the work required to substitute records across routes, sources, or execution order.

## Memory boundary

Package and QuickJS memories are WebAssembly linear memories. JavaScript uses centralized range-checked access where access is required, and transient plaintext is cleared at the earliest safe point. Browser-level tooling can still observe memory.

## Server boundary

Trusted services remain responsible for secrets, privileged credentials, authoritative entitlements, payments, signing, and irreversible decisions. Venom-protected client logic may assist but must not be the sole security authority.
