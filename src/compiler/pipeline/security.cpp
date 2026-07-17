#include "compiler/pipeline/security.hpp"
#include "compiler/pipeline/security_analysis.hpp"
#include "compiler/core/console.hpp"

#include "package/crypto.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

namespace venom::compiler {
namespace {

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to read key file " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), {});
}

std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

void print_release_report(const ReleaseCheckReport& report, const ReleaseCheckOptions& options) {
  const bool ok = report.failures.empty();
  const std::string operation = options.runtime_verification ? "QuickJS runtime verification" : "Package integrity verification";
  if (options.format == OutputFormat::Json) {
    std::cout << "{\"ok\":" << (ok ? "true" : "false")
              << ",\"operation\":\"" << operation << "\""
              << ",\"package\":\"" << report.package_path.generic_string() << "\""
              << ",\"sections\":" << report.section_count
              << ",\"protectedSections\":" << report.encrypted_sections
              << ",\"runtimeAbi\":" << report.quickjs_runtime_abi
              << ",\"failures\":[";
    for (std::size_t i = 0; i < report.failures.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << "\"" << report.failures[i] << "\"";
    }
    std::cout << "]}\n";
    return;
  }
  if (options.verbosity == 0) {
    if (!ok) print_status("FAIL", operation);
    return;
  }
  if (options.verbosity < 2) {
    if (ok) {
      print_pass(operation + " passed");
      std::cout << "  Package: " << report.package_path.filename().string()
                << " / " << report.section_count << " sections / "
                << report.encrypted_sections << " protected\n";
      if (options.runtime_verification) {
        std::cout << "  Runtime: QuickJS/WASM ABI " << report.quickjs_runtime_abi
                  << " / real engine " << (report.quickjs_runtime_real_engine_candidate ? "confirmed" : "unconfirmed") << "\n";
      }
    } else {
      print_status("FAIL", operation + " failed");
      for (const auto& failure : report.failures) std::cout << "  - " << failure << "\n";
      std::cout << "  Run again with --verbose for complete verification evidence.\n";
    }
    return;
  }
  std::cout << (options.runtime_verification ? "Runtime verification report:\n" : "Protection report:\n")
            << "  target: " << options.security_target << "\n"
            << "  package: " << report.package_path.string() << "\n"
            << "  package_hash: " << hex64(report.package_hash) << "\n"
            << "  sections: " << report.section_count << "\n"
            << "  protected_sections: " << report.encrypted_sections << "\n"
            << "  compressed_sections: " << report.compressed_sections << "\n"
            << "  provider_runtime_sections: " << report.v_runtime_sections << "\n"
            << "  provider_libsodium_sections: " << report.v_sodium_sections << "\n"
            << "  provider_legacy_sections: " << report.v_legacy_sections << "\n"
            << "  quickjs_bytecode_records: " << report.quickjs_bytecode_records << "\n"
            << "  protection_closure_present: " << (report.protection_closure_present ? "yes" : "no") << "\n"
            << "  protection_intents_requested: " << report.protection_intents_requested << "\n"
            << "  protection_intents_resolved: " << report.protection_intents_resolved << "\n"
            << "  protection_expected_quickjs_records: " << report.protection_expected_quickjs_records << "\n"
            << "  quickjs_wasm_execution: " << (report.quickjs_wasm_execution ? "yes" : "no") << "\n"
            << "  quickjs_execution_backend: " << (report.quickjs_execution_backend.empty() ? "none" : report.quickjs_execution_backend) << "\n"
            << "  quickjs_host_js_fallback_allowed: " << (report.quickjs_host_js_fallback_allowed ? "yes" : "no") << "\n"
            << "  quickjs_release_fail_closed: " << (report.quickjs_release_fail_closed ? "yes" : "no") << "\n"
            << "  quickjs_wasm_chunks: " << report.quickjs_wasm_chunks << "\n"
            << "  quickjs_bytecode_boundary: " << (report.quickjs_opaque_bytecode_transfer ? "wasm-owned" : "legacy-or-none") << "\n"
            << "  quickjs_source_transfer: " << (report.quickjs_source_transfer_mode.empty() ? "none" : report.quickjs_source_transfer_mode) << "\n"
            << "  quickjs_oversized_record_threshold: " << report.quickjs_oversized_record_threshold << "\n"
            << "  quickjs_oversized_record_path: " << (report.quickjs_oversized_record_path.empty() ? "none" : report.quickjs_oversized_record_path) << "\n"
            << "  quickjs_wasm_runtime_mode: " << (report.quickjs_wasm_runtime_mode.empty() ? "none" : report.quickjs_wasm_runtime_mode) << "\n"
            << "  quickjs_runtime_implementation: " << (report.quickjs_runtime_implementation.empty() ? "unknown" : report.quickjs_runtime_implementation) << "\n"
            << "  quickjs_runtime_claim: " << (report.quickjs_runtime_claim.empty() ? "unknown" : report.quickjs_runtime_claim) << "\n"
            << "  quickjs_runtime_contract_only: " << (report.quickjs_runtime_contract_only ? "yes" : "no") << "\n"
            << "  quickjs_runtime_scaffold: " << (report.quickjs_runtime_scaffold ? "yes" : "no") << "\n"
            << "  quickjs_runtime_real_engine_candidate: " << (report.quickjs_runtime_real_engine_candidate ? "yes" : "no") << "\n"
            << "  quickjs_runtime_full_upstream_quickjs: " << (report.quickjs_runtime_full_upstream_quickjs ? "yes" : "no") << "\n"
            << "  quickjs_runtime_fallback_required: " << (report.quickjs_runtime_fallback_required ? "yes" : "no") << "\n"
            << "  quickjs_runtime_finish_blocker: " << (report.quickjs_runtime_finish_blocker.empty() ? "none" : report.quickjs_runtime_finish_blocker) << "\n"
            << "  quickjs_runtime_artifact_kind: " << (report.quickjs_runtime_artifact_kind.empty() ? "unknown" : report.quickjs_runtime_artifact_kind) << "\n"
            << "  quickjs_runtime_wasm_sha256: " << (report.quickjs_runtime_wasm_sha256.empty() ? "unknown" : report.quickjs_runtime_wasm_sha256) << "\n"
            << "  quickjs_runtime_required_exports_satisfied: " << (report.quickjs_runtime_required_exports_satisfied ? "yes" : "no") << "\n"
            << "  quickjs_runtime_missing_export_count: " << report.quickjs_runtime_missing_export_count << "\n"
            << "  quickjs_execute_bytecode_export: " << (report.quickjs_execute_bytecode_export ? "yes" : "no") << "\n"
            << "  quickjs_runtime_abi: " << report.quickjs_runtime_abi << "\n"
            << "  quickjs_runtime_package_version: " << report.quickjs_runtime_package_version << "\n"
            << "  quickjs_abi_contract: " << (report.quickjs_abi_contract.empty() ? "none" : report.quickjs_abi_contract) << "\n"
            << "  quickjs_abi12_runtime: " << (report.quickjs_abi12_runtime ? "yes" : "no") << "\n"
            << "  quickjs_status_exports: " << (report.quickjs_status_exports ? "yes" : "no") << "\n"
            << "  quickjs_limit_exports: " << (report.quickjs_limit_exports ? "yes" : "no") << "\n"
            << "  quickjs_bytecode_validate_export: " << (report.quickjs_bytecode_validate_export ? "yes" : "no") << "\n"
            << "  quickjs_backend_contract_export: " << (report.quickjs_backend_contract_export ? "yes" : "no") << "\n"
            << "  quickjs_interpreter_dispatch: " << (report.quickjs_interpreter_dispatch ? "yes" : "no") << "\n"
            << "  quickjs_interpreter_exports: " << (report.quickjs_interpreter_exports ? "yes" : "no") << "\n"
            << "  quickjs_interpreter_contract_export: " << (report.quickjs_interpreter_contract_export ? "yes" : "no") << "\n"
            << "  quickjs_semantic_runtime: " << (report.quickjs_semantic_runtime ? "yes" : "no") << "\n"
            << "  quickjs_semantic_runtime_exports: " << (report.quickjs_semantic_runtime_exports ? "yes" : "no") << "\n"
            << "  quickjs_upstream_parity: " << (report.quickjs_upstream_parity ? "yes" : "no") << "\n"
            << "  quickjs_upstream_core: " << (report.quickjs_upstream_core.empty() ? "none" : report.quickjs_upstream_core) << "\n"
            << "  quickjs_upstream_exports: " << (report.quickjs_upstream_exports ? "yes" : "no") << "\n"
            << "  quickjs_upstream_runtime_exports: " << (report.quickjs_upstream_runtime_exports ? "yes" : "no") << "\n"
            << "  quickjs_upstream_bytecode_semantics: " << (report.quickjs_upstream_bytecode_semantics ? "yes" : "no") << "\n"
            << "  quickjs_upstream_bytecode_semantics_exports: " << (report.quickjs_upstream_bytecode_semantics_exports ? "yes" : "no") << "\n"
            << "  quickjs_upstream_intrinsic_semantics: " << (report.quickjs_upstream_intrinsic_semantics ? "yes" : "no") << "\n"
            << "  quickjs_upstream_intrinsic_semantics_exports: " << (report.quickjs_upstream_intrinsic_semantics_exports ? "yes" : "no") << "\n"
            << "  package_binding: " << (report.package_binding ? "yes" : "no") << "\n"
            << "  loader_binding: " << (report.loader_binding ? "yes" : "no") << "\n"
            << "  loader_sri: " << (report.loader_sri ? "yes" : "no") << "\n"
            << "  style_sri: " << (report.style_sri ? "yes" : "no") << "\n"
            << "  release_profile: " << (report.release_profile ? "yes" : "no") << "\n"
            << "  threat_model: " << (report.release_threat_model.empty() ? "none" : report.release_threat_model) << "\n"
            << "  secret_model: " << (report.release_secret_model.empty() ? "none" : report.release_secret_model) << "\n"
            << "  release_profile_audited_crypto: " << (report.release_profile_audited_crypto ? "yes" : "no") << "\n"
            << "  release_profile_external_key_required: " << (report.release_profile_external_key_required ? "yes" : "no") << "\n"
            << "  layout_polymorphic: " << (report.layout_polymorphic ? "yes" : "no") << "\n"
            << "  lazy_sections: " << (report.lazy_sections ? "yes" : "no") << "\n"
            << "  lazy_route_sections: " << report.lazy_route_sections << "\n"
            << "  lazy_script_sections: " << report.lazy_script_sections << "\n"
            << "  payload_padding_bytes: " << report.payload_padding_bytes << "\n"
            << "  bound_assets: " << report.bound_assets << "\n"
            << "  remote_vendor_metadata: " << (report.remote_vendor_metadata ? "yes" : "no") << "\n"
            << "  remote_vendor_count: " << report.remote_vendor_count << "\n"
            << "  remote_vendor_unique_count: " << report.remote_vendor_unique_count << "\n"
            << "  remote_vendor_lock: " << (report.remote_vendor_lock ? "yes" : "no") << "\n"
            << "  remote_vendor_lock_mode: " << (report.remote_vendor_lock_mode.empty() ? "none" : report.remote_vendor_lock_mode) << "\n"
            << "  remote_vendor_lock_entries: " << report.remote_vendor_lock_entries << "\n"
            << "  remote_vendor_lock_sha256: " << (report.remote_vendor_lock_sha256.empty() ? "none" : report.remote_vendor_lock_sha256) << "\n"
            << "  vendored_remote_chunks: " << report.vendored_remote_chunks << "\n"
            << "  runtime_remote_chunks: " << report.runtime_remote_chunks << "\n"
            << "  external_debug_manifest: " << (report.external_manifest ? "yes" : "no") << "\n"
            << "  debug_manifest_section: " << (report.debug_manifest_section ? "yes" : "no") << "\n"
            << "  readable_section_names: " << (report.readable_name_leak ? "yes" : "no") << "\n"
            << "  runtime_hardened: " << (report.runtime_hardened ? "yes" : "no") << "\n"
            << "  integrity_metadata: " << (report.integrity_metadata ? "yes" : "no") << "\n"
            << "  require_audited_crypto: " << (options.require_audited_crypto ? "yes" : "no") << "\n"
            << "  require_real_engine: " << (options.require_real_engine ? "yes" : "no") << "\n";
  if (report.failures.empty()) {
    std::cout << "  release_status: PASS\n";
  } else {
    std::cout << "  release_status: FAIL\n";
    for (const auto& failure : report.failures) {
      std::cout << "  failure: " << failure << "\n";
    }
  }
}

} // namespace

void load_package_key_file_for_process(const std::filesystem::path& key_file) {
  venom::package::set_package_key_hex_override(read_text_file(key_file));
}

bool generate_key_file(const KeygenOptions& options) {
  std::array<unsigned char, 32> key{};
  std::random_device rd;
  for (auto& byte : key) {
    byte = static_cast<unsigned char>(rd() & 0xffu);
  }

  std::ostringstream hex;
  hex << std::hex << std::setfill('0');
  for (const unsigned char raw_byte : key) {
    const auto byte = static_cast<unsigned char>(raw_byte);
    hex << std::setw(2) << static_cast<unsigned int>(byte);
  }
  const auto text = hex.str() + "\n";

  if (options.output.empty()) {
    std::cout << text;
    return true;
  }
  if (std::filesystem::exists(options.output) && !options.force) {
    throw std::runtime_error("key file already exists; pass --force to overwrite: " + options.output.string());
  }
  if (!options.output.parent_path().empty()) {
    std::filesystem::create_directories(options.output.parent_path());
  }
  std::ofstream out(options.output, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to write key file " + options.output.string());
  }
  out << text;
  if (!out) {
    throw std::runtime_error("failed while writing key file " + options.output.string());
  }
  std::cout << "venom: wrote package key to " << options.output.string() << "\n";
  return true;
}

bool release_check(const ReleaseCheckOptions& options) {
  const auto report = analyze_package_for_release_internal(options);
  print_release_report(report, options);
  return report.failures.empty();
}

} // namespace venom::compiler
