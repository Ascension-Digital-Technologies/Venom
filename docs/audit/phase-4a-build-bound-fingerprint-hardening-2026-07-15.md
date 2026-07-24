# Phase 4A — Build-Bound Fingerprint Hardening

Date: 2026-07-15

## Objective

Reduce transferability of reverse-engineering work across Venom production builds and remove descriptive application metadata from protected TurboJS records.

## Implemented

1. **Build-bound JavaScript hardener seeds**
   - `ast_harden_release_js` now accepts a build diversification salt.
   - Runtime, engine, worker, loader, protected scripts, modules, and registries are salted per build.
   - The hardener cache identity is versioned to `venom-hardener-cache-v2` and includes the build salt.

2. **Per-build identifier namespace**
   - JavaScript Obfuscator's identifier prefix now includes the derived seed.
   - Identical generated runtime sources no longer retain the same identifier namespace across independently diversified builds.

3. **Protected-source hardening before TurboJS serialization**
   - Production protected scripts and modules pass through the native Terser/JavaScript-Obfuscator pipeline before bytecode compilation.
   - Development builds remain readable.

4. **Opaque production labels**
   - Route labels, source labels, module compilation names, and protected-registry compilation names are replaced by build-bound SHA-256 aliases.
   - Development builds preserve descriptive labels for diagnostics.

5. **Registry diversification**
   - Protected bridge registry and lazy registry chunks receive build-bound hardening and opaque compilation names.

## Security impact

This pass directly targets:

- stable runtime patterns;
- retained function and local identifiers;
- application-specific source and route strings;
- reusable signatures across builds;
- descriptive TurboJS compilation metadata.

It does **not** claim that client-delivered code is unrecoverable. TurboJS/WASM remains observable at runtime. The improvement is that analysis artifacts from one build are less reusable against another and static distributions expose fewer semantic anchors.

## Validation

- C++ compilation completed through `libvenom_core.a` and all modified translation units.
- `venom_diversification_rng_test`: PASS.
- `venom_html_route_hydration_test`: PASS.
- `cryptographic-diversification-smoke.py`: PASS.
- Final executable linking was not performed in the lightweight validation configuration because `VENOM_ENABLE_FULL_TURBOJS=OFF` intentionally omits the native TurboJS symbols.
- The existing `build-specific-bytecode-envelope-smoke.py` expects an older scanner source expression and fails against the archive's current implementation; this is an existing test/source drift issue, not a compilation failure introduced by this pass.

## Recommended next phase

Move from a shared TurboJS bytecode ISA toward a generated Venom IR/VM backend for selected high-value functions, while retaining TurboJS as the compatibility backend. Add differential two-build tests that assert low similarity for loader/runtime JavaScript, WASM export tables, protected labels, and registry bytecode.
