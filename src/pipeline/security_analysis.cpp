#include "security_analysis.hpp"
#include "security_artifact_inspection.hpp"

#include "venom/package/hash.hpp"
#include "venom/package/reader.hpp"

#include <algorithm>
#include <filesystem>
#include <set>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {

using namespace security_detail;

void add_failure(ReleaseCheckReport& report, std::string message) {
  report.failures.push_back(std::move(message));
}

ReleaseCheckReport analyze_package_for_release(const ReleaseCheckOptions& options) {
  ReleaseCheckReport report;
  report.package_path = resolve_package_path(options.target);
  report.dist_root = resolve_dist_root(options.target, report.package_path);

  if (!options.key_file.empty()) {
    load_package_key_file_for_process(options.key_file);
  }

  const auto raw = read_binary_file(report.package_path);
  report.v_runtime_sections = count_bytes(raw, "VAEAD001");
  report.v_sodium_sections = count_bytes(raw, "VSODIUM1");
  report.v_legacy_sections = count_bytes(raw, "VSEAL001");
  report.external_manifest = std::filesystem::exists(report.dist_root / "assets" / "asset-manifest.txt");

  const auto build_metadata_path = std::filesystem::exists(report.dist_root / "assets" / "app" / "build.json")
      ? report.dist_root / "assets" / "app" / "build.json"
      : report.dist_root / "assets" / "build.json";
  if (std::filesystem::exists(build_metadata_path)) {
    const auto metadata = read_text_file(build_metadata_path);
    std::smatch closure;
    const std::regex closure_pattern(
        R"("protection_closure"\s*:\s*\{[^}]*"requested"\s*:\s*([0-9]+)[^}]*"resolved"\s*:\s*([0-9]+)[^}]*"expected_quickjs_records"\s*:\s*([0-9]+))");
    if (std::regex_search(metadata, closure, closure_pattern)) {
      report.protection_closure_present = true;
      report.protection_intents_requested = static_cast<std::size_t>(std::stoull(closure[1].str()));
      report.protection_intents_resolved = static_cast<std::size_t>(std::stoull(closure[2].str()));
      report.protection_expected_quickjs_records = static_cast<std::size_t>(std::stoull(closure[3].str()));
    }
  }

  const char* forbidden_raw[] = {
    "console.log",
    "basic site script loaded",
    "route-bytecode.vmbc",
    "routes.vbrt",
    "strings.vstr",
    "runtime-policy.vhrd",
    "package-features.vfeat",
    "VENOM_INTEGRITY_V1",
    "VENOM_PACKAGE_FEATURES_V2",
    "VENOM_POLYMORPH",
    "word_layout=",
    "VQJSBC01",
    "source-preserving-byte-buffer-record",
    "VENOM_LAZY_SECTIONS_V1",
    "lazy-sections.vlazy",
    "route-chunk.",
    "script-chunk.",
    "VENOM_RELEASE_PROFILE_V1",
    "release-profile.vrpf",
    "browser-client-protection-v1",
    "native-private-aead-v1",
    "VENOM_QJS_WASM_EXECUTION_V1",
    "quickjs-wasm-execution.vqwe",
    "quickjs-wasm-real",
    "VENOM_QJS_INTERPRETER_CONTRACT_V1",
    "quickjs-wasm-abi12-interpreter-v1",
    "quickjs-wasm-abi12-interpreter-dispatch",
    "quickjs-wasm-abi12-upstream-global-host-api-shims",
    "quickjs-upstream-global-host-api-shims-v7",
    "quickjs-upstream-object-semantics-v3",
    "quickjs-upstream-exception-semantics-v3",
    "quickjs-upstream-module-semantics-v3",
    "VENOM_QUICKJS_NATIVE_PARITY_V3",
  };
  for (const char* needle : forbidden_raw) {
    if (contains_bytes(raw, needle)) {
      add_failure(report, std::string("raw package leaks protected/debug marker: ") + needle);
    }
  }

  venom::package::Package pkg;
  try {
    pkg = venom::package::read_package(report.package_path);
  } catch (const std::exception& ex) {
    if (report.v_sodium_sections != 0u && options.key_file.empty()) {
      add_failure(report, "libsodium package requires --key-file for decoded integrity verification");
    }
    add_failure(report, std::string("package reader rejected artifact: ") + ex.what());
    return report;
  }

  report.flags = pkg.flags;
  report.package_hash = pkg.package_hash;
  report.section_count = pkg.sections.size();
  report.release_or_protect = (pkg.flags & (venom::package::PackageFlagProtectProfile | venom::package::PackageFlagReleaseProfile)) != 0u;
  report.runtime_hardened = (pkg.flags & venom::package::PackageFlagRuntimeHardened) != 0u;
  report.integrity_metadata = (pkg.flags & venom::package::PackageFlagIntegrityMetadata) != 0u;
  report.payload_padding_bytes = decoded_payload_padding_bytes(pkg);

  if (!report.protection_closure_present) {
    for (const auto& section : pkg.sections) {
      if (section.type != venom::package::SectionType::Integrity ||
          !venom::package::section_name_matches(section.type, section.name, "protection-closure.vpcl")) continue;
      const auto text = std::string(reinterpret_cast<const char*>(section.data.data()), section.data.size());
      if (text.rfind("VENOM_PROTECTION_CLOSURE_V1", 0) != 0) {
        add_failure(report, "protection closure metadata has invalid header");
        break;
      }
      report.protection_closure_present = true;
      const auto requested = metadata_value(text, "requested");
      const auto resolved = metadata_value(text, "resolved");
      const auto expected = metadata_value(text, "expected_quickjs_records");
      report.protection_intents_requested = requested.empty() ? 0u : static_cast<std::size_t>(std::stoull(requested));
      report.protection_intents_resolved = resolved.empty() ? 0u : static_cast<std::size_t>(std::stoull(resolved));
      report.protection_expected_quickjs_records = expected.empty() ? 0u : static_cast<std::size_t>(std::stoull(expected));
      break;
    }
  }

  const venom::package::Section* release_profile_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "release-profile.vrpf")) {
      release_profile_section = &section;
      break;
    }
  }
  if (release_profile_section != nullptr) {
    report.release_profile = true;
    const auto text = std::string(reinterpret_cast<const char*>(release_profile_section->data.data()), release_profile_section->data.size());
    if (text.rfind("VENOM_RELEASE_PROFILE_V1", 0) != 0) {
      add_failure(report, "release profile metadata has invalid header");
    }
    report.release_threat_model = metadata_value(text, "threat_model");
    report.release_secret_model = metadata_value(text, "secret_material_model");
    report.release_profile_audited_crypto = metadata_value(text, "audited_crypto") == "true";
    report.release_profile_external_key_required = metadata_value(text, "external_key_required") == "true";
    report.release_profile_browser_executable = metadata_value(text, "browser_executable") == "true";
    if (metadata_value(text, "debug_metadata_allowed") != "false") {
      add_failure(report, "release profile allows debug metadata");
    }
    if (metadata_value(text, "external_manifest_allowed") != "false") {
      add_failure(report, "release profile allows external debug manifests");
    }
    if (metadata_value(text, "source_preserving_js_allowed") != "false") {
      add_failure(report, "release profile allows source-preserving JavaScript records");
    }
    if (metadata_value(text, "remote_script_policy") != "build-time-vendor") {
      add_failure(report, "release profile does not require build-time remote script vendoring");
    }
    if (metadata_value(text, "unvendored_remote_scripts_allowed") != "false") {
      add_failure(report, "release profile allows unvendored remote scripts");
    }
    if (metadata_value(text, "runtime_remote_script_network_required") != "false") {
      add_failure(report, "release profile still requires runtime network loading for script tags");
    }
    if (metadata_value(text, "package_binding_required") != "true") {
      add_failure(report, "release profile does not require package binding");
    }
    if (metadata_value(text, "layout_polymorphism_required") != "true") {
      add_failure(report, "release profile does not require layout polymorphism");
    }
    if (metadata_value(text, "lazy_sections_required") != "true") {
      add_failure(report, "release profile does not require lazy sections");
    }
    if (metadata_value(text, "server_secret_claim") != "false") {
      add_failure(report, "release profile incorrectly claims browser/server secret protection");
    }
    if (metadata_value(text, "quickjs_backend") == "scaffold") {
      add_failure(report, "release profile still declares scaffold QuickJS backend");
    }
    if (metadata_value(text, "host_js_fallback_allowed") == "true") {
      add_failure(report, "release profile allows host-JS fallback");
    }
  }

  const venom::package::Section* remote_vendor_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "remote-vendors.vrvd")) {
      remote_vendor_section = &section;
      break;
    }
  }
  if (remote_vendor_section != nullptr) {
    report.remote_vendor_metadata = true;
    const auto text = std::string(reinterpret_cast<const char*>(remote_vendor_section->data.data()), remote_vendor_section->data.size());
    if (text.rfind("VENOM_REMOTE_VENDOR_V2", 0) != 0) {
      add_failure(report, "remote vendor metadata has invalid header");
    }
    if (metadata_value(text, "policy") != "build-time-fetch-package-bytecode") {
      add_failure(report, "remote vendor metadata has an invalid production policy");
    }
    if (metadata_value(text, "network_runtime_required") != "false") {
      add_failure(report, "remote vendor metadata requires runtime network script loading");
    }
    const auto vendor_count = metadata_value(text, "vendor_count");
    const auto unique_vendor_count = metadata_value(text, "unique_vendor_count");
    const auto unvendored = metadata_value(text, "unvendored_remote_scripts");
    report.remote_vendor_count = static_cast<std::size_t>(std::stoull(vendor_count.empty() ? "0" : vendor_count));
    report.remote_vendor_unique_count = static_cast<std::size_t>(std::stoull(unique_vendor_count.empty() ? "0" : unique_vendor_count));
    report.unvendored_remote_scripts = static_cast<std::size_t>(std::stoull(unvendored.empty() ? "0" : unvendored));
    report.remote_vendor_lock = metadata_value(text, "vendor_lock_required") == "true";
    report.remote_vendor_lock_mode = metadata_value(text, "vendor_lock_mode");
    report.remote_vendor_lock_sha256 = metadata_value(text, "vendor_lock_sha256");
    const auto lock_entries = metadata_value(text, "vendor_lock_entries");
    report.remote_vendor_lock_entries = static_cast<std::size_t>(std::stoull(lock_entries.empty() ? "0" : lock_entries));
    if (!report.remote_vendor_lock) {
      add_failure(report, "remote vendor metadata does not require venom.lock");
    }
    if (report.remote_vendor_lock_mode != "enforced" && report.remote_vendor_lock_mode != "generated" &&
        report.remote_vendor_lock_mode != "refreshed") {
      add_failure(report, "remote vendor metadata has an invalid lock mode");
    }
    if (report.remote_vendor_lock_sha256.size() != 64u ||
        !std::all_of(report.remote_vendor_lock_sha256.begin(), report.remote_vendor_lock_sha256.end(), [](unsigned char c) {
          return std::isdigit(c) != 0 || (c >= 'a' && c <= 'f');
        })) {
      add_failure(report, "remote vendor metadata has an invalid lock SHA-256");
    }
    if (report.unvendored_remote_scripts != 0u) {
      add_failure(report, "protected package contains unvendored remote scripts");
    }
  }

  const venom::package::Section* wasm_exec_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::QuickJsEngineBackend && venom::package::section_name_matches(section.type, section.name, "quickjs-wasm-execution.vqwe")) {
      wasm_exec_section = &section;
      break;
    }
  }
  if (wasm_exec_section != nullptr) {
    report.quickjs_wasm_execution = true;
    const auto text = std::string(reinterpret_cast<const char*>(wasm_exec_section->data.data()), wasm_exec_section->data.size());
    if (text.rfind("VENOM_QJS_WASM_EXECUTION_V2", 0) != 0 && text.rfind("VENOM_QJS_WASM_EXECUTION_V1", 0) != 0) {
      add_failure(report, "QuickJS/WASM execution metadata has invalid header");
    }
    report.quickjs_execution_backend = metadata_value(text, "selected_backend");
    const auto exec_runtime_impl = metadata_value(text, "runtime_implementation");
    if (!exec_runtime_impl.empty()) report.quickjs_runtime_implementation = exec_runtime_impl;
    const auto exec_backend_claim = metadata_value(text, "backend_claim");
    if (!exec_backend_claim.empty()) report.quickjs_runtime_claim = exec_backend_claim;
    if (metadata_value(text, "contract_only") == "true") report.quickjs_runtime_contract_only = true;
    if (metadata_value(text, "scaffold_runtime") == "true") report.quickjs_runtime_scaffold = true;
    if (metadata_value(text, "real_engine_candidate") == "true") report.quickjs_runtime_real_engine_candidate = true;
    if (metadata_value(text, "full_upstream_quickjs") == "true") report.quickjs_runtime_full_upstream_quickjs = true;
    const auto exec_artifact_kind = metadata_value(text, "artifact_kind");
    if (!exec_artifact_kind.empty()) report.quickjs_runtime_artifact_kind = exec_artifact_kind;
    report.quickjs_runtime_required_exports_satisfied = report.quickjs_runtime_required_exports_satisfied || metadata_value(text, "required_exports_satisfied") == "true";
    const auto exec_missing_exports = metadata_value(text, "missing_export_count");
    if (!exec_missing_exports.empty()) report.quickjs_runtime_missing_export_count = static_cast<std::size_t>(std::stoull(exec_missing_exports));
    report.quickjs_host_js_fallback_allowed = metadata_value(text, "host_js_fallback_allowed") == "true";
    report.quickjs_release_fail_closed = metadata_value(text, "release_fail_closed") == "true";
    report.quickjs_wasm_chunks = static_cast<std::size_t>(std::stoull(metadata_value(text, "chunk_count").empty() ? "0" : metadata_value(text, "chunk_count")));
    const auto exec_runtime_abi_text = metadata_value(text, "runtime_abi");
    if (!exec_runtime_abi_text.empty()) report.quickjs_runtime_abi = static_cast<std::uint32_t>(std::stoul(exec_runtime_abi_text));
    report.quickjs_interpreter_dispatch = metadata_value(text, "interpreter_dispatch") == "enabled";
    report.quickjs_semantic_runtime = metadata_value(text, "semantic_runtime") == "enabled";
    report.quickjs_upstream_parity = metadata_value(text, "upstream_quickjs_bridge") == "enabled" || metadata_value(text, "upstream_parity_bridge") == "enabled";
    report.quickjs_upstream_bytecode_semantics = metadata_value(text, "upstream_bytecode_semantics") == "enabled";
    report.quickjs_upstream_intrinsic_semantics = metadata_value(text, "upstream_intrinsic_semantics") == "enabled";
    if (!report.quickjs_upstream_parity) {
      add_failure(report, "QuickJS/WASM execution metadata does not enable the upstream QuickJS parity bridge");
    }
    if (metadata_value(text, "backend_contract") != "quickjs-wasm-abi12-upstream-global-host-api-shims-v7") {
      add_failure(report, "QuickJS/WASM execution metadata is not ABI12 upstream-runtime contract-bound");
    }
    if (metadata_value(text, "decode_boundary") != "wasm-owned-bytecode-abi12-upstream-semantics") {
      add_failure(report, "QuickJS/WASM execution metadata does not use ABI12 upstream-runtime bytecode boundary");
    }
    if (!report.quickjs_interpreter_dispatch) {
      add_failure(report, "QuickJS/WASM execution metadata does not enable interpreter dispatch");
    }
    if (!report.quickjs_semantic_runtime) {
      add_failure(report, "QuickJS/WASM execution metadata does not enable semantic runtime records");
    }
    if (!report.quickjs_upstream_bytecode_semantics) {
      add_failure(report, "QuickJS/WASM execution metadata does not enable upstream bytecode semantics records");
    }
    if (!report.quickjs_upstream_intrinsic_semantics) {
      add_failure(report, "QuickJS/WASM execution metadata does not enable upstream intrinsic/object semantics records");
    }
    if (metadata_value(text, "source_eval_in_runtime") != "false") {
      add_failure(report, "QuickJS/WASM execution metadata allows source eval inside runtime");
    }
    if (report.release_or_protect && report.quickjs_runtime_abi < 12u) {
      add_failure(report, "QuickJS/WASM execution metadata runtime ABI is below v12");
    }
    if (metadata_value(text, "bytecode_validate_export") != "venom_qjs_bytecode_validate") {
      add_failure(report, "QuickJS/WASM execution metadata is missing bytecode validation export");
    }
    if (metadata_value(text, "enabled") != "true") {
      add_failure(report, "QuickJS/WASM execution metadata is not enabled");
    }
    if (report.quickjs_execution_backend != "quickjs-wasm-real") {
      add_failure(report, "QuickJS/WASM execution metadata does not select quickjs-wasm-real");
    }
    if (report.quickjs_host_js_fallback_allowed) {
      add_failure(report, "QuickJS/WASM execution metadata allows host-JS fallback");
    }
    if (!report.quickjs_release_fail_closed) {
      add_failure(report, "QuickJS/WASM execution metadata is not fail-closed");
    }
    if (metadata_value(text, "source_eval_fallback") != "false") {
      add_failure(report, "QuickJS/WASM execution metadata allows source eval fallback");
    }
    if (metadata_value(text, "bytecode_format") != "VQJSBC03") {
      add_failure(report, "QuickJS/WASM execution metadata does not require VQJSBC03 bytecode records");
    }
  }

  const venom::package::Section* wasm_runtime_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::QuickJsWasmRuntime && venom::package::section_name_matches(section.type, section.name, "quickjs-wasm-runtime.vqwr")) {
      wasm_runtime_section = &section;
      break;
    }
  }
  if (wasm_runtime_section != nullptr) {
    const auto text = std::string(reinterpret_cast<const char*>(wasm_runtime_section->data.data()), wasm_runtime_section->data.size());
    report.quickjs_wasm_runtime_mode = metadata_value(text, "execution_mode");
    const auto runtime_impl = metadata_value(text, "runtime_implementation");
    if (!runtime_impl.empty()) report.quickjs_runtime_implementation = runtime_impl;
    const auto runtime_claim = metadata_value(text, "runtime_claim");
    if (!runtime_claim.empty()) report.quickjs_runtime_claim = runtime_claim;
    report.quickjs_runtime_contract_only = report.quickjs_runtime_contract_only || metadata_value(text, "contract_only") == "true";
    report.quickjs_runtime_scaffold = report.quickjs_runtime_scaffold || metadata_value(text, "scaffold_runtime") == "true";
    report.quickjs_runtime_real_engine_candidate = report.quickjs_runtime_real_engine_candidate || metadata_value(text, "real_engine_candidate") == "true";
    report.quickjs_runtime_full_upstream_quickjs = report.quickjs_runtime_full_upstream_quickjs || metadata_value(text, "full_upstream_quickjs") == "true";
    report.quickjs_runtime_fallback_required = report.quickjs_runtime_fallback_required || metadata_value(text, "fallback_required") == "true";
    report.quickjs_runtime_finish_blocker = metadata_value(text, "finish_blocker");
    report.quickjs_runtime_artifact_kind = metadata_value(text, "artifact_kind");
    report.quickjs_runtime_wasm_sha256 = metadata_value(text, "wasm_sha256");
    report.quickjs_runtime_required_exports_satisfied = report.quickjs_runtime_required_exports_satisfied || metadata_value(text, "required_exports_satisfied") == "true";
    const auto runtime_missing_exports = metadata_value(text, "missing_export_count");
    if (!runtime_missing_exports.empty()) report.quickjs_runtime_missing_export_count = static_cast<std::size_t>(std::stoull(runtime_missing_exports));
    const auto runtime_abi_text = metadata_value(text, "abi");
    const auto runtime_pkg_text = metadata_value(text, "package_version");
    if (!runtime_abi_text.empty()) report.quickjs_runtime_abi = static_cast<std::uint32_t>(std::stoul(runtime_abi_text));
    if (!runtime_pkg_text.empty()) report.quickjs_runtime_package_version = static_cast<std::uint32_t>(std::stoul(runtime_pkg_text));
    report.quickjs_abi_contract = metadata_value(text, "abi_contract");
    report.quickjs_execute_bytecode_export = contains_bytes(wasm_runtime_section->data, "venom_qjs_execute_bytecode");
    report.quickjs_bytecode_validate_export = contains_bytes(wasm_runtime_section->data, "venom_qjs_bytecode_validate");
    report.quickjs_status_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_status_code") && contains_bytes(wasm_runtime_section->data, "venom_qjs_release_status");
    report.quickjs_limit_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_context_heap_limit") && contains_bytes(wasm_runtime_section->data, "venom_qjs_context_heap_used") && contains_bytes(wasm_runtime_section->data, "venom_qjs_context_stack_limit");
    report.quickjs_bytecode_validation_exports = report.quickjs_bytecode_validate_export && contains_bytes(wasm_runtime_section->data, "venom_qjs_bytecode_record_hash32") && contains_bytes(wasm_runtime_section->data, "venom_qjs_bytecode_payload_size");
    report.quickjs_backend_contract_export = contains_bytes(wasm_runtime_section->data, "venom_qjs_backend_kind") && contains_bytes(wasm_runtime_section->data, "venom_qjs_backend_ready");
    report.quickjs_interpreter_contract_export = contains_bytes(wasm_runtime_section->data, "venom_qjs_interpreter_ready") && contains_bytes(wasm_runtime_section->data, "venom_qjs_execute_bytecode");
    report.quickjs_interpreter_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_interpreter_ready") && contains_bytes(wasm_runtime_section->data, "venom_qjs_execute_bytecode");
    report.quickjs_semantic_runtime_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_bytecode_validate") && contains_bytes(wasm_runtime_section->data, "venom_qjs_result_ptr") && contains_bytes(wasm_runtime_section->data, "venom_qjs_result_size");
    report.quickjs_upstream_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_upstream_quickjs_ready") && contains_bytes(wasm_runtime_section->data, "venom_qjs_real_engine_candidate");
    report.quickjs_upstream_runtime_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_exception_ptr") && contains_bytes(wasm_runtime_section->data, "venom_qjs_module_execute");
    report.quickjs_upstream_bytecode_semantics_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_execute_bytecode") && contains_bytes(wasm_runtime_section->data, "venom_qjs_bytecode_validate");
    report.quickjs_upstream_intrinsic_semantics_exports = contains_bytes(wasm_runtime_section->data, "venom_qjs_upstream_quickjs_ready") && contains_bytes(wasm_runtime_section->data, "venom_qjs_backend_ready");
    report.quickjs_abi12_runtime = report.quickjs_runtime_abi >= 12u && report.quickjs_abi_contract == "quickjs-wasm-abi12-runtime" && report.quickjs_status_exports && report.quickjs_limit_exports && report.quickjs_bytecode_validation_exports && report.quickjs_backend_contract_export && report.quickjs_interpreter_contract_export && report.quickjs_interpreter_exports && report.quickjs_semantic_runtime_exports && report.quickjs_upstream_exports && report.quickjs_upstream_runtime_exports && report.quickjs_upstream_bytecode_semantics_exports && report.quickjs_upstream_intrinsic_semantics_exports;
    if (report.release_or_protect && report.quickjs_wasm_runtime_mode != "quickjs-wasm-abi12-upstream-global-host-api-shims") {
      add_failure(report, "QuickJS/WASM runtime metadata does not use ABI12 upstream-runtime bridge mode");
    }
    if (report.release_or_protect && !report.quickjs_abi12_runtime) {
      add_failure(report, "QuickJS/WASM runtime metadata does not expose the minimal ABI12 execution contract");
    }
    if (report.release_or_protect && !report.quickjs_execute_bytecode_export) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing venom_qjs_execute_bytecode export");
    }
    if (report.release_or_protect && !report.quickjs_bytecode_validate_export) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing venom_qjs_bytecode_validate export");
    }
    if (report.release_or_protect && !report.quickjs_interpreter_exports) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing interpreter-dispatch exports");
    }
    if (report.release_or_protect && !report.quickjs_interpreter_contract_export) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing the minimal interpreter execution contract");
    }
    if (report.release_or_protect && !report.quickjs_semantic_runtime_exports) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing semantic runtime/global slot exports");
    }
    if (report.release_or_protect && !report.quickjs_upstream_exports) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing upstream QuickJS parity exports");
    }
    if (report.release_or_protect && !report.quickjs_upstream_runtime_exports) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing upstream object/exception/module runtime bridge exports");
    }
    if (report.release_or_protect && !report.quickjs_upstream_bytecode_semantics_exports) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing upstream bytecode semantics exports");
    }
    if (report.release_or_protect && !report.quickjs_upstream_intrinsic_semantics_exports) {
      add_failure(report, "QuickJS/WASM runtime metadata is missing upstream intrinsic/object semantics exports");
    }
  }


  const venom::package::Section* native_parity_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::QuickJsNativeParity && venom::package::section_name_matches(section.type, section.name, "quickjs-native-parity.vqnp")) {
      native_parity_section = &section;
      break;
    }
  }
  if (native_parity_section != nullptr) {
    const auto text = std::string(reinterpret_cast<const char*>(native_parity_section->data.data()), native_parity_section->data.size());
    if (text.rfind("VENOM_QUICKJS_NATIVE_PARITY_V3", 0) != 0) {
      add_failure(report, "QuickJS native parity metadata is not v3 upstream-runtime metadata");
    }
    report.quickjs_upstream_core = metadata_value(text, "upstream_interpreter_core");
    if (metadata_value(text, "upstream_bridge") != "enabled") {
      add_failure(report, "QuickJS native parity metadata does not enable upstream bridge");
    }
    if (report.quickjs_upstream_core != "quickjs-upstream-global-host-api-shims-v7") {
      add_failure(report, "QuickJS native parity metadata does not declare the upstream runtime bridge core");
    }
  }

  const venom::package::Section* source_transfer_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::QuickJsSourceTransfer && venom::package::section_name_matches(section.type, section.name, "quickjs-source-transfer.vqst")) {
      source_transfer_section = &section;
      break;
    }
  }
  if (source_transfer_section != nullptr) {
    const auto text = std::string(reinterpret_cast<const char*>(source_transfer_section->data.data()), source_transfer_section->data.size());
    report.quickjs_source_transfer_mode = metadata_value(text, "transfer_mode");
    report.quickjs_oversized_record_path = metadata_value(text, "oversized_record_path");
    const auto oversized_threshold_text = metadata_value(text, "oversized_record_threshold");
    if (!oversized_threshold_text.empty()) {
      try { report.quickjs_oversized_record_threshold = static_cast<std::size_t>(std::stoull(oversized_threshold_text)); }
      catch (...) { report.quickjs_oversized_record_threshold = 0; }
    }
    report.quickjs_opaque_bytecode_transfer = report.quickjs_source_transfer_mode == "opaque-vqjsbc03-native-object" && metadata_value(text, "execute_bytecode") == "venom_qjs_execute_bytecode" && metadata_value(text, "validate_bytecode") == "venom_qjs_bytecode_validate";
    if (report.release_or_protect && !report.quickjs_opaque_bytecode_transfer) {
      add_failure(report, "QuickJS source-transfer metadata does not require opaque native VQJSBC03 bytecode handoff");
    }
    if (report.release_or_protect && metadata_value(text, "encoding") != "native-quickjs-object-v3") {
      add_failure(report, "QuickJS source-transfer metadata uses an unexpected encoding policy");
    }
    if (report.release_or_protect && (report.quickjs_oversized_record_threshold != 786432u ||
        report.quickjs_oversized_record_path != "wasm-native-object-read" ||
        metadata_value(text, "oversized_execution_export") != "venom_qjs_execute_bytecode" ||
        metadata_value(text, "host_source_eval") != "false" ||
        metadata_value(text, "protected_source_execution") != "false")) {
      add_failure(report, "QuickJS oversized-record transfer path is missing or inconsistent");
    }
  }

  const venom::package::Section* layout_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-layout.vlay")) {
      layout_section = &section;
      break;
    }
  }
  if (layout_section != nullptr) {
    report.layout_polymorphic = true;
    const auto text = std::string(reinterpret_cast<const char*>(layout_section->data.data()), layout_section->data.size());
    if (text.rfind("VENOM_PACKAGE_LAYOUT_V1", 0) != 0) {
      add_failure(report, "package layout metadata has invalid header");
    }
    if (metadata_value(text, "enabled") != "true") {
      add_failure(report, "package layout metadata is not enabled");
    }
    if (metadata_value(text, "section_shuffle") != "true") {
      add_failure(report, "package layout metadata does not enable section shuffle");
    }
    if (metadata_value(text, "payload_jitter") != "true") {
      add_failure(report, "package layout metadata does not enable payload jitter");
    }
  }


  const venom::package::Section* lazy_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "lazy-sections.vlazy")) {
      lazy_section = &section;
      break;
    }
  }
  if (lazy_section != nullptr) {
    report.lazy_sections = true;
    const auto text = std::string(reinterpret_cast<const char*>(lazy_section->data.data()), lazy_section->data.size());
    if (text.rfind("VENOM_LAZY_SECTIONS_V1", 0) != 0) {
      add_failure(report, "lazy section metadata has invalid header");
    }
    if (metadata_value(text, "enabled") != "true") {
      add_failure(report, "lazy section metadata is not enabled");
    }
    report.lazy_route_sections = static_cast<std::size_t>(std::stoull(metadata_value(text, "route_count").empty() ? "0" : metadata_value(text, "route_count")));
    report.lazy_script_sections = static_cast<std::size_t>(std::stoull(metadata_value(text, "script_route_count").empty() ? "0" : metadata_value(text, "script_route_count")));
    if (report.lazy_route_sections == 0u) {
      add_failure(report, "lazy section metadata has no route-scoped bytecode sections");
    }
  }

  const venom::package::Section* binding_section = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-binding.vbind")) {
      binding_section = &section;
      break;
    }
  }
  if (binding_section != nullptr) {
    report.package_binding = true;
    const auto text = std::string(reinterpret_cast<const char*>(binding_section->data.data()), binding_section->data.size());
    if (text.rfind("VENOM_PACKAGE_BINDING_V1", 0) != 0) {
      add_failure(report, "package binding metadata has invalid header");
    }
    const auto token_hash = metadata_value(text, "binding_token_sha256");
    const auto asset_records = parse_bound_assets(text);
    report.bound_assets = asset_records.size();
    for (const auto& asset : asset_records) {
      const auto asset_path = report.dist_root / "assets" / asset.name;
      if (!std::filesystem::is_regular_file(asset_path)) {
        add_failure(report, "bound asset is missing: " + asset.role + " => " + asset.name);
        continue;
      }
      const auto digest = venom::package::sha256_hex(read_binary_file(asset_path));
      if (digest != asset.digest) {
        add_failure(report, "bound asset hash mismatch: " + asset.role + " => " + asset.name);
      }
    }
    const auto loader_path = find_loader_asset(report.dist_root);
    if (loader_path.empty()) {
      add_failure(report, "could not find loader asset for package binding verification");
    } else {
      const auto token = extract_loader_binding_token(read_text_file(loader_path));
      if (token.empty()) {
        add_failure(report, "loader is missing bindingToken for package binding");
      } else if (!token_hash.empty() && venom::package::sha256_hex(std::vector<unsigned char>(token.begin(), token.end())) != token_hash) {
        add_failure(report, "loader/package binding token mismatch");
      } else {
        report.loader_binding = true;
      }
    }
  }

  if (report.release_or_protect) {
    const auto html_path = verification_html_path(report.dist_root);
    const auto loader_path = find_loader_asset(report.dist_root);
    const auto style_path = find_style_asset(report.dist_root);
    if (html_path.empty()) {
      add_failure(report, "production dist is missing a protected bootstrap HTML page for SRI verification");
    } else {
      const auto html = read_text_file(html_path);
      if (loader_path.empty()) {
        add_failure(report, "production dist is missing loader asset for SRI verification");
      } else {
        const auto declared = extract_integrity_for_asset(html, public_asset_path(loader_path, report.dist_root));
        const auto expected = venom::package::sha256_sri(read_binary_file(loader_path));
        if (declared.empty()) add_failure(report, "protected bootstrap HTML is missing loader subresource integrity");
        else if (declared != expected) add_failure(report, "loader subresource integrity mismatch");
        else report.loader_sri = true;
      }
      if (style_path.empty()) {
        add_failure(report, "production dist is missing style asset for SRI verification");
      } else {
        const auto declared = extract_integrity_for_asset(html, public_asset_path(style_path, report.dist_root));
        const auto expected = venom::package::sha256_sri(read_binary_file(style_path));
        if (declared.empty()) add_failure(report, "protected bootstrap HTML is missing stylesheet subresource integrity");
        else if (declared != expected) add_failure(report, "stylesheet subresource integrity mismatch");
        else report.style_sri = true;
      }
    }
  }

  for (const auto& section : pkg.sections) {
    if ((section.flags & venom::package::SectionFlagEncrypted) != 0u) {
      ++report.encrypted_sections;
    }
    if ((section.flags & venom::package::SectionFlagCompressed) != 0u) {
      ++report.compressed_sections;
    }
    if (section.type == venom::package::SectionType::Manifest && section.name == "manifest.txt") {
      report.debug_manifest_section = true;
    }
    if (section_has_canonical_leak(section.type, section.name)) {
      report.readable_name_leak = true;
      add_failure(report, "protected package exposes readable internal section name: " + section.name);
    }
    if (section.type == venom::package::SectionType::JavaScript) {
      report.runtime_remote_chunks += count_js_bundle_flag(section.data, 1u << 5u);
      report.vendored_remote_chunks = std::max(report.vendored_remote_chunks, count_js_bundle_flag(section.data, 1u << 7u));
    }
    constexpr std::uint32_t js_chunk_bytecode_encoded = 1u << 6u;
    if (section.type == venom::package::SectionType::JavaScript) {
      // Protected bridge registries are emitted as a dedicated direct VQJSE006
      // JavaScript section. Whole-file and route scripts are stored as entries
      // in VJSB0006 bundles. Count either structural form exactly once; never
      // search arbitrary decoded text for the envelope marker.
      if (payload_starts_with(section.data, "VQJSE006")) {
        ++report.quickjs_bytecode_records;
      } else {
        report.quickjs_bytecode_records += count_js_bundle_flagged_payload_prefix(
            section.data, js_chunk_bytecode_encoded, "VQJSE006");
      }
      if (contains_bytes(section.data, "VQJSBC01") || contains_bytes(section.data, "VQJSBC02") ||
          contains_bytes(section.data, "source-preserving-byte-buffer-record")) {
        add_failure(report, "JavaScript payload contains legacy source-preserving QuickJS byte buffer records");
      }
      if (js_bundle_flagged_payload_contains(section.data, js_chunk_bytecode_encoded, "console.log") ||
          js_bundle_flagged_payload_contains(section.data, js_chunk_bytecode_encoded, "basic site script loaded")) {
        add_failure(report, "protected JavaScript payload contains clear source text instead of native bytecode records");
      }
    }
  }

  if (report.release_or_protect && !report.remote_vendor_metadata) {
    add_failure(report, "protected package is missing remote vendor metadata");
  }
  if (report.release_or_protect && report.runtime_remote_chunks != 0u) {
    add_failure(report, "protected package still contains runtime remote URL script chunks");
  }
  if (report.release_or_protect && report.remote_vendor_count != report.vendored_remote_chunks) {
    add_failure(report, "remote vendor metadata count does not match vendored script chunks");
  }
  if (report.release_or_protect && !report.remote_vendor_lock) {
    add_failure(report, "protected package is missing the vendor lock contract");
  }
  if (report.release_or_protect && report.remote_vendor_lock_entries != report.remote_vendor_unique_count) {
    add_failure(report, "vendor lock entry count does not match the unique vendored dependency count");
  }
  if (report.release_or_protect && report.remote_vendor_unique_count > report.remote_vendor_count) {
    add_failure(report, "unique remote vendor count exceeds vendored script chunk count");
  }

  if (report.release_or_protect && !report.protection_closure_present) {
    add_failure(report, "protected release is missing build-time protection closure metadata");
  }
  if (report.protection_closure_present && report.protection_intents_requested != report.protection_intents_resolved) {
    add_failure(report, "protection closure requested/resolved counts do not match");
  }
  if (report.protection_closure_present && report.quickjs_bytecode_records != report.protection_expected_quickjs_records) {
    add_failure(report, "emitted QuickJS bytecode record count does not match protection closure metadata");
  }

  // Hybrid and browser-only applications are valid release targets. A package
  // may intentionally contain zero protected JavaScript records when every
  // script is selected for the native browser runtime. QuickJS-specific gates
  // below remain conditional on actual VQJSBC03 records.
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_wasm_execution) {
    add_failure(report, "protected package with scripts is missing quickjs-wasm-execution.vqwe metadata");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && report.quickjs_wasm_chunks == 0u) {
    add_failure(report, "QuickJS/WASM execution metadata has no executable chunks");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_opaque_bytecode_transfer) {
    add_failure(report, "protected QuickJS bytecode records must be transferred as opaque bytes into WASM");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_execute_bytecode_export) {
    add_failure(report, "protected QuickJS bytecode records require venom_qjs_execute_bytecode WASM export");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_interpreter_dispatch) {
    add_failure(report, "protected QuickJS bytecode records must execute through WASM interpreter dispatch");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_interpreter_exports) {
    add_failure(report, "protected QuickJS bytecode records require interpreter-dispatch WASM exports");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_upstream_exports) {
    add_failure(report, "protected QuickJS bytecode records require upstream QuickJS parity WASM exports");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_upstream_runtime_exports) {
    add_failure(report, "protected QuickJS bytecode records require upstream object/exception/module runtime bridge WASM exports");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && report.quickjs_upstream_core != "quickjs-upstream-global-host-api-shims-v7") {
    add_failure(report, "protected QuickJS bytecode records require quickjs-upstream-global-host-api-shims-v7 metadata");
  }
  if (report.release_or_protect && report.quickjs_bytecode_records != 0u && !report.quickjs_upstream_bytecode_semantics_exports) {
    add_failure(report, "protected QuickJS bytecode records require upstream bytecode semantics WASM exports");
  }

  if (!report.release_or_protect) {
    add_failure(report, "package is not a protect/release profile artifact");
  }
  if (!report.runtime_hardened) {
    add_failure(report, "package is missing runtime-hardened flag");
  }
  if (!report.integrity_metadata) {
    add_failure(report, "package is missing integrity metadata flag");
  }
  if (report.release_or_protect && !report.package_binding) {
    add_failure(report, "protected package is missing package-binding.vbind metadata");
  }
  if (report.release_or_protect && !report.release_profile) {
    add_failure(report, "protected package is missing release-profile.vrpf threat model metadata");
  }
  if (report.release_or_protect && !report.layout_polymorphic) {
    add_failure(report, "protected package is missing package-layout.vlay polymorphic layout metadata");
  }
  if (report.release_or_protect && !report.lazy_sections) {
    add_failure(report, "protected package is missing lazy-sections.vlazy route-scoped lazy loading metadata");
  }
  if (report.release_or_protect && report.payload_padding_bytes == 0u) {
    add_failure(report, "protected package has no payload layout padding/jitter");
  }
  if (report.release_or_protect && !report.loader_binding) {
    add_failure(report, "protected package is not bound to the shipped loader token");
  }
  if (report.encrypted_sections == 0u) {
    add_failure(report, "package has zero encrypted/protected sections");
  }
  if (report.v_legacy_sections != 0u) {
    add_failure(report, "legacy VSEAL001 sections are not allowed in production verify");
  }
  if (report.external_manifest) {
    add_failure(report, "external assets/asset-manifest.txt is present; use --emit-debug-manifest only for debug builds");
  }
  if (report.debug_manifest_section) {
    add_failure(report, "debug manifest.txt section is present in the protected package");
  }

  const bool require_native = options.security_target == "native" || options.require_audited_crypto;
  const bool require_browser = options.security_target == "browser";
  if (require_native && report.release_profile && report.release_threat_model != "native-private-aead-v1") {
    add_failure(report, "native/audited release profile must use native-private-aead-v1 threat model");
  }
  if (require_native && report.release_profile && !report.release_profile_audited_crypto) {
    add_failure(report, "native/audited release profile must declare audited_crypto=true");
  }
  if (require_native && report.release_profile && !report.release_profile_external_key_required) {
    add_failure(report, "native/audited release profile must require an external package key");
  }
  if (require_native && report.v_sodium_sections == 0u) {
    add_failure(report, "native/audited release requires VSODIUM1 libsodium sections");
  }
  if (require_native && report.v_runtime_sections != 0u) {
    add_failure(report, "native/audited release must not contain browser-runtime VAEAD001 sections");
  }
  if (require_browser && report.release_profile && report.release_threat_model != "browser-client-protection-v1") {
    add_failure(report, "browser release profile must use browser-client-protection-v1 threat model");
  }
  if (require_browser && report.release_profile && report.release_profile_audited_crypto) {
    add_failure(report, "browser release profile must not claim audited secret crypto");
  }
  if (require_browser && report.release_profile && report.release_profile_external_key_required) {
    add_failure(report, "browser release profile must not require a native package key");
  }
  if (require_browser && report.release_profile && !report.release_profile_browser_executable) {
    add_failure(report, "browser release profile must declare browser_executable=true");
  }
  if (require_browser && report.v_runtime_sections == 0u) {
    add_failure(report, "browser release requires VAEAD001 runtime-decodable sections");
  }
  if (require_browser && report.v_sodium_sections != 0u) {
    add_failure(report, "browser release must not contain VSODIUM1 sections because browsers cannot keep the package key secret");
  }
  if (require_browser && report.quickjs_bytecode_records != 0u && report.quickjs_execution_backend != "quickjs-wasm-real") {
    add_failure(report, "browser protected scripts require quickjs-wasm-real execution backend");
  }
  if (require_browser && report.quickjs_host_js_fallback_allowed) {
    add_failure(report, "browser protected release must deny host-JS fallback");
  }
  if (options.require_real_engine) {
    if (!report.quickjs_runtime_real_engine_candidate || report.quickjs_runtime_contract_only || report.quickjs_runtime_scaffold || !report.quickjs_runtime_full_upstream_quickjs) {
      add_failure(report, "runtime verification requires a real upstream QuickJS WASM engine; current artifact is contract/scaffold only");
    }
    if (!report.quickjs_runtime_required_exports_satisfied || report.quickjs_runtime_missing_export_count != 0u) {
      add_failure(report, "runtime verification requires a generated WASM artifact with all ABI12 exports present");
    }
    if (report.quickjs_runtime_artifact_kind != "upstream-quickjs-wasm") {
      add_failure(report, "runtime verification requires artifact_kind=upstream-quickjs-wasm");
    }
  }

  return report;
}

} // namespace

ReleaseCheckReport analyze_package_for_release_internal(const ReleaseCheckOptions& options) {
  return analyze_package_for_release(options);
}

} // namespace venom::compiler
