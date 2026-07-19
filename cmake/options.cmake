option(VENOM_BUILD_TESTS "Build and register Venom test targets" ON)
option(VENOM_BUILD_RUNTIME_NATIVE "Build the native C package runtime and diagnostic probes" ON)
option(VENOM_ENABLE_STRICT "Enable stricter compiler warnings for Venom source" ON)
option(VENOM_ENABLE_WERROR "Treat Venom warnings as errors" OFF)

option(VENOM_ENABLE_FULL_QUICKJS "Compile the full vendored QuickJS engine required for production bytecode generation" ON)
option(VENOM_BUILD_COMPILER_ASSET_TOOLS "Build internal generators for pinned compiler assets" OFF)
option(VENOM_ENABLE_UPSTREAM_QJS_WASM "Enable upstream QuickJS when compiling src/runtime/quickjs_runtime_scaffold.c with an external WASM toolchain" OFF)

option(VENOM_BUILD_FUZZERS "Build libFuzzer and deterministic corpus replay targets" OFF)
option(VENOM_ENABLE_SANITIZERS "Enable ASan and UBSan for supported compilers" OFF)

option(VENOM_ENABLE_IPO "Enable interprocedural optimization/LTO for release builds" ON)
set(VENOM_PGO_MODE "none" CACHE STRING "Profile-guided optimization mode: none, generate, or use")
set_property(CACHE VENOM_PGO_MODE PROPERTY STRINGS none generate use)
set(VENOM_PGO_DIR "${CMAKE_BINARY_DIR}/pgo" CACHE PATH "Directory containing compiler profile data")
option(VENOM_REQUIRE_BROWSER_RUNTIME_TESTS
  "Require the verified embedded QuickJS/WASM runtime and make browser-runtime tests mandatory"
  OFF)

option(VENOM_ENABLE_NODE_ECOSYSTEM_TESTS
  "Enable optional Node.js, npm, and Vite integration tests"
  OFF)
