# Source API boundary pass — 2026-07-19

## Goal

Turn Venom's source-domain directory layout into a real compile-time encapsulation model. Before this pass, domains compiled independently, but every target inherited the full `src/` include directory. That allowed any implementation header to be included from any domain and made the CMake target graph less strict than the documented architecture.

## Changes

- Classified native headers by actual product ownership and cross-domain use.
- Moved 47 exported headers into domain-local `src/<domain>/include/venom/<domain>/` trees.
- Retained 32 implementation-only headers beside their owning source files.
- Replaced repository-relative cross-domain includes with stable `venom/<domain>/...` includes.
- Split every compiled domain into an interface API target and a private object target.
- Removed the repository-wide `src/` include path from product and test link surfaces.
- Declared API dependencies and implementation dependencies separately, avoiding accidental transitive exposure.
- Added target-scoped private-header access only for internal tests that intentionally inspect pipeline or frontend internals.
- Updated generated-code tools to emit the canonical namespaced includes and new generated-header destinations.
- Added `tools/architecture/check_header_boundaries.py` and registered it in CTest.
- Extended the source-domain checker to reject legacy include spellings and unresolved namespaced headers.

## Boundary model

```text
src/package/
├── include/venom/package/   Exported package API
├── compress.hpp             Package-private helper
├── reader.cpp
└── writer.cpp
```

Consumers include `venom/package/reader.hpp`; they cannot see `compress.hpp` unless they are compiling the package implementation itself.

## Validation

- Full native build completed for the compiler, all first-party domain targets, native package runtime, and QuickJS runtime probes.
- Header boundary check reports 47 public and 32 private headers.
- Source-domain dependency, source layout, source inventory, CMake source/module completeness, generated artifact drift, and typed error API checks passed.
- Production package build and native runtime probes passed.
- Static gate sweep passed, including release certification and full section-layout diversification.
