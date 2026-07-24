# Source API boundary pass — 2026-07-19

## Goal

Turn Venom's source-domain directory layout into a real compile-time encapsulation model. Before this pass, domains compiled independently, but every target inherited the full `src/` include directory. That allowed any implementation header to be included from any domain and made the CMake target graph less strict than the documented architecture.

## Changes

- Classified native headers by actual product ownership and cross-domain use.
- The later Venom 3.0 layout colocates every first-party header directly with its implementation under `src/<domain>/`.
- Retained 32 implementation-only headers beside their owning source files.
- Uses source-root-relative cross-domain includes such as `package/reader.hpp` and `pipeline/build.hpp`.
- Split every compiled domain into an interface API target and a private object target.
- Uses the repository `src/` root as the canonical include base so domain ownership remains visible in every include.
- Declared API dependencies and implementation dependencies separately, avoiding accidental transitive exposure.
- Added target-scoped private-header access only for internal tests that intentionally inspect pipeline or frontend internals.
- Updated generated-code tools to emit source-root-relative includes and generated headers under `src/generated/`.
- Added `tools/architecture/check_header_boundaries.py` and registered it in CTest.
- Extended the source-domain checker to reject legacy include spellings and unresolved namespaced headers.

## Boundary model

```text
src/package/
├── reader.cpp
├── reader.hpp
├── writer.cpp
├── writer.hpp
└── compress.hpp
```

Consumers include `package/reader.hpp`; related implementation and header files remain together.

## Validation

- Full native build completed for the compiler, all first-party domain targets, native package runtime, and TurboJS runtime probes.
- Header layout check reports 79 colocated first-party headers.
- Source-domain dependency, source layout, source inventory, CMake source/module completeness, generated artifact drift, and typed error API checks passed.
- Production package build and native runtime probes passed.
- Static gate sweep passed, including release certification and full section-layout diversification.
