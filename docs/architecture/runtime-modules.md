# Runtime Module Architecture

Venom authors its browser runtime and TurboJS engine wrapper as ordered JavaScript modules. The final distribution still contains compact single-file runtime artifacts, but those artifacts are generated rather than maintained as monolithic source files.

## Browser runtime

`src/templates/browser/` owns package loading, runtime contracts, host queues, protected bridge orchestration, browser module linking, and route execution. `tools/generators/runtime/bundle_js_modules.py` concatenates the ordered modules and CMake embeds the resulting bundle.

## TurboJS engine wrapper

`src/templates/turbojs-engine/` owns the WASM contract, memory access, context lifecycle, bytecode execution, module linking, and diagnostic surface. `tools/generators/runtime/generate_turbojs_engine_module.py` generates the C++ wrapper compiled into Venom.

## Generated compatibility copies

`src/generated/runtime/javascript/browser_runtime.js` and `src/generated/runtime/turbojs_engine_module.cpp` remain checked in for source-inspection tests and source releases. Dedicated drift tests require them to match the authored modules exactly.
