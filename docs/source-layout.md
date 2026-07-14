# Source layout

Venom separates stable public compiler interfaces, handwritten implementations, and generated runtime artifacts.

## Compiler interfaces

Compiler headers are grouped beside their implementation responsibilities: `commands`, `core`, `pipeline`, and `services`. Public includes use paths such as `compiler/pipeline/build.hpp` and `compiler/core/site.hpp`; headers must not be added directly to `src/compiler/`.

## Compiler implementations

- `src/compiler/commands/` — CLI-facing commands and diagnostics.
- `src/compiler/core/` — configuration, project, site, process, profile, and compatibility primitives.
- `src/compiler/pipeline/` — build orchestration, HTML/CSS/JavaScript processing, capability analysis, and security transforms.
- `src/compiler/services/` — runtime, update, and generated-module lifecycle services.


## Build-system templates

CMake input templates live under `cmake/templates/`, not under `src/`. The project version template is `cmake/templates/version.hpp.in`; CMake generates `compiler/core/version.hpp` beneath the build tree. Source-layout validation rejects `.in` templates under `src/`.

## Native and WebAssembly runtime sources

`src/runtime/` contains only executable runtime implementations, native probes, public runtime APIs, and the editable JavaScript runtime template:

- `package_runtime.c/.h` — native package validation and route/runtime analysis.
- `wasm_runtime.c` — browser package, route, and section runtime compiled to WebAssembly.
- `quickjs_upstream_runtime.c/.h` — native QuickJS parity and runtime probes.
- `quickjs_runtime_scaffold.c` — the generated/runtime ABI scaffold used by runtime tooling.
- `runtime_probe.c` and `quickjs_native_probe.c` — focused diagnostic executables.
- `templates/runtime.js` — auditable browser runtime source.

Empty host/loader initialization stubs are not allowed. Compiler-facing template declarations live under `src/compiler/services/`, and generated runtime contracts live under `src/generated/runtime/`.

## Generated runtime artifacts

`src/generated/runtime/` contains generated browser runtime modules and embedded WASM artifacts:

- `runtime_js.cpp`
- `worker_runtime_js.cpp`
- `browser_asset_runtime_js.cpp`
- `wasm_runtime_js.cpp`
- `quickjs_engine_module.cpp`
- `quickjs_runtime_wasm_blob.hpp`
- `wasm_runtime_blob.hpp`
- `wasm_runtime_provenance.json`

These files are generated or mechanically maintained and should not be mixed with handwritten compiler implementation files. Regenerate embedded blobs and provenance through the canonical build tools rather than editing them manually.

## Policy

The source-layout smoke test enforces size limits for handwritten translation units and allows larger generated runtime sources only under `src/generated/runtime/`.

generated blob files are never hand-edited; regenerate them through the canonical runtime build pipeline.

## Build pipeline support split

`src/compiler/pipeline/build.cpp` owns high-level package assembly. Reusable release-build support lives in `src/compiler/pipeline/build_support.cpp` behind the internal `src/compiler/pipeline/build_support.hpp` interface. This keeps runtime verification, WASM export rewriting, JavaScript hardening, file I/O, and route-shell emission out of the main orchestration unit without changing public include paths.

- `src/compiler/pipeline/js_planning.cpp` owns function-realm directives, extraction analysis, and bridge-plan reporting.

### JavaScript pipeline responsibilities

- `js_discovery.cpp` discovers scripts and module graphs.
- `js_planning.cpp` assigns execution realms and extraction plans.
- `js_rewriting.cpp` owns protected-module transformation and bridge extraction.
- `js.cpp` packages bytecode, diagnostics, plans, and loader output.

### Release security analysis

Release-check responsibilities are split between:

- `src/compiler/pipeline/security.cpp` for command orchestration, report rendering, and key-file operations.
- `src/compiler/pipeline/security_analysis.cpp` for runtime provenance checks and release-policy evaluation.
- `src/compiler/pipeline/security_artifact_inspection.cpp` for package discovery, byte-level inspection, asset binding/SRI helpers, and payload-layout analysis.
- `src/compiler/pipeline/security_analysis.hpp` for the internal report contract shared by those units.

This keeps the user-facing command surface small while allowing the large package-analysis path to be compiled and tested independently.
