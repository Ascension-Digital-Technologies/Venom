# Repository and source layout

Venom uses a shallow repository root and explicit source-domain boundaries. A path should reveal whether a file is product source or support infrastructure, which subsystem owns it, and whether it is authored or generated.

## Repository root

```text
.github/       CI workflows
cmake/         CMake modules, source-domain targets, and configure templates
contracts/     Versioned machine-readable product contracts
docs/          Architecture, guides, operations, security, and audit records
examples/      Complete runnable applications
include/       Central public and internal C/C++ header tree
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
├── quickjs/      Venom-owned QuickJS ABI, bridge, bytecode, and engine adapters
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

## Public and internal header boundaries

Every first-party C/C++ header lives in one central include tree. Stable cross-domain APIs remain grouped by owning domain, while implementation-only surfaces are visibly separated beneath `internal/`:

```text
include/venom/
├── <domain>/                 Stable cross-domain API headers
├── generated/               Generated contracts and embedded public assets
└── internal/
    └── <domain>/             Implementation-only headers

src/<domain>/
└── *.c / *.cpp               Implementation translation units only
```

Public cross-domain includes use the stable namespace form:

```cpp
#include <venom/package/reader.hpp>
#include <venom/pipeline/build.hpp>
```

Implementation files include their own private surfaces explicitly:

```cpp
#include <venom/internal/pipeline/build_support.hpp>
```

Public headers may never include `venom/internal/...`, and one domain may never include another domain's internal headers. The architecture gates enforce both rules, while also rejecting any first-party header that drifts back into `src/`.

## Build boundaries

`cmake/source_domains.cmake` creates two targets for each compiled first-party domain: an interface API target exposing the central include root plus declared API dependencies, and a private object target containing the implementation. `venom_core` aggregates the object targets and publishes the deliberate API targets; no target receives a first-party `src/` include path. Internal-header ownership is enforced by the architecture checks rather than by source-directory visibility.

The architecture checks at `tools/architecture/check_domain_dependencies.py` and `tools/architecture/check_header_boundaries.py` enforce allowed cross-domain includes, public-header resolution, private-header isolation, removal of legacy `domain/header.hpp` include spellings, and authored dependency acyclicity. See [dependency-rules.md](dependency-rules.md).

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
- `src/templates/quickjs-engine/` contains ordered authored QuickJS wrapper modules.
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
tests/quickjs/     QuickJS integration tests
tests/runtime/     Native/WASM runtime tests
tests/vm/          Route VM tests
```
