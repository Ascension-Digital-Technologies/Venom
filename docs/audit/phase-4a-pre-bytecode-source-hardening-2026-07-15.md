# Phase 4A — Pre-bytecode source hardening

Date: 2026-07-15

## Objective

Reduce information recoverable from protected TurboJS bytecode without replacing the TurboJS execution engine or weakening JavaScript compatibility.

## Changes

Production builds now run protected JavaScript through the embedded Terser + javascript-obfuscator pipeline **before** TurboJS bytecode compilation. Development builds remain unchanged for diagnostics.

The protected-code hardener uses a build-specific salt and enables:

- top-level and global identifier renaming;
- function/class name removal;
- high-coverage RC4 string-array encoding;
- transformed string-array calls;
- string splitting;
- moderate control-flow flattening;
- low-rate dead-code injection;
- object-key transformation where semantics permit it;
- per-build deterministic diversification.

The same protection is applied to:

- whole-file protected scripts;
- protected ES modules;
- the protected bridge registry;
- lazy protected registry chunks.

## Security effect

This directly addresses the easiest clean-room leakage observed in prior dist analysis:

- internal function names are no longer expected to survive as a readable algorithm map;
- application-specific string constants are encoded before TurboJS serializes constants;
- protected bytecode differs between independently salted builds;
- a generic TurboJS bytecode extractor receives already-obfuscated program logic.

## Compatibility boundary

Property renaming remains disabled because arbitrary JavaScript property names are observable and may cross browser/host boundaries. Public protected export names remain part of the bridge contract and cannot be hidden from a client that must invoke them.

## Verification

- Full native project compilation completed successfully.
- `venom_diversification_rng` passed.
- `venom_native_bytecode_security` passed.
- `turbojs_module_bundle_runtime` passed.

## Residual risks

This pass does not remove:

- recognizable TurboJS runtime machinery;
- fixed `VTJSE006` / `VJSB0006` envelope markers;
- the publicly understood TurboJS bytecode instruction set after runtime decoding;
- stable WASM interpreter architecture;
- externally visible bridge/API property names.

Recommended follow-on work:

1. Per-build binary envelope signatures and field layouts.
2. WASM-owned incremental bytecode decoding with no full plaintext buffer.
3. Runtime handler and import/export diversification.
4. Optional Venom IR/custom VM backend for maximum-protection functions.
5. Automated clean-room leakage scoring as a release gate.
