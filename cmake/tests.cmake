# Test registration is isolated from the production target graph.
# Keep this file declarative so the root CMakeLists remains easy to audit.

include(CTest)
if(BUILD_TESTING)
  add_executable(venom_diversification_rng_test tests/vm/diversification-rng.cpp)
  target_link_libraries(venom_diversification_rng_test PRIVATE venom_core)
  venom_apply_warnings(venom_diversification_rng_test)
  add_test(NAME venom_diversification_rng COMMAND venom_diversification_rng_test)
  add_executable(venom_fake_curl tests/helpers/fake_curl.cpp)
  if(MSVC)
    target_compile_definitions(venom_fake_curl PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  venom_apply_warnings(venom_fake_curl)

  if(VENOM_ENABLE_FULL_QUICKJS)
    add_executable(venom_native_bytecode_security tests/quickjs/native-bytecode-security.cpp)
    target_link_libraries(venom_native_bytecode_security PRIVATE venom_core)
    venom_apply_warnings(venom_native_bytecode_security)
    add_test(NAME venom_native_bytecode_security COMMAND venom_native_bytecode_security)

    add_executable(venom_quickjs_stack_overflow_safety tests/quickjs/stack-overflow-safety.cpp)
    target_link_libraries(venom_quickjs_stack_overflow_safety PRIVATE venom_core)
    venom_apply_warnings(venom_quickjs_stack_overflow_safety)
    add_test(NAME venom_quickjs_stack_overflow_safety COMMAND venom_quickjs_stack_overflow_safety)
  endif()

  add_test(NAME venom_version COMMAND venom --version)
  add_test(NAME venom_build_production
    COMMAND venom build ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production-site --out ${CMAKE_BINARY_DIR}/dist-production --vendor-cache ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/remote-cache --offline)
  add_test(NAME venom_verify_production_runtime
    COMMAND venom verify-runtime ${CMAKE_BINARY_DIR}/dist-production --target browser --require-real-engine)
  set_tests_properties(venom_verify_production_runtime PROPERTIES DEPENDS venom_build_production)

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
  add_test(NAME venom_cryptographic_diversification_source
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/cryptographic-diversification-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME venom_route_vm_contract_parity_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/route-vm-contract-parity-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME venom_route_wasm_artifact_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/route-wasm-artifact-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
  add_test(NAME venom_v1_2_contracts_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/v1.2.0-contracts-smoke.py ${CMAKE_CURRENT_SOURCE_DIR} $<TARGET_FILE:venom>)
    add_test(NAME venom_v1_4_production_assurance_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/v1.4.0-production-assurance-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME repository_consistency_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/repository-consistency-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_release_closure_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/release-closure-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_production_only_build_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/production-only-build-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production-site ${CMAKE_BINARY_DIR}/dist-production-only-smoke)
    add_test(NAME venom_image_assets_layout_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/image-assets-layout-smoke.py $<TARGET_FILE:venom>)
    add_test(NAME venom_runtime_surface_diversification_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/runtime-surface-diversification-smoke.py $<TARGET_FILE:venom>)
    add_test(NAME venom_production_artifact_layout_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/production-artifact-layout-smoke.py $<TARGET_FILE:venom>)
    add_test(NAME venom_basic_site_preview_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/basic-site-preview-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production-site ${CMAKE_BINARY_DIR}/dist-basic-preview)
    add_test(NAME venom_build_scripts_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/build-scripts-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_source_layout_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/source-layout-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_quickjs_bytecode_manifest_v3_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-bytecode-manifest-v3-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_quickjs_wasm_stack_safety_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-wasm-stack-safety-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_quickjs_minimal_public_abi_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-minimal-public-abi-smoke.py)
    add_test(NAME venom_phase2_runtime_performance_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase2-runtime-performance-smoke.py)
    add_test(NAME venom_phase3_advanced_protection_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase3-advanced-protection-smoke.py)
    add_test(NAME venom_phase4_live_diversification_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase4-live-diversification-smoke.py)
    add_test(NAME venom_phase5_runtime_resource_hardening_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase5-resource-hardening-smoke.py)
    add_test(NAME venom_phase6_release_supply_chain_hardening_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase6-supply-chain-hardening-smoke.py)
    add_test(NAME venom_phase10_fuzzing_adversarial_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase10-fuzzing-adversarial-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_phase11_performance_tuning_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase11-performance-tuning-smoke.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_phase12_runtime_artifact_consistency_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase12-runtime-artifact-consistency-smoke.py ${CMAKE_CURRENT_SOURCE_DIR} $<TARGET_FILE:venom>)
    add_test(NAME venom_quickjs_module_artifact_gate_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-module-artifact-gate-smoke.py ${CMAKE_CURRENT_SOURCE_DIR} $<TARGET_FILE:venom> ${CMAKE_BINARY_DIR}/dist-quickjs-module-artifact-gate)
    add_test(NAME venom_phase7_unified_runtime_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase7-unified-runtime-smoke.py)
    add_test(NAME venom_quickjs_abi_fingerprint_canonicalization_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/quickjs-abi-fingerprint-canonicalization-smoke.py)
    add_test(NAME venom_phase8_host_bridge_hardening_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase8-host-bridge-hardening-smoke.py)
    add_test(NAME venom_phase9_route_isolation_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/phase9-route-isolation-smoke.py)
    add_test(NAME venom_quickjs_runtime_memory_lifecycle_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/quickjs/runtime-memory-lifecycle-smoke.py)
    add_test(NAME venom_quickjs_release_build_hygiene_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/quickjs/release-build-hygiene-smoke.py)
    add_test(NAME venom_source_inventory_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_source_inventory.py ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME venom_remote_vendor_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/remote-vendor-smoke.py $<TARGET_FILE:venom> $<TARGET_FILE:venom_fake_curl> ${CMAKE_BINARY_DIR}/remote-vendor-work)
    add_test(NAME venom_wasm_owned_package_decode_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/wasm-owned-package-decode-smoke.py)
    add_test(NAME venom_wasm_owned_package_parser_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/wasm-owned-package-parser-smoke.py)
    add_test(NAME venom_adversarial_package_corpus_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/adversarial-package-corpus-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production-site ${CMAKE_BINARY_DIR}/adversarial-package-work ${CMAKE_BINARY_DIR}/adversarial-package-corpus)
    set_tests_properties(venom_adversarial_package_corpus_smoke PROPERTIES TIMEOUT 60)
    add_test(NAME venom_polymorph_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/polymorph-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production-site ${CMAKE_BINARY_DIR}/dist-polymorph-a ${CMAKE_BINARY_DIR}/dist-polymorph-b)
    set_tests_properties(venom_polymorph_smoke PROPERTIES RUN_SERIAL TRUE)
    add_test(NAME venom_release_bytecode_metadata_redaction_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/release-bytecode-metadata-redaction-smoke.py)
    add_test(NAME venom_release_package_metadata_redaction_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/release-package-metadata-redaction-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/examples/browser-compat-site ${CMAKE_BINARY_DIR}/dist-release-metadata-redaction ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_release_metadata.py)
    add_test(NAME venom_release_section_budget_smoke
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/release-section-budget-smoke.py ${CMAKE_BINARY_DIR}/dist-production)
    set_tests_properties(venom_release_section_budget_smoke PROPERTIES DEPENDS venom_build_production)

    find_program(NODE_EXECUTABLE node)
    if(NODE_EXECUTABLE)
      add_test(NAME venom_browser_api_shim_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/browser-api-shim-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/examples/browser-api-shim-site ${CMAKE_BINARY_DIR}/dist-browser-api-shim ${NODE_EXECUTABLE})
      add_test(NAME venom_browser_runtime_compat_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/browser-runtime-compat-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/examples/browser-compat-site ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/production-site ${CMAKE_BINARY_DIR}/dist-browser-runtime-compat ${NODE_EXECUTABLE})
      add_test(NAME venom_production_loader_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/production-loader-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/examples/browser-compat-site ${CMAKE_BINARY_DIR}/dist-production-loader ${NODE_EXECUTABLE})
      add_test(NAME venom_production_tamper_gate_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/production-tamper-gate-smoke.py $<TARGET_FILE:venom> ${CMAKE_CURRENT_SOURCE_DIR}/examples/browser-compat-site ${CMAKE_BINARY_DIR}/dist-production-tamper ${NODE_EXECUTABLE})
      set_tests_properties(venom_production_tamper_gate_smoke PROPERTIES TIMEOUT 120)
      add_test(NAME venom_production_site_boot_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/production-site-boot-smoke.py $<TARGET_FILE:venom> ${CMAKE_BINARY_DIR}/production-site-boot ${NODE_EXECUTABLE})
      set_tests_properties(venom_production_site_boot_smoke PROPERTIES TIMEOUT 120)
      add_test(NAME venom_html_rendering_fidelity_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/html-rendering-fidelity-smoke.py $<TARGET_FILE:venom> ${CMAKE_BINARY_DIR}/html-rendering-fidelity ${NODE_EXECUTABLE})
      set_tests_properties(venom_html_rendering_fidelity_smoke PROPERTIES TIMEOUT 120)
      if(NOT VENOM_REQUIRE_BROWSER_RUNTIME_TESTS)
        set_tests_properties(
          venom_browser_api_shim_smoke
          venom_browser_runtime_compat_smoke
          venom_production_loader_smoke
          venom_production_tamper_gate_smoke
          venom_production_site_boot_smoke
          venom_html_rendering_fidelity_smoke
          PROPERTIES
            SKIP_REGULAR_EXPRESSION "QuickJS WASM interpreter unavailable; source-decode fallback denied"
        )
      endif()
    endif()
  endif()

  add_test(NAME venom_production_readiness_report_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/production-readiness-report-smoke.py)
  add_test(NAME venom_browser_validation_tool_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/browser-validation-tool-smoke.py)
add_test(NAME venom_browser_performance_budget_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/browser-performance-budget-smoke.py ${CMAKE_SOURCE_DIR})
  add_test(NAME venom_compatibility_matrix_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/compatibility-matrix-smoke.py)

add_test(NAME venom_release_publication_set_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/release-publication-set-smoke.py)
  add_test(NAME venom_verified_runtime_release_chain_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/verified-runtime-release-chain-smoke.py)
  add_test(NAME venom_route_generation_lifecycle_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/route-generation-lifecycle-smoke.py ${CMAKE_SOURCE_DIR})
  add_test(NAME venom_runtime_resource_policy_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/runtime-resource-policy-smoke.py)
  add_test(NAME venom_wasm_route_vm_surface_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/wasm-route-vm-surface-smoke.py $<TARGET_FILE:venom> ${CMAKE_SOURCE_DIR})
  add_test(NAME venom_release_installation_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/release-installation-smoke.py)
  add_test(NAME venom_compatibility_evidence_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/compatibility-evidence-smoke.py)
  add_test(NAME venom_react16_framework_fixture_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/react16-framework-fixture-smoke.py)
  add_test(NAME venom_umd_compatibility_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/umd-compatibility-smoke.py $<TARGET_FILE:venom>)

  add_test(NAME venom_phase13_release_bootstrap_optimization_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/phase13-release-bootstrap-optimization-smoke.py)
  add_test(NAME venom_phase14_minimal_release_runtime_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/phase14-minimal-release-runtime-smoke.py)
  add_test(NAME venom_phase15_binary_host_bridge_smoke
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/phase15-binary-host-bridge-smoke.py)
endif()


add_test(NAME product-contracts-generated-check
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_product_contracts.py --repo-root ${CMAKE_CURRENT_SOURCE_DIR} --check)

add_test(NAME product-contracts-and-minimal-metadata-smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/package/product-contracts-and-minimal-metadata-smoke.py $<TARGET_FILE:venom>)

add_test(NAME venom_capability_graph_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/capability-graph-smoke.py $<TARGET_FILE:venom>)
add_test(NAME venom_dynamic_module_graph_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/dynamic-module-graph-smoke.py $<TARGET_FILE:venom>)
add_test(NAME venom_dynamic_module_browser_evidence_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/dynamic-module-browser-evidence-smoke.py)
add_test(NAME venom_runtime_module_specialization_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/runtime-module-specialization-smoke.py $<TARGET_FILE:venom>)
add_test(NAME venom_dom_compatibility_modules_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/dom-compatibility-modules-smoke.py $<TARGET_FILE:venom>)

foreach(_venom_qjs_artifact_test IN ITEMS
    venom_release_package_metadata_redaction_smoke
    venom_browser_runtime_compat_smoke
    venom_production_loader_smoke
    venom_production_tamper_gate_smoke
    venom_runtime_module_specialization_smoke)
  if(TEST ${_venom_qjs_artifact_test})
    set_tests_properties(${_venom_qjs_artifact_test} PROPERTIES SKIP_RETURN_CODE 77)
  endif()
endforeach()

add_test(
  NAME venom_function_realm_plan_smoke
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/function-realm-plan-smoke.py
)

add_test(NAME venom_function_extraction_plan_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/function-extraction-plan-smoke.py)
add_test(NAME venom_realm_bridge_worker_protocol_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/realm-bridge-worker-protocol-smoke.py)
add_test(NAME venom_realm_bridge_quickjs_abi_smoke COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/package/realm-bridge-quickjs-abi-smoke.py)
