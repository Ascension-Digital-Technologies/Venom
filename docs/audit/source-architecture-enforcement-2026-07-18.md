# Source architecture enforcement — 2026-07-18

## Objective

Turn the cleaned directory tree into a build-enforced architecture rather than a naming convention.

## Changes

- Removed the ambiguous `src/compiler/` umbrella.
- Promoted shared compiler infrastructure to `src/core/`.
- Promoted canonical module graph ownership to `src/graph/`.
- Added `src/remote/` for vendoring, integrity, cache, and lock-file behavior that was incorrectly housed in the CLI.
- Moved JavaScript, JSX, and TypeScript frontend APIs beneath `src/frontends/`.
- Moved package/section plans and build-only services beneath `src/pipeline/`.
- Moved runtime management to `src/runtime/` and update management to `src/cli/`.
- Extracted shared CLI/build option models into `src/core/include/venom/core/options.hpp`.
- Extracted `JsChunk` and module-edge models into `src/graph/include/venom/graph/module_types.hpp`, removing the graph-to-pipeline dependency.
- Added independent CMake object targets for every compiled first-party domain.
- Added an automated source-domain include and cycle checker.
- Re-homed native tests from the obsolete `tests/compiler/` bucket into `tests/frontends/`, `tests/graph/`, and `tests/pipeline/`.

## Enforced invariants

- Non-CLI source cannot include the CLI surface.
- Frontends cannot include pipeline implementation headers.
- Graph code cannot include pipeline implementation headers.
- Cross-domain includes require an explicit allowlist entry.
- Authored source-domain dependencies must remain acyclic.
- Source inventory and CMake completeness checks include the modular CMake files.

## Validation

- Production `venom` executable built successfully.
- Native runtime and QuickJS runtime probes built successfully.
- Fixed the native package-runtime route-table parser to consume the canonical `VRTE0003` version `1` emitted by the pipeline and already used by the browser/WASM runtimes.
- Updated the runtime-probe harness for the hash-only `assets/app/<digest>.vbc` production layout.
- Added CTest coverage for native package execution and QuickJS native parity.
- Version output: `venom 2.0.0`.
- Final focused build, production-runtime, architecture, and ownership gate set passed 12/12.
- QuickJS native parity passed 6/6 checks.
- The native runtime executed all 1,255 instructions from the production fixture package successfully.
