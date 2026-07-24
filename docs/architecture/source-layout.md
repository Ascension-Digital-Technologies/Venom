# Repository and source layout

Venom uses a shallow repository root and explicit source-domain boundaries. A path should reveal whether a file is product source or support infrastructure, which subsystem owns it, and whether it is authored or generated.

## Repository root

```text
.github/       CI workflows
cmake/         CMake modules, source-domain targets, and configure templates
contracts/     Versioned machine-readable product contracts
docs/          Architecture, guides, operations, security, and audit records
examples/      Complete runnable applications
packages/      Optional published ecosystem packages
scripts/       Thin public launchers only
src/           First-party product source and authored generator inputs
tests/         Native, integration, browser, package, fixture, and fuzz tests
third_party/   Vendored upstream dependencies
tools/         Build, generation, architecture, verification, and release tools
```

## First-class source domains

```text
src/
├── base/         Dependency-free error and result foundations
├── cli/          Executable entry point and command adapters
├── core/         Shared options, diagnostics, project state, profiles, and processes
├── frontends/    JavaScript, JSX, and TypeScript parsing/transpilation
├── generated/    Generated contracts, embedded assets, snapshots, and metadata
├── graph/        Canonical module identity, dependency, and resolution model
├── package/      VBC format, crypto, compression, reader, and writer
├── pipeline/     Build orchestration, planning, rewriting, security, and packaging
├── turbojs/      Venom-owned TurboJS ABI, bridge, bytecode, and engine adapters
├── remote/       Remote source normalization, locking, caching, and acquisition
├── runtime/      Native/WASM runtime services and translation units
├── templates/    Authored non-native generator inputs
└── vm/           Route VM bytecode, encoding, and diversification
```

`base` is the only first-party domain allowed to depend on no other Venom domain. Shared error types and stable exit codes live there, while console-aware error rendering remains in `core`.

The former catch-all `src/compiler/` hierarchy no longer exists. Its internal folders were promoted or split by ownership:

- `compiler/core` became `core`.
- `compiler/graph` became `graph`.
- compiler planning moved into `pipeline/planning`.
- runtime, update, ABI, and module services moved to `runtime`, `cli`, or `pipeline`.
- command option models moved into `core/options.hpp`, so reusable code never includes the CLI parser surface.

## Colocated source and header layout

Each native domain keeps its headers beside the `.cpp` or `.c` files that implement them:

```text
src/<domain>/
├── feature.cpp
├── feature.hpp
├── internal_helper.cpp
└── internal_helper.hpp
```

Cross-domain includes are source-root relative, so ownership remains visible without a separate include tree:

```cpp
#include "package/reader.hpp"
#include "pipeline/build.hpp"
```

Generated C and C++ contracts follow the same rule under `src/generated/`. The configured version header is the only build-generated exception and is emitted beneath the build tree at `generated/core/version.hpp`.

## Build boundaries

`cmake/source_domains.cmake` still creates interface dependency targets and implementation object targets for every domain. The filesystem no longer separates exported and private headers; target dependencies and architecture checks enforce the intended domain graph while keeping related files together.

The architecture checks at `tools/architecture/check_domain_dependencies.py` and `tools/architecture/check_header_boundaries.py` reject a top-level `include/` tree, nested include folders, obsolete `venom/...` include spellings, and missing source-domain include configuration. See [dependency-rules.md](dependency-rules.md).

## Frontend ownership

All language frontend code is grouped by language:

```text
src/frontends/
├── javascript/
│   ├── frontend.cpp
│   └── embedded_bundles.cpp
├── jsx/
│   └── frontend.cpp
└── typescript/
    ├── frontend.cpp
    └── typescript_runtime.cpp
```

Parser APIs no longer live inside the build pipeline. The pipeline consumes frontend APIs as a higher-level orchestration layer.

## JavaScript ownership

JavaScript inside `src/` is never mixed with native implementation code:

- `src/templates/browser/` contains ordered authored browser-runtime modules.
- `src/templates/turbojs-engine/` contains ordered authored TurboJS wrapper modules.
- `src/templates/typescript/` contains the authored TypeScript bridge template.
- `src/generated/runtime/javascript/` contains checked-in generated JavaScript snapshots.
- `src/generated/runtime/metadata/` contains generated provenance metadata.

## Test ownership

Native tests mirror their owning domains where practical:

```text
tests/frontends/   Language frontend runtime tests
tests/graph/       Canonical graph tests
tests/pipeline/    Planning and build-pipeline tests
tests/package/     Package and broad static/integration gates
tests/turbojs/     TurboJS integration tests
tests/runtime/     Native/WASM runtime tests
tests/vm/          Route VM tests
```
