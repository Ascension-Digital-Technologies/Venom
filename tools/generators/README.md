# Source generators

This directory contains deterministic source-generation utilities grouped by the artifact they produce.

- `runtime/` bundles authored runtime JavaScript and emits native C++ wrappers.
- `typescript/` embeds the vendored TypeScript compiler and its verified bridge bytecode.

Generator inputs live in `src/**/templates/` or `third_party/`; generated implementation snapshots live in `src/generated/`, while generated C/C++ contracts are exported from `src/generated/include/venom/generated/`.
