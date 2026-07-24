# Venom 1.0.1 stable release verification

> [!NOTE]
> This is historical evidence for the 1.0.1 release line. It is retained for traceability and does not describe the current 1.0.0 qualification state.

Venom 1.0.1 records the first public stable source-package verification after the extracted source archive configured, built, and ran successfully on Windows.

## Verified release properties

- Clean extracted source-tree configure and generation
- Complete native compiler and runtime build
- Successful protected-site build and browser runtime execution
- 127-test native CTest closure
- Source-package CMake module and source-list completeness
- Documentation and changelog consistency
- TurboJS/WASM provenance and embedded-runtime synchronization
- Signed stable-release policy and release-entrypoint enforcement
- Production artifact verification and leak-scanning contracts

## Publication boundary

The stable source archive is ready for signed publication. Browser-specific equivalence and benchmark evidence should always identify the exact operating system, browser version, hardware, and build profile used.

Venom is designed to increase the time, specialization, and per-build effort required to recover or modify protected browser-delivered logic. It cannot create permanent secrecy on a device controlled by the analyst.
