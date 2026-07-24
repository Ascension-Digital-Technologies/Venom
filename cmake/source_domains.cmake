# First-party source domains
#
# Every top-level source domain has two targets:
#   venom_<domain>_api      - target-scoped API dependencies
#   venom_<domain>          - implementation objects
#
# Headers are colocated with their implementation files beneath src/<domain>.
# The source root is an include root so cross-domain includes remain explicit
# (for example, #include "package/reader.hpp").

add_library(venom_domain_settings INTERFACE)
target_compile_definitions(venom_domain_settings INTERFACE
  VENOM_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
)
if(VENOM_ENABLE_FULL_TURBOJS)
  target_compile_definitions(venom_domain_settings INTERFACE VENOM_ENABLE_FULL_TURBOJS)
endif()

function(venom_add_source_domain target)
  cmake_parse_arguments(ARG "NO_PCH" "DOMAIN" "SOURCES;API_DEPENDS;PRIVATE_DEPENDS;EXTERNAL_LIBS;EXTERNAL_INCLUDE_DIRS" ${ARGN})
  if(NOT ARG_DOMAIN)
    message(FATAL_ERROR "${target} has no DOMAIN")
  endif()
  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "${target} has no source files")
  endif()

  set(_api_target "${target}_api")
  add_library(${_api_target} INTERFACE)
  target_include_directories(${_api_target} INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
  )
  foreach(_dependency IN LISTS ARG_API_DEPENDS)
    target_link_libraries(${_api_target} INTERFACE "venom_${_dependency}_api")
  endforeach()

  add_library(${target} OBJECT ${ARG_SOURCES})
  target_include_directories(${target} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    ${ARG_EXTERNAL_INCLUDE_DIRS}
  )
  target_link_libraries(${target} PRIVATE
    venom_domain_settings
    ${_api_target}
    ${ARG_EXTERNAL_LIBS}
  )
  foreach(_dependency IN LISTS ARG_PRIVATE_DEPENDS)
    target_link_libraries(${target} PRIVATE "venom_${_dependency}_api")
  endforeach()

  venom_apply_warnings(${target})
  venom_apply_build_acceleration(${target})
  if(VENOM_PGO_MODE_NORMALIZED STREQUAL "generate")
    file(MAKE_DIRECTORY "${VENOM_PGO_DIR}")
    if(MSVC)
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/GL>)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-fprofile-generate=${VENOM_PGO_DIR}>)
    endif()
  elseif(VENOM_PGO_MODE_NORMALIZED STREQUAL "use")
    if(MSVC)
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:/GL>)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
      target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-fprofile-use=${VENOM_PGO_DIR}> $<$<CONFIG:Release>:-fprofile-correction>)
    endif()
  endif()
  if(VENOM_ENABLE_PCH AND NOT ARG_NO_PCH)
    target_precompile_headers(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/core/pch.hpp")
  endif()

  string(REPLACE "venom_" "" _domain_alias "${target}")
  add_library("venom::${_domain_alias}" ALIAS "${_api_target}")
  add_library("venom::${_domain_alias}_objects" ALIAS "${target}")
endfunction()

venom_add_source_domain(venom_base
  DOMAIN base
  NO_PCH
  SOURCES src/base/error.cpp
)

venom_add_source_domain(venom_generated
  DOMAIN generated
  PRIVATE_DEPENDS pipeline runtime_services
  SOURCES
    src/generated/compiler/typescript_compiler_asset.cpp
    src/generated/compiler/typescript_bridge_bytecode.cpp
    src/generated/runtime/worker_runtime_js.cpp
    src/generated/runtime/browser_asset_runtime_js.cpp
    src/generated/runtime/runtime_js.cpp
    "${VENOM_RUNTIME_TEMPLATE_CPP}"
    src/generated/runtime/wasm_runtime_js.cpp
    "${VENOM_TURBOJS_ENGINE_CPP}"
)

venom_add_source_domain(venom_package
  DOMAIN package
  API_DEPENDS generated
  PRIVATE_DEPENDS base
  SOURCES
    src/package/compress.cpp
    src/package/crypto.cpp
    src/package/format.cpp
    src/package/hash.cpp
    src/package/reader.cpp
    src/package/writer.cpp
)

venom_add_source_domain(venom_core_support
  DOMAIN core
  API_DEPENDS base
  PRIVATE_DEPENDS package
  SOURCES
    src/core/config.cpp
    src/core/console.cpp
    src/core/diagnostic.cpp
    src/core/error_reporting.cpp
    src/core/process.cpp
    src/core/profile.cpp
    src/core/project.cpp
    src/core/site.cpp
)

venom_add_source_domain(venom_frontends
  DOMAIN frontends
  PRIVATE_DEPENDS base core_support generated package
  EXTERNAL_LIBS tjs
  SOURCES
    src/frontends/javascript/embedded_bundles.cpp
    src/frontends/javascript/frontend.cpp
    src/frontends/jsx/frontend.cpp
    src/frontends/typescript/frontend.cpp
    src/frontends/typescript/typescript_runtime.cpp
)

venom_add_source_domain(venom_graph
  DOMAIN graph
  PRIVATE_DEPENDS frontends package
  SOURCES src/graph/module_graph.cpp
)

venom_add_source_domain(venom_vm
  DOMAIN vm
  PRIVATE_DEPENDS base package
  SOURCES
    src/vm/encoder.cpp
    src/vm/opcode.cpp
    src/vm/polymorph.cpp
)

venom_add_source_domain(venom_turbojs
  DOMAIN turbojs
  PRIVATE_DEPENDS base package
  EXTERNAL_LIBS tjs
  SOURCES
    src/turbojs/abi.cpp
    src/turbojs/bridge.cpp
    src/turbojs/bytecode.cpp
    src/turbojs/engine.cpp
)

venom_add_source_domain(venom_remote
  DOMAIN remote
  PRIVATE_DEPENDS base core_support package
  SOURCES src/remote/remote.cpp
)

venom_add_source_domain(venom_runtime_services
  DOMAIN runtime
  PRIVATE_DEPENDS base core_support generated
  SOURCES src/runtime/manager.cpp
)

venom_add_source_domain(venom_pipeline
  DOMAIN pipeline
  API_DEPENDS core_support graph package remote vm
  PRIVATE_DEPENDS base frontends generated turbojs
  EXTERNAL_LIBS tjs
  SOURCES
    src/pipeline/assets.cpp
    src/pipeline/build.cpp
    src/pipeline/build_package_metadata.cpp
    src/pipeline/build_report.cpp
    src/pipeline/build_runtime_audit_metadata.cpp
    src/pipeline/build_runtime_metadata.cpp
    src/pipeline/build_runtime_module_metadata.cpp
    src/pipeline/build_support.cpp
    src/pipeline/build_hardener_cache.cpp
    src/pipeline/capability_analysis.cpp
    src/pipeline/chrome_extension.cpp
    src/pipeline/compatibility.cpp
    src/pipeline/css.cpp
    src/pipeline/function_dependencies.cpp
    src/pipeline/html.cpp
    src/pipeline/js.cpp
    src/pipeline/js_bundle_encoding.cpp
    src/pipeline/js_discovery.cpp
    src/pipeline/js_planning.cpp
    src/pipeline/js_rewriting.cpp
    src/pipeline/native_js_hardener.cpp
    src/pipeline/planner.cpp
    src/pipeline/planning/package_plan.cpp
    src/pipeline/planning/section_plan.cpp
    src/pipeline/project_ir.cpp
    src/pipeline/turbojs_wasm_contract.cpp
    src/pipeline/runtime_modules.cpp
    src/pipeline/security.cpp
    src/pipeline/security_analysis.cpp
    src/pipeline/security_artifact_binary.cpp
    src/pipeline/security_artifact_inspection.cpp
)

venom_add_source_domain(venom_cli_commands
  DOMAIN cli
  API_DEPENDS core_support
  PRIVATE_DEPENDS base package pipeline turbojs runtime_services
  SOURCES
    src/cli/cli.cpp
    src/cli/compile_snippet.cpp
    src/cli/dev.cpp
    src/cli/dist_analyzer.cpp
    src/cli/doctor.cpp
    src/cli/inspect.cpp
    src/cli/update_manager.cpp
)

set(VENOM_DOMAIN_TARGETS
  venom_base
  venom_core_support
  venom_frontends
  venom_graph
  venom_package
  venom_vm
  venom_turbojs
  venom_remote
  venom_runtime_services
  venom_generated
  venom_pipeline
  venom_cli_commands
)

set(VENOM_DOMAIN_API_TARGETS
  venom_base_api
  venom_core_support_api
  venom_frontends_api
  venom_graph_api
  venom_package_api
  venom_vm_api
  venom_turbojs_api
  venom_remote_api
  venom_runtime_services_api
  venom_generated_api
  venom_pipeline_api
  venom_cli_commands_api
)

add_library(venom_core STATIC)
foreach(_venom_domain_target IN LISTS VENOM_DOMAIN_TARGETS)
  target_sources(venom_core PRIVATE $<TARGET_OBJECTS:${_venom_domain_target}>)
endforeach()
target_link_libraries(venom_core PUBLIC tjs ${CMAKE_DL_LIBS} ${VENOM_DOMAIN_API_TARGETS})
add_library(venom::core ALIAS venom_core)
