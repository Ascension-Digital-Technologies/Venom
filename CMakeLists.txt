cmake_minimum_required(VERSION 3.20)

project(venom
  VERSION 2.0.0
  DESCRIPTION "Venom - Secure Web Runtime: protected web compiler and dual-VM browser runtime"
  LANGUAGES C CXX
)

# Stable public product version.
set(VENOM_VERSION_FULL "${PROJECT_VERSION}")

include(cmake/options.cmake)
include(cmake/warnings.cmake)
include(cmake/platforms.cmake)
include(cmake/build_acceleration.cmake)
include(CheckCSourceCompiles)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
  # Windows uses ACLs rather than POSIX modes and the UCRT deprecates standard
  # portable C APIs. Venom validates their inputs and keeps cross-platform
  # implementations, so avoid toolchain-only deprecation noise.
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
endif()

# One authoritative project version for C++, scripts, and release metadata.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/core")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/include/venom/core/version.hpp
  @ONLY
)

# Visual Studio is a multi-config generator. The build scripts intentionally
# select Release/Debug with `cmake --build --config`, so acknowledge an
# optionally supplied CMAKE_BUILD_TYPE instead of leaving a noisy warning.
if(CMAKE_CONFIGURATION_TYPES AND DEFINED CMAKE_BUILD_TYPE)
  message(STATUS "Multi-config generator detected; build configuration is selected with --config ${CMAKE_BUILD_TYPE}")
endif()

find_package(Threads REQUIRED)
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
venom_configure_compiler_cache()

set(VENOM_GENERATED_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/runtime")
set(VENOM_RUNTIME_BUNDLE_JS "${VENOM_GENERATED_RUNTIME_DIR}/runtime.bundle.js")
set(VENOM_RUNTIME_TEMPLATE_CPP "${VENOM_GENERATED_RUNTIME_DIR}/runtime_js_template.cpp")
set(VENOM_QUICKJS_ENGINE_CPP "${VENOM_GENERATED_RUNTIME_DIR}/quickjs_engine_module.cpp")
file(GLOB VENOM_BROWSER_RUNTIME_MODULES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/templates/browser/*.js")
file(GLOB VENOM_QUICKJS_ENGINE_MODULES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/templates/quickjs-engine/*.js")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # These files intentionally embed generated JavaScript bundles as C++ string
  # data. The strings are valid and tested; the ISO minimum translation-limit
  # warning is not actionable for supported compilers.
  set_source_files_properties(
    src/pipeline/native_js_hardener.cpp
    src/frontends/javascript/embedded_bundles.cpp
    src/frontends/javascript/frontend.cpp
    "${VENOM_QUICKJS_ENGINE_CPP}"
    PROPERTIES COMPILE_OPTIONS "-Wno-overlength-strings"
  )
  set_source_files_properties(
    "${VENOM_RUNTIME_TEMPLATE_CPP}"
    PROPERTIES COMPILE_OPTIONS "-Wno-overlength-strings"
  )
endif()
add_custom_command(
  OUTPUT "${VENOM_RUNTIME_BUNDLE_JS}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/bundle_js_modules.py"
          --input-dir "${CMAKE_CURRENT_SOURCE_DIR}/src/templates/browser"
          --output "${VENOM_RUNTIME_BUNDLE_JS}"
  DEPENDS ${VENOM_BROWSER_RUNTIME_MODULES}
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/bundle_js_modules.py"
  COMMENT "Bundling authored browser runtime modules"
  VERBATIM
)
add_custom_command(
  OUTPUT "${VENOM_RUNTIME_TEMPLATE_CPP}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/embed_runtime_template.py"
          --input "${VENOM_RUNTIME_BUNDLE_JS}"
          --output "${VENOM_RUNTIME_TEMPLATE_CPP}"
  DEPENDS "${VENOM_RUNTIME_BUNDLE_JS}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/embed_runtime_template.py"
  COMMENT "Embedding browser runtime bundle"
  VERBATIM
)
add_custom_command(
  OUTPUT "${VENOM_QUICKJS_ENGINE_CPP}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/generate_quickjs_engine_module.py"
          --input-dir "${CMAKE_CURRENT_SOURCE_DIR}/src/templates/quickjs-engine"
          --prefix "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/templates/quickjs_engine_module.prefix.cpp"
          --suffix "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/templates/quickjs_engine_module.suffix.cpp"
          --output "${VENOM_QUICKJS_ENGINE_CPP}"
  DEPENDS ${VENOM_QUICKJS_ENGINE_MODULES}
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/generate_quickjs_engine_module.py"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/templates/quickjs_engine_module.prefix.cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/generators/runtime/templates/quickjs_engine_module.suffix.cpp"
  COMMENT "Generating QuickJS browser-engine module"
  VERBATIM
)

# Phase 11: release optimization and profile-guided optimization.
include(CheckIPOSupported)
if(VENOM_ENABLE_IPO)
  check_ipo_supported(RESULT VENOM_IPO_SUPPORTED OUTPUT VENOM_IPO_ERROR)
  if(VENOM_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  else()
    message(WARNING "IPO/LTO unavailable: ${VENOM_IPO_ERROR}")
  endif()
endif()

string(TOLOWER "${VENOM_PGO_MODE}" VENOM_PGO_MODE_NORMALIZED)
if(NOT VENOM_PGO_MODE_NORMALIZED MATCHES "^(none|generate|use)$")
  message(FATAL_ERROR "VENOM_PGO_MODE must be none, generate, or use")
endif()
function(venom_apply_pgo target)
  if(VENOM_PGO_MODE_NORMALIZED STREQUAL "generate")
    file(MAKE_DIRECTORY "${VENOM_PGO_DIR}")
    if(MSVC)
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/GL>)
      target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/LTCG:PGINSTRUMENT> $<$<CONFIG:Release>:/GENPROFILE>)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-fprofile-generate=${VENOM_PGO_DIR}>)
      target_link_options(${target} PRIVATE $<$<CONFIG:Release>:-fprofile-generate=${VENOM_PGO_DIR}>)
    endif()
  elseif(VENOM_PGO_MODE_NORMALIZED STREQUAL "use")
    if(MSVC)
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/GL>)
      target_link_options(${target} PRIVATE $<$<CONFIG:Release>:/LTCG:PGOPTIMIZE>)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-fprofile-use=${VENOM_PGO_DIR}> $<$<CONFIG:Release>:-fprofile-correction>)
      target_link_options(${target} PRIVATE $<$<CONFIG:Release>:-fprofile-use=${VENOM_PGO_DIR}> $<$<CONFIG:Release>:-fprofile-correction>)
    endif()
  endif()
endfunction()

if(VENOM_ENABLE_FULL_QUICKJS)
  add_library(qjs STATIC
    third_party/quickjs/dtoa.c
    third_party/quickjs/libregexp.c
    third_party/quickjs/libunicode.c
    third_party/quickjs/quickjs.c
    third_party/quickjs/quickjs-libc.c
  )
  target_include_directories(qjs PUBLIC third_party/quickjs)
  set_target_properties(qjs PROPERTIES C_EXTENSIONS ON)
  target_compile_definitions(qjs PRIVATE QUICKJS_NG_BUILD _GNU_SOURCE)
  if(WIN32)
    target_compile_definitions(qjs PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601 _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
  endif()
  target_link_libraries(qjs PUBLIC Threads::Threads)
  if(NOT VENOM_ENABLE_QUICKJS_IPO)
    set_property(TARGET qjs PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  endif()
  venom_apply_build_acceleration(qjs)
  if(UNIX AND NOT APPLE)
    target_link_libraries(qjs PUBLIC dl m)
  elseif(APPLE)
    target_link_libraries(qjs PUBLIC m)
  endif()
  if(VENOM_BUILD_COMPILER_ASSET_TOOLS)
    add_executable(venom_compile_typescript_bridge_bytecode
      tools/generators/typescript/compile_typescript_bridge_bytecode.cpp)
    target_link_libraries(venom_compile_typescript_bridge_bytecode PRIVATE qjs)
    target_include_directories(venom_compile_typescript_bridge_bytecode PRIVATE third_party/quickjs)
  endif()
  if(MSVC)
    # QuickJS uses C11 `_Atomic` and <stdatomic.h> for SharedArrayBuffer and
    # worker reference counts. MSVC keeps C11 atomics behind an explicit
    # compiler switch even when /std:c11 is selected. Probe the exact feature
    # here so unsupported toolchains fail during configure with a useful error.
    set(_venom_saved_required_flags "${CMAKE_REQUIRED_FLAGS}")
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} /std:c11 /experimental:c11atomics")
    check_c_source_compiles([[
      #include <stdatomic.h>
      int main(void) {
        _Atomic unsigned int value = 0;
        return (int)atomic_fetch_add(&value, 1u);
      }
    ]] VENOM_MSVC_C11_ATOMICS_AVAILABLE)
    set(CMAKE_REQUIRED_FLAGS "${_venom_saved_required_flags}")
    unset(_venom_saved_required_flags)

    if(NOT VENOM_MSVC_C11_ATOMICS_AVAILABLE)
      message(FATAL_ERROR
        "The selected MSVC toolset cannot compile C11 atomics required by QuickJS. "
        "Install Visual Studio 2022 17.5 or newer (including Visual Studio 2026), "
        "or configure with clang-cl.")
    endif()

    target_compile_options(qjs PRIVATE
      "$<$<COMPILE_LANGUAGE:C>:/std:c11>"
      "$<$<COMPILE_LANGUAGE:C>:/experimental:c11atomics>"
      "$<$<COMPILE_LANGUAGE:C>:/J>"
      /wd4244 /wd4267 /wd4996 /wd4018 /wd4100 /wd4200
    )
  else()
    target_compile_options(qjs PRIVATE -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-missing-field-initializers -Wno-implicit-fallthrough)
  endif()
else()
  add_library(qjs INTERFACE)
  target_include_directories(qjs INTERFACE third_party/quickjs)
endif()

include(cmake/source_domains.cmake)


add_executable(venom src/cli/main.cpp)
target_link_libraries(venom PRIVATE venom_core)

# The embedded Terser/javascript-obfuscator pipeline executes inside QuickJS and
# legitimately needs more native call-stack headroom than the Windows linker
# default. JS_SetMaxStackSize() only establishes QuickJS's guard threshold; it
# cannot enlarge the operating-system thread stack. Keep the PE stack reserve
# above the 8 MiB QuickJS limit so QuickJS can fail safely before Windows raises
# STATUS_STACK_OVERFLOW (0xC00000FD).
if(MSVC)
  target_link_options(venom PRIVATE "/STACK:16777216,1048576")
endif()

venom_apply_warnings(venom)
venom_apply_build_acceleration(venom)
venom_apply_pgo(venom)


if(VENOM_BUILD_RUNTIME_NATIVE)
  add_library(venom_runtime_native STATIC
    src/runtime/package_runtime.c
    src/runtime/quickjs_upstream_runtime.c
  )
  target_include_directories(venom_runtime_native
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE src/runtime
  )
  target_link_libraries(venom_runtime_native PUBLIC qjs ${CMAKE_DL_LIBS})
  venom_apply_warnings(venom_runtime_native)
  venom_apply_build_acceleration(venom_runtime_native)
  venom_apply_pgo(venom_runtime_native)
  if(VENOM_ENABLE_FULL_QUICKJS)
    target_compile_definitions(venom_runtime_native PRIVATE VENOM_ENABLE_FULL_QUICKJS)
  endif()

  add_executable(venom_runtime_probe src/runtime/runtime_probe.c)
  target_link_libraries(venom_runtime_probe PRIVATE venom_runtime_native)
  venom_apply_warnings(venom_runtime_probe)

  add_executable(venom_qjs_native_probe src/runtime/quickjs_native_probe.c)
  target_link_libraries(venom_qjs_native_probe PRIVATE venom_runtime_native)
  venom_apply_warnings(venom_qjs_native_probe)
endif()


if(VENOM_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  foreach(_venom_domain_target IN LISTS VENOM_DOMAIN_TARGETS)
    target_compile_options(${_venom_domain_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  endforeach()
  target_link_options(venom PUBLIC -fsanitize=address,undefined)
endif()

if(VENOM_BUILD_FUZZERS)
  add_executable(venom_package_parser_replay tests/fuzz/targets/package_parser_replay.cpp)
  target_link_libraries(venom_package_parser_replay PRIVATE venom_core)
  venom_apply_warnings(venom_package_parser_replay)

  add_executable(venom_runtime_contract_replay
    tests/fuzz/targets/runtime_contract_replay.cpp
    tests/fuzz/targets/runtime_contract_fuzz.cpp)
  venom_apply_warnings(venom_runtime_contract_replay)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_executable(venom_package_parser_fuzz tests/fuzz/targets/package_parser_fuzz.cpp)
    add_executable(venom_runtime_contract_fuzz tests/fuzz/targets/runtime_contract_fuzz.cpp)
    add_executable(venom_package_parser_differential_fuzz tests/fuzz/targets/package_parser_differential.cpp)
    foreach(_venom_fuzzer_target
        venom_package_parser_fuzz
        venom_runtime_contract_fuzz
        venom_package_parser_differential_fuzz)
      target_compile_options(${_venom_fuzzer_target} PRIVATE -fsanitize=fuzzer -fno-omit-frame-pointer)
      target_link_options(${_venom_fuzzer_target} PRIVATE -fsanitize=fuzzer)
      if(VENOM_ENABLE_SANITIZERS)
        target_compile_options(${_venom_fuzzer_target} PRIVATE -fsanitize=address,undefined)
        target_link_options(${_venom_fuzzer_target} PRIVATE -fsanitize=address,undefined)
      endif()
    endforeach()
    target_link_libraries(venom_package_parser_fuzz PRIVATE venom_core)
    target_link_libraries(venom_package_parser_differential_fuzz PRIVATE venom_core)
  else()
    message(WARNING "VENOM_BUILD_FUZZERS requested without Clang; building deterministic replay targets only")
  endif()
endif()

if(VENOM_BUILD_TESTS)
  include(cmake/tests.cmake)

if(BUILD_TESTING AND VENOM_ENABLE_FULL_QUICKJS)

  add_executable(venom_typescript_embedded_smoke tests/frontends/typescript-embedded-smoke.cpp)
  target_link_libraries(venom_typescript_embedded_smoke PRIVATE venom_core)
  venom_apply_warnings(venom_typescript_embedded_smoke)
  add_test(NAME typescript_embedded_smoke COMMAND venom_typescript_embedded_smoke)

  add_executable(venom_quickjs_module_bundle_runtime
    tests/quickjs/module-bundle-runtime.cpp
    src/runtime/quickjs_runtime_scaffold.c)
  target_compile_definitions(venom_quickjs_module_bundle_runtime PRIVATE VENOM_ENABLE_UPSTREAM_QJS_WASM)
  target_link_libraries(venom_quickjs_module_bundle_runtime PRIVATE venom_core qjs ${CMAKE_DL_LIBS})
  add_test(NAME quickjs_module_bundle_runtime COMMAND venom_quickjs_module_bundle_runtime)

  add_executable(venom_quickjs_bridge_abi_runtime
    tests/quickjs/bridge-abi-runtime.cpp
    src/runtime/quickjs_runtime_scaffold.c)
  target_compile_definitions(venom_quickjs_bridge_abi_runtime PRIVATE VENOM_ENABLE_UPSTREAM_QJS_WASM)
  target_link_libraries(venom_quickjs_bridge_abi_runtime PRIVATE venom_core qjs ${CMAKE_DL_LIBS})
  add_test(NAME quickjs_bridge_abi_runtime COMMAND venom_quickjs_bridge_abi_runtime)
endif()

if(BUILD_TESTING)
  add_executable(venom_route_browser_module_map_test tests/package/route-browser-module-map.cpp)
  target_link_libraries(venom_route_browser_module_map_test PRIVATE venom_core)
  venom_apply_warnings(venom_route_browser_module_map_test)
  add_test(NAME route_browser_module_map_smoke COMMAND venom_route_browser_module_map_test)
  add_executable(venom_canonical_module_graph_test tests/graph/canonical-module-graph.cpp)
  target_link_libraries(venom_canonical_module_graph_test PRIVATE venom_core)
  venom_apply_warnings(venom_canonical_module_graph_test)
  add_test(NAME canonical_module_graph_smoke COMMAND venom_canonical_module_graph_test)
  add_executable(venom_chrome_extension_manifest_analysis_test tests/pipeline/chrome-extension-manifest-analysis.cpp)
  target_link_libraries(venom_chrome_extension_manifest_analysis_test PRIVATE venom_core)
  venom_apply_warnings(venom_chrome_extension_manifest_analysis_test)
  add_test(NAME chrome_extension_manifest_analysis_smoke COMMAND venom_chrome_extension_manifest_analysis_test)
  add_executable(venom_chrome_extension_compatibility_matrix_test tests/pipeline/chrome-extension-compatibility-matrix.cpp)
  target_link_libraries(venom_chrome_extension_compatibility_matrix_test PRIVATE venom_core)
  target_compile_definitions(venom_chrome_extension_compatibility_matrix_test PRIVATE VENOM_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
  venom_apply_warnings(venom_chrome_extension_compatibility_matrix_test)
  add_test(NAME chrome_extension_compatibility_matrix_smoke COMMAND venom_chrome_extension_compatibility_matrix_test)
  add_test(NAME chrome_extension_rpc_generation_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/chrome-extension-rpc-generation-smoke.py)
  add_test(NAME chrome_extension_compatibility_matrix_source_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/chrome-extension-compatibility-matrix-smoke.py)
  add_executable(venom_package_plan_test tests/pipeline/package-plan.cpp)
  target_link_libraries(venom_package_plan_test PRIVATE venom_core)
  venom_apply_warnings(venom_package_plan_test)
  add_test(NAME package_plan_smoke COMMAND venom_package_plan_test)

  add_test(NAME empty_redacted_asset_name_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/empty-redacted-asset-name-smoke.py)
  add_test(NAME cli_project_runtime_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/cli-project-runtime-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME dev_server_incremental_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/dev-server-incremental-smoke.py)
  add_test(NAME protected_module_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/protected-module-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME protected_contract_function_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/protected-contract-function-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME quickjs_runtime_lifecycle_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-runtime-lifecycle-smoke.py
            ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/quickjs-runtime-lifecycle-smoke)
  add_test(NAME cli_clean_commands_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/cli-clean-commands-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME javascript_ast_frontend_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/javascript-ast-frontend-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME javascript_ast_scope_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/javascript-ast-scope-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME javascript_ast_planner_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/javascript-ast-planner-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME javascript_ast_module_closure_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/javascript-ast-module-closure-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME html_script_type_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/html-script-type-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME capability_policy_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/capability-policy-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME typescript_input_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/typescript-input-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME typescript_showcase_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/typescript-showcase-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME typescript_frontend_integration_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/typescript-frontend-integration-smoke.py)
  add_test(NAME incremental_build_cache_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/incremental-build-cache-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME generated_runtime_no_touch_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/generated-runtime-no-touch-smoke.py)
  if(VENOM_ENABLE_NODE_ECOSYSTEM_TESTS)
    add_test(NAME vite_plugin_integration_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/vite-plugin-integration-smoke.py)
  endif()
  add_test(NAME framework_vite_fixtures_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/framework-vite-fixtures-smoke.py)
  add_test(NAME tsx_showcase_production_verify_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/tsx-showcase-production-verify-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME aegis_operations_production_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/aegis-operations-production-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME javascript_playground_quickjs_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/javascript-playground-quickjs-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME quickjs_benchmark_example_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-benchmark-example-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME quickjs_wasm_authoritative_contract_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-wasm-authoritative-contract-smoke.py)
  add_test(NAME quickjs_engine_generated_abi_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-engine-generated-abi-smoke.py)
  add_test(NAME javascript_playground_ephemeral_context_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/javascript-playground-ephemeral-context-smoke.py)
  add_test(NAME production_only_profile_boundary_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime/production-only-profile-boundary-smoke.py)
  add_test(NAME binary_protected_values_integration_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/binary-protected-values-integration-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME runtime_performance_budget_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/runtime-performance-budget-smoke.py
            ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME protected_runtime_assurance_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/protected-runtime-assurance-smoke.py)
  add_test(NAME protected_bridge_limits_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/protected-bridge-limits-smoke.py)
  add_test(NAME bridge_frame_mutation_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/bridge-frame-mutation-smoke.py)
  add_test(NAME bot_detection_protection_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/bot-detection-protection-smoke.py)
endif()

add_test(NAME venom_compatibility_report_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/compatibility-report-smoke.py)

# Verified minimal browser QuickJS/WASM source-set regression.
add_test(NAME venom_emscripten_minimal_quickjs_source_set COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime/emscripten-minimal-quickjs-source-set.py)

add_test(NAME venom_binary_protected_values_smoke
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/package/binary-protected-values-smoke.py")

add_test(NAME venom_diagnostics_explain_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/diagnostics-explain-smoke.py)

add_test(NAME venom_language_server_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/language-server-smoke.py)
add_test(NAME venom_module_query_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/module-query-smoke.py)

add_test(NAME venom_runtime_api_contract_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/runtime-api-contract-smoke.py)
add_test(NAME venom_abi_contract_lock
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/verify_contract_lock.py --repo-root ${CMAKE_SOURCE_DIR})
add_test(NAME venom_release_archive_safety_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/release-archive-safety-smoke.py)
add_test(NAME venom_generated_runtime_client_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/generated-runtime-client-smoke.py $<TARGET_FILE:venom>)
add_test(NAME venom_cxx17_portability_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/cxx17-portability-smoke.py)

if(VENOM_ENABLE_NODE_ECOSYSTEM_TESTS)
  find_program(VENOM_NODE_EXECUTABLE node REQUIRED)
  add_test(NAME venom_runtime_sdk_lifecycle_smoke
    COMMAND ${VENOM_NODE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/runtime-sdk-lifecycle-smoke.mjs)
endif()

add_test(NAME venom_quickjs_release_export_allowlist_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/quickjs-release-export-allowlist-smoke.py)

add_test(NAME venom_protected_facade_browser_dependency_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/protected-facade-browser-dependency-smoke.py)

add_executable(venom_lazy_browser_dependency_closure_test
  tests/package/lazy-browser-dependency-closure.cpp)
target_link_libraries(venom_lazy_browser_dependency_closure_test PRIVATE venom_core)
add_test(NAME venom_lazy_browser_dependency_closure
  COMMAND venom_lazy_browser_dependency_closure_test)

add_executable(venom_protected_browser_import_lowering_test
  tests/pipeline/protected-browser-import-lowering.cpp)
target_link_libraries(venom_protected_browser_import_lowering_test PRIVATE venom_core)
add_test(NAME venom_protected_browser_import_lowering
  COMMAND venom_protected_browser_import_lowering_test)

add_test(NAME venom_example_certification_contract_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/example-certification-contract-smoke.py)

add_test(NAME venom_browser_certification_v2_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/browser-certification-v2-smoke.py)

add_test(NAME venom_runtime_startup_observability_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/runtime-startup-observability-smoke.py)

add_test(NAME venom_runtime_startup_performance_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/runtime-startup-performance-smoke.py)

add_test(NAME venom_final_readiness_report_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/final-readiness-report-smoke.py)
endif() # VENOM_BUILD_TESTS
