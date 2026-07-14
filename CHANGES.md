# Changelog

This changelog records public Venom releases and meaningful user-facing changes. Internal development passes, documentation polish, repository cleanup, and CI maintenance are intentionally not versioned separately.

Venom follows [Semantic Versioning](docs/operations/versioning.md).

## 1.0.0

### First public release

- Introduced Venom's hybrid browser and protected QuickJS/WebAssembly execution model.
- Added protected-by-default JavaScript planning with file- and function-level `@venom` execution directives.
- Added local protection analysis, manual realm overrides, confidence reporting, and fail-closed production planning.
- Compiled protected JavaScript into QuickJS bytecode and packaged it in polymorphic `.vbc` containers with build-specific bytecode envelopes.
- Added worker-isolated protected execution, WebAssembly-owned streamed decoding, runtime-integrity seals, asset binding, and a private binary capability bridge.
- Added least-privilege host-capability manifests and capability-scoped protected-runtime facades.
- Embedded pinned Terser and javascript-obfuscator hardening directly into the native compiler through QuickJS, removing the external Node.js hardener requirement.
- Added deterministic production builds, release leakage scanning, tamper rejection, runtime provenance checks, and fail-closed release qualification.
- Added structured build phases, elapsed timings, useful build statistics, and `--verbose` / `--quiet` output controls.
- Added complete installation, integration, architecture, security, operations, CLI, and example documentation.
- Included Protected Chess, NOVA TRADE, and Venom Sentinel as complete reference applications.

### Security boundary

- Production distributions do not ship the exact original protected source and cannot reproduce it byte-for-byte.
- Venom raises the cost of reverse engineering, tampering, extraction, and reuse; it does not claim permanent secrecy against an analyst who controls the browser and operating environment.
