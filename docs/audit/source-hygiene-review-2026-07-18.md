# Source hygiene and repository structure review — 2026-07-18

## Scope

This pass reviewed the complete repository tree, source ownership, generator inputs and outputs, CMake paths, public launcher naming, platform parity, tests, documentation, and build behavior.

## Main findings

1. The repository root separated Vite integration code from other published packages and separated fuzz targets from their corpora and tests.
2. `src/` mixed native engine code, native runtime translation units, authored JavaScript modules, generated JavaScript snapshots, and generated JSON metadata without sufficiently explicit ownership boundaries.
3. The initial cleanup grouped TurboJS and the route VM under a generic engine folder, but both had sufficient independent ownership to be promoted again as explicit source domains.
4. Compiler package planning lived at `src/compiler/package/`, which was easy to confuse with the actual VBC package implementation at `src/package/`.
5. TypeScript and runtime generation inputs were split between `tools/compiler-assets/`, `tools/templates/`, loose generator scripts, and `src/runtime/js/`.
6. Public example launchers used sequence numbers, Windows and Linux did not expose the same example set, and documentation encoded those unstable numbers.
7. Existing structure tests encoded the old layout rather than enforcing clear placement rules for future files.

## Implemented structure

### Root consolidation

- Merged `integrations/vite/` into `packages/vite/`.
- Moved native fuzz targets from `fuzz/` to `tests/fuzz/targets/` beside `tests/fuzz/corpus/`.
- Reduced the root directory surface from thirteen directories to eleven, including `.github`.

### Native source ownership

- Promoted CLI command handling and the executable entry point to `src/cli/`.
- Promoted the large compiler build lifecycle to `src/pipeline/`.
- Promoted language integrations to `src/frontends/`.
- Promoted Venom-owned TurboJS integration to `src/turbojs/`.
- Promoted the route VM to `src/vm/`.
- Retained smaller compiler-only internals under `src/compiler/{core,graph,planning,services}`.
- Flattened native and WASM runtime translation units directly into `src/runtime/`.
- Established ten explicit source domains: `cli`, `compiler`, `frontends`, `generated`, `package`, `pipeline`, `turbojs`, `runtime`, `templates`, and `vm`.

### JavaScript and generated artifacts

- Centralized authored browser runtime modules at `src/templates/browser/`.
- Centralized authored TurboJS wrapper modules at `src/templates/turbojs-engine/`.
- Centralized the authored TypeScript bridge at `src/templates/typescript/bridge.js`.
- Moved generated browser runtime and ABI JavaScript to `src/generated/runtime/javascript/`.
- Moved runtime provenance JSON to `src/generated/runtime/metadata/`.
- Grouped runtime generators under `tools/generators/runtime/` and TypeScript generators under `tools/generators/typescript/`.

### Semantic example launchers

The numbered launcher surface was replaced with stable names on both Windows and Linux/macOS:

- `build-and-launch-chess`
- `build-and-launch-nova-trade`
- `build-and-launch-bot-detection`
- `build-and-launch-typescript-showcase`
- `build-and-launch-tsx-showcase`
- `build-and-launch-aegis-operations`
- `build-and-launch-javascript-playground`
- `build-and-launch-turbojs-benchmark`
- `build-and-launch-chrome-extension`

All wrappers delegate to `tools/launch_example.py`. Numeric identifiers are no longer part of the launcher interface.

## Enforcement added

- `tests/package/source-layout-smoke.py` now enforces the new source domains, prevents legacy directories from returning, and rejects non-native source outside owned template/generated locations.
- `tools/check_source_inventory.py` now inventories fuzz targets from `tests/fuzz/targets/`.
- Build-script and repository-consistency tests now enforce semantic names, platform parity, root consolidation, and the new Vite/fuzz locations.
- Runtime bundle drift tests were updated and the checked-in browser runtime snapshot was regenerated from authored modules.

## Validation

- CMake configured successfully with the reorganized paths.
- The `venom` production executable built successfully with GCC after disabling IPO for validation speed.
- Source layout, source inventory, CMake source completeness, build-script naming, repository consistency, runtime module ownership, runtime bundle drift, TurboJS module generation, TypeScript frontend integration, and Vite package integration checks passed.
- The focused CTest structural suite passed 6/6 tests.

Several broader static tests still report pre-existing product/runtime expectation drift unrelated to this structural pass, including missing old release-closure helper files and generated runtime security markers. Those failures were not caused by the moved source paths and were intentionally not masked by this cleanup.
