#pragma once

#include "pipeline/security.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler {

struct ReleaseCheckReport {
  std::filesystem::path package_path;
  std::filesystem::path dist_root;
  std::uint32_t flags = 0;
  std::uint64_t package_hash = 0;
  std::size_t section_count = 0;
  std::size_t encrypted_sections = 0;
  std::size_t compressed_sections = 0;
  std::size_t v_runtime_sections = 0;
  std::size_t v_sodium_sections = 0;
  std::size_t v_legacy_sections = 0;
  std::size_t turbojs_bytecode_records = 0;
  bool protection_closure_present = false;
  std::size_t protection_intents_requested = 0;
  std::size_t protection_intents_resolved = 0;
  std::size_t protection_expected_turbojs_records = 0;
  bool turbojs_wasm_execution = false;
  std::string turbojs_execution_backend;
  bool turbojs_host_js_fallback_allowed = false;
  bool turbojs_release_fail_closed = false;
  std::size_t turbojs_wasm_chunks = 0;
  bool turbojs_opaque_bytecode_transfer = false;
  bool turbojs_execute_bytecode_export = false;
  bool turbojs_abi12_runtime = false;
  bool turbojs_status_exports = false;
  bool turbojs_limit_exports = false;
  bool turbojs_bytecode_validation_exports = false;
  bool turbojs_backend_contract_export = false;
  bool turbojs_bytecode_validate_export = false;
  bool turbojs_interpreter_dispatch = false;
  bool turbojs_interpreter_exports = false;
  bool turbojs_interpreter_contract_export = false;
  bool turbojs_semantic_runtime = false;
  bool turbojs_semantic_runtime_exports = false;
  bool turbojs_upstream_parity = false;
  bool turbojs_upstream_exports = false;
  bool turbojs_upstream_runtime_exports = false;
  bool turbojs_upstream_bytecode_semantics_exports = false;
  bool turbojs_upstream_bytecode_semantics = false;
  bool turbojs_upstream_intrinsic_semantics_exports = false;
  bool turbojs_upstream_intrinsic_semantics = false;
  std::string turbojs_upstream_core;
  std::uint32_t turbojs_runtime_abi = 0;
  std::uint32_t turbojs_runtime_package_version = 0;
  std::string turbojs_abi_contract;
  std::string turbojs_source_transfer_mode;
  std::string turbojs_oversized_record_path;
  std::size_t turbojs_oversized_record_threshold = 0;
  std::string turbojs_wasm_runtime_mode;
  std::string turbojs_runtime_implementation;
  std::string turbojs_runtime_claim;
  bool turbojs_runtime_contract_only = false;
  bool turbojs_runtime_scaffold = false;
  bool turbojs_runtime_real_engine_candidate = false;
  bool turbojs_runtime_full_upstream_turbojs = false;
  bool turbojs_runtime_fallback_required = false;
  std::string turbojs_runtime_finish_blocker;
  std::string turbojs_runtime_artifact_kind;
  std::string turbojs_runtime_wasm_sha256;
  bool turbojs_runtime_required_exports_satisfied = false;
  std::size_t turbojs_runtime_missing_export_count = 0;
  bool external_manifest = false;
  bool release_or_protect = false;
  bool runtime_hardened = false;
  bool integrity_metadata = false;
  bool debug_manifest_section = false;
  bool readable_name_leak = false;
  bool package_binding = false;
  bool loader_binding = false;
  bool loader_sri = false;
  bool style_sri = false;
  bool release_profile = false;
  std::string release_threat_model;
  std::string release_secret_model;
  bool release_profile_audited_crypto = false;
  bool release_profile_external_key_required = false;
  bool release_profile_browser_executable = false;
  bool layout_polymorphic = false;
  bool lazy_sections = false;
  std::size_t lazy_route_sections = 0;
  std::size_t lazy_script_sections = 0;
  std::uint64_t payload_padding_bytes = 0;
  std::size_t bound_assets = 0;
  bool remote_vendor_metadata = false;
  std::size_t remote_vendor_count = 0;
  std::size_t remote_vendor_unique_count = 0;
  bool remote_vendor_lock = false;
  std::size_t remote_vendor_lock_entries = 0;
  std::string remote_vendor_lock_mode;
  std::string remote_vendor_lock_sha256;
  std::size_t unvendored_remote_scripts = 0;
  std::size_t vendored_remote_chunks = 0;
  std::size_t runtime_remote_chunks = 0;
  std::vector<std::string> failures;
};

ReleaseCheckReport analyze_package_for_release_internal(const ReleaseCheckOptions& options);

} // namespace venom::compiler
