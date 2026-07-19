# Source tree ownership

The `src/` tree is divided into first-class product domains. Each native domain compiles independently through `cmake/source_domains.cmake`; `venom_core` is only the final object aggregation boundary.

```text
src/
├── base/                     Dependency-free error and result foundations
├── cli/                      Executable and command adapters
├── core/                     Shared options, diagnostics, project state, profiles, and processes
├── frontends/
│   ├── javascript/           JavaScript parser and embedded hardener assets
│   ├── jsx/                  JSX lowering frontend
│   └── typescript/           TypeScript frontend and embedded compiler runtime
├── generated/
│   ├── compiler/             Embedded TypeScript compiler and bridge payloads
│   ├── contracts/            Generated native contract headers
│   └── runtime/              Runtime snapshots, blobs, JavaScript, and metadata
├── graph/                    Canonical module graph and shared JS module models
├── package/                  VBC package format and storage primitives
├── pipeline/
│   └── planning/             Package and section construction plans
├── quickjs/                  QuickJS ABI, bytecode, bridge, and engine integration
├── remote/                   Remote source cache, lock, integrity, and acquisition
├── runtime/                  Native/WASM runtime services and probes
├── templates/                Authored JavaScript generator inputs
└── vm/                       Route VM bytecode, encoding, and diversification
```

## Placement rules

- `base/` is the lowest-level native domain. It may use the standard library but must not include another Venom source domain.
- Reusable code must not include `cli/`; shared command models belong in `core/options.hpp`.
- Frontends expose parsing and transpilation APIs and must not depend on pipeline implementation code.
- Graph code owns module identity/types and must not depend on pipeline implementation code.
- Pipeline code coordinates lower-level domains and owns build-specific planning.
- Authored `.js` belongs only under `src/templates/`.
- Generated `.js` and `.json` belong under explicitly named generated JavaScript or metadata folders.
- Venom QuickJS integration belongs under `src/quickjs/`; upstream QuickJS remains under `third_party/quickjs/`.
- New cross-domain includes must be added deliberately to the architecture policy and remain acyclic.

Run `python tools/architecture/check_domain_dependencies.py .` to verify the dependency boundaries.
