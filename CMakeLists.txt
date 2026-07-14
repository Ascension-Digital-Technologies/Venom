cmake_minimum_required(VERSION 3.20)

project(venom
  VERSION 1.0.0
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

# One authoritative project version for C++, scripts, and release metadata.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/compiler/core")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/version.hpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/compiler/core/version.hpp
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
set(VENOM_RUNTIME_TEMPLATE_CPP "${VENOM_GENERATED_RUNTIME_DIR}/runtime_js_template.cpp")
add_custom_command(
  OUTPUT "${VENOM_RUNTIME_TEMPLATE_CPP}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_runtime_template.py"
          --input "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/templates/runtime.js"
          --output "${VENOM_RUNTIME_TEMPLATE_CPP}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/templates/runtime.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_runtime_template.py"
  COMMENT "Embedding browser runtime template"
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

set(VENOM_CORE_SOURCES
  src/compiler/commands/cli.cpp
  src/compiler/core/config.cpp
  src/compiler/core/project.cpp
  src/compiler/services/runtime_manager.cpp
  src/compiler/services/update_manager.cpp
  src/compiler/commands/doctor.cpp
  src/compiler/commands/dev.cpp
  src/compiler/core/compatibility.cpp
  src/compiler/core/planner.cpp
  src/compiler/commands/dist_analyzer.cpp
  src/compiler/pipeline/capability_analysis.cpp
  src/compiler/pipeline/build.cpp
  src/compiler/pipeline/build_support.cpp
  src/compiler/pipeline/native_js_hardener.cpp
  src/compiler/pipeline/build_package_metadata.cpp
  src/compiler/pipeline/build_runtime_metadata.cpp
  src/compiler/pipeline/build_runtime_audit_metadata.cpp
  src/compiler/pipeline/build_runtime_module_metadata.cpp
  src/compiler/commands/inspect.cpp
  src/compiler/pipeline/security.cpp
  src/compiler/pipeline/security_analysis.cpp
  src/compiler/pipeline/security_artifact_inspection.cpp
  src/compiler/core/site.cpp
  src/compiler/pipeline/assets.cpp
  src/compiler/pipeline/html.cpp
  src/compiler/pipeline/css.cpp
  src/compiler/pipeline/js.cpp
  src/compiler/pipeline/js_discovery.cpp
  src/compiler/pipeline/js_planning.cpp
  src/compiler/pipeline/js_rewriting.cpp
  src/compiler/pipeline/function_dependencies.cpp
  src/generated/runtime/worker_runtime_js.cpp
  src/generated/runtime/browser_asset_runtime_js.cpp
  src/generated/runtime/runtime_js.cpp
  ${VENOM_RUNTIME_TEMPLATE_CPP}
  src/compiler/services/runtime_modules.cpp
  src/generated/runtime/wasm_runtime_js.cpp
  src/generated/runtime/quickjs_engine_module.cpp
  src/compiler/core/profile.cpp
  src/compiler/core/process.cpp
  src/compiler/commands/remote.cpp
  src/package/format.cpp
  src/package/writer.cpp
  src/package/reader.cpp
  src/package/crypto.cpp
  src/package/compress.cpp
  src/package/hash.cpp
  src/vm/opcode.cpp
  src/vm/encoder.cpp
  src/vm/polymorph.cpp
  src/quickjs/abi.cpp
  src/quickjs/engine.cpp
  src/quickjs/bytecode.cpp
  src/quickjs/bridge.cpp
)

add_library(venom_core STATIC ${VENOM_CORE_SOURCES})
target_include_directories(venom_core PUBLIC src third_party/quickjs "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(venom_core PRIVATE VENOM_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")
target_link_libraries(venom_core PUBLIC qjs ${CMAKE_DL_LIBS})
if(VENOM_ENABLE_FULL_QUICKJS)
  target_compile_definitions(venom_core PRIVATE VENOM_ENABLE_FULL_QUICKJS)
endif()
venom_apply_warnings(venom_core)
venom_apply_build_acceleration(venom_core)
if(VENOM_ENABLE_PCH)
  target_precompile_headers(venom_core PRIVATE src/compiler/core/pch.hpp)
endif()
venom_apply_pgo(venom_core)

add_executable(venom src/compiler/main.cpp)
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
  target_include_directories(venom_runtime_native PUBLIC src/runtime third_party/quickjs)
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
  target_compile_options(venom_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(venom_core PUBLIC -fsanitize=address,undefined)
endif()

if(VENOM_BUILD_FUZZERS)
  add_executable(venom_package_parser_replay fuzz/package_parser_replay.cpp)
  target_link_libraries(venom_package_parser_replay PRIVATE venom_core)
  venom_apply_warnings(venom_package_parser_replay)

  add_executable(venom_runtime_contract_replay
    fuzz/runtime_contract_replay.cpp
    fuzz/runtime_contract_fuzz.cpp)
  venom_apply_warnings(venom_runtime_contract_replay)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_executable(venom_package_parser_fuzz fuzz/package_parser_fuzz.cpp)
    add_executable(venom_runtime_contract_fuzz fuzz/runtime_contract_fuzz.cpp)
    add_executable(venom_package_parser_differential_fuzz fuzz/package_parser_differential.cpp)
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

include(cmake/tests.cmake)

if(BUILD_TESTING AND VENOM_ENABLE_FULL_QUICKJS)

  add_executable(venom_quickjs_module_bundle_runtime
    tests/quickjs/module-bundle-runtime.cpp
    src/runtime/quickjs_runtime_scaffold.c)
  target_include_directories(venom_quickjs_module_bundle_runtime PRIVATE src third_party/quickjs)
  target_compile_definitions(venom_quickjs_module_bundle_runtime PRIVATE VENOM_ENABLE_UPSTREAM_QJS_WASM)
  target_link_libraries(venom_quickjs_module_bundle_runtime PRIVATE venom_core qjs ${CMAKE_DL_LIBS})
  add_test(NAME quickjs_module_bundle_runtime COMMAND venom_quickjs_module_bundle_runtime)

  add_executable(venom_quickjs_bridge_abi_runtime
    tests/quickjs/bridge-abi-runtime.cpp
    src/runtime/quickjs_runtime_scaffold.c)
  target_include_directories(venom_quickjs_bridge_abi_runtime PRIVATE src third_party/quickjs)
  target_compile_definitions(venom_quickjs_bridge_abi_runtime PRIVATE VENOM_ENABLE_UPSTREAM_QJS_WASM)
  target_link_libraries(venom_quickjs_bridge_abi_runtime PRIVATE venom_core qjs ${CMAKE_DL_LIBS})
  add_test(NAME quickjs_bridge_abi_runtime COMMAND venom_quickjs_bridge_abi_runtime)
endif()

if(BUILD_TESTING)
  add_test(NAME cli_project_runtime_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/cli-project-runtime-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME dev_server_incremental_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/dev-server-incremental-smoke.py)
  add_test(NAME protected_module_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/protected-module-smoke.py
            $<TARGET_FILE:venom>)
  add_test(NAME quickjs_runtime_lifecycle_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-runtime-lifecycle-smoke.py
            ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/quickjs-runtime-lifecycle-smoke)
  add_test(NAME dist_analyzer_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/dist-analyzer-smoke.py
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
