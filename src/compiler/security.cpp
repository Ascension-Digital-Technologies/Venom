#include "compiler/security.hpp"

#include "package/crypto.hpp"
#include "package/format.hpp"
#include "package/hash.hpp"
#include "package/reader.hpp"

#include <algorithm>
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
#include <vector>

namespace venom::compiler {
namespace {

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to read key file " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), {});
}

std::vector<unsigned char> read_binary_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to read " + path.string());
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

bool contains_bytes(const std::vector<unsigned char>& haystack, const std::string& needle) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return false;
  }
  return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end()) != haystack.end();
}

std::size_t count_bytes(const std::vector<unsigned char>& haystack, const std::string& needle) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return 0;
  }
  std::size_t count = 0;
  auto it = haystack.begin();
  while ((it = std::search(it, haystack.end(), needle.begin(), needle.end())) != haystack.end()) {
    ++count;
    ++it;
  }
  return count;
}

std::uint32_t read_u32_le(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 4u > bytes.size()) return 0u;
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::size_t count_js_bundle_flag(const std::vector<unsigned char>& bytes, std::uint32_t flag) {
  static const std::vector<unsigned char> magic{'V','J','S','B','0','0','0','6'};
  if (bytes.size() < 24u || !std::equal(magic.begin(), magic.end(), bytes.begin())) return 0u;
  const auto count = read_u32_le(bytes, 12u);
  constexpr std::size_t entry_size = 40u;
  constexpr std::size_t flags_offset = 28u;
  if (24u + static_cast<std::size_t>(count) * entry_size > bytes.size()) return 0u;
  std::size_t matches = 0u;
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto flags = read_u32_le(bytes, 24u + static_cast<std::size_t>(i) * entry_size + flags_offset);
    if ((flags & flag) != 0u) ++matches;
  }
  return matches;
}


bool js_bundle_flagged_payload_contains(const std::vector<unsigned char>& bytes,
                                        std::uint32_t required_flag,
                                        const std::string& needle) {
  static const std::vector<unsigned char> magic{'V','J','S','B','0','0','0','6'};
  if (needle.empty() || bytes.size() < 24u || !std::equal(magic.begin(), magic.end(), bytes.begin())) return false;
  const auto count = read_u32_le(bytes, 12u);
  const auto text_size = read_u32_le(bytes, 16u);
  constexpr std::size_t entry_size = 40u;
  constexpr std::size_t code_offset_field = 16u;
  constexpr std::size_t code_size_field = 20u;
  constexpr std::size_t flags_offset = 28u;
  const auto table_end = 24u + static_cast<std::size_t>(count) * entry_size;
  if (table_end > bytes.size() || static_cast<std::size_t>(text_size) > bytes.size() - table_end) return false;
  const auto code_base = table_end + static_cast<std::size_t>(text_size);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto entry = 24u + static_cast<std::size_t>(i) * entry_size;
    const auto flags = read_u32_le(bytes, entry + flags_offset);
    if ((flags & required_flag) == 0u) continue;
    const auto code_offset = static_cast<std::size_t>(read_u32_le(bytes, entry + code_offset_field));
    const auto code_size = static_cast<std::size_t>(read_u32_le(bytes, entry + code_size_field));
    if (code_offset > bytes.size() - code_base || code_size > bytes.size() - code_base - code_offset) return true;
    const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(code_base + code_offset);
    const auto finish = begin + static_cast<std::ptrdiff_t>(code_size);
    if (std::search(begin, finish, needle.begin(), needle.end()) != finish) return true;
  }
  return false;
}


std::filesystem::path resolve_package_path(const std::filesystem::path& target) {
  if (std::filesystem::is_regular_file(target)) {
    return target;
  }
  if (!std::filesystem::is_directory(target)) {
    throw std::runtime_error("release-check target does not exist: " + target.string());
  }
  const auto assets_dir = target / "assets";
  const auto stable = assets_dir / "app" / "app.vbc";
  if (std::filesystem::exists(stable)) {
    return stable;
  }
  const auto legacy_stable = assets_dir / "app.vbc";
  if (std::filesystem::exists(legacy_stable)) {
    return legacy_stable;
  }
  const auto app_dir = assets_dir / "app";
  if (std::filesystem::is_directory(app_dir)) {
    for (const auto& entry : std::filesystem::directory_iterator(app_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const auto filename = entry.path().filename().string();
      if (filename.rfind("app.", 0) == 0 && entry.path().extension() == ".vbc") {
        return entry.path();
      }
    }
  }
  if (std::filesystem::is_directory(assets_dir)) {
    for (const auto& entry : std::filesystem::directory_iterator(assets_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const auto filename = entry.path().filename().string();
      if (filename.rfind("app.", 0) == 0 && entry.path().extension() == ".vbc") {
        return entry.path();
      }
    }
  }
  throw std::runtime_error("release-check could not find assets/app/app.vbc or assets/app/app.<hash>.vbc under " + target.string());
}

std::filesystem::path resolve_dist_root(const std::filesystem::path& target, const std::filesystem::path& package) {
  if (std::filesystem::is_directory(target)) {
    return target;
  }
  const auto parent = package.parent_path();
  if (parent.filename() == "app" && parent.parent_path().filename() == "assets") {
    return parent.parent_path().parent_path();
  }
  if (parent.filename() == "assets") {
    return parent.parent_path();
  }
  return parent;
}

std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

bool section_has_canonical_leak(venom::package::SectionType type, const std::string& name) {
  static const char* forbidden[] = {
    "route-bytecode.vmbc",
    "routes.vbrt",
    "strings.vstr",
    "runtime-policy.vhrd",
    "package-features.vfeat",
    "integrity-auth.vsig",
    "quickjs-release-gate.vqrg",
    "quickjs-bytecode-manifest.vqbm",
    "quickjs-wasm-execution.vqwe",
    "release-profile.vrpf",
    "vm-polymorph.vpol",
    "remote-vendors.vrvd",
  };
  for (const char* canonical : forbidden) {
    if (venom::package::section_name_matches(type, name, canonical) && name == canonical) {
      return true;
    }
  }
  if (name.rfind("route-chunk.", 0) == 0 || name.rfind("script-chunk.", 0) == 0 || name == "lazy-sections.vlazy") {
    return true;
  }
  return false;
}

struct BoundAssetRecord {
  std::string role;
  std::string name;
  std::string digest;
};

std::vector<std::string> split_tabs_local(const std::string& line) {
  std::vector<std::string> parts;
  std::string current;
  std::stringstream input(line);
  while (std::getline(input, current, '	')) {
    parts.push_back(current);
  }
  return parts;
}

std::string metadata_value(const std::string& text, const std::string& key) {
  std::stringstream input(text);
  std::string line;
  const std::string prefix = key + "=";
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind(prefix, 0) == 0) {
      return line.substr(prefix.size());
    }
  }
  return {};
}

std::vector<BoundAssetRecord> parse_bound_assets(const std::string& text) {
  std::vector<BoundAssetRecord> assets;
  std::stringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind("asset\t", 0) != 0) {
      continue;
    }
    const auto parts = split_tabs_local(line);
    if (parts.size() != 4u) {
      throw std::runtime_error("invalid package binding asset row");
    }
    assets.push_back(BoundAssetRecord{parts[1], parts[2], parts[3]});
  }
  return assets;
}

std::filesystem::path find_loader_asset(const std::filesystem::path& dist_root) {
  const auto assets_dir = dist_root / "assets";
  if (!std::filesystem::is_directory(assets_dir)) {
    return {};
  }
  for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto filename = entry.path().filename().string();
    if (filename.rfind("loader", 0) == 0 && entry.path().extension() == ".js") {
      return entry.path();
    }
  }
  return {};
}

std::filesystem::path find_style_asset(const std::filesystem::path& dist_root) {
  const auto style_dir = dist_root / "assets" / "style";
  if (!std::filesystem::is_directory(style_dir)) {
    return {};
  }
  for (const auto& entry : std::filesystem::directory_iterator(style_dir)) {
    if (!entry.is_regular_file()) continue;
    const auto filename = entry.path().filename().string();
    if (filename.rfind("s.", 0) == 0 && entry.path().extension() == ".css") return entry.path();
  }
  return {};
}

std::string public_asset_path(const std::filesystem::path& asset, const std::filesystem::path& dist_root) {
  if (asset.empty()) return {};
  return std::filesystem::relative(asset, dist_root).generic_string();
}

std::string extract_integrity_for_asset(const std::string& html, const std::string& public_path) {
  if (public_path.empty()) return {};
  const auto asset_pos = html.find(public_path);
  if (asset_pos == std::string::npos) return {};
  const auto tag_start = html.rfind('<', asset_pos);
  const auto tag_end = html.find('>', asset_pos);
  if (tag_start == std::string::npos || tag_end == std::string::npos || tag_end <= tag_start) return {};
  const auto tag = html.substr(tag_start, tag_end - tag_start + 1u);
  const std::string marker = "integrity=\"";
  const auto integrity_pos = tag.find(marker);
  if (integrity_pos == std::string::npos) return {};
  const auto value_start = integrity_pos + marker.size();
  const auto value_end = tag.find('"', value_start);
  if (value_end == std::string::npos) return {};
  return tag.substr(value_start, value_end - value_start);
}

std::string extract_loader_binding_token(const std::string& loader_text) {
  const std::string marker = "vbind:";
  const auto marker_start = loader_text.find(marker);
  if (marker_start != std::string::npos) {
    const auto value_start = marker_start + marker.size();
    const auto value_end = loader_text.find_first_of("\"'", value_start);
    if (value_end != std::string::npos && value_end > value_start) {
      return loader_text.substr(value_start, value_end - value_start);
    }
  }
  const std::string needle = "bindingToken: '";
  const auto start = loader_text.find(needle);
  if (start == std::string::npos) return {};
  const auto value_start = start + needle.size();
  const auto end = loader_text.find("'", value_start);
  if (end == std::string::npos) return {};
  return loader_text.substr(value_start, end - value_start);
}

std::uint64_t decoded_payload_padding_bytes(const venom::package::Package& pkg) {
  std::vector<const venom::package::Section*> sections;
  sections.reserve(pkg.sections.size());
  for (const auto& section : pkg.sections) {
    sections.push_back(&section);
  }
  std::sort(sections.begin(), sections.end(), [](const auto* a, const auto* b) {
    return a->offset < b->offset;
  });
  std::uint64_t cursor = pkg.payload_offset;
  std::uint64_t padding = 0;
  for (const auto* section : sections) {
    if (section->offset > cursor) {
      padding += section->offset - cursor;
    }
    const auto end = section->offset + section->stored_size;
    if (end > cursor) {
      cursor = end;
    }
  }
  const auto payload_end = pkg.payload_offset + pkg.payload_size;
  if (payload_end > cursor) {
    padding += payload_end - cursor;
  }
  return padding;
}


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
  std::size_t quickjs_bytecode_records = 0;
  bool quickjs_wasm_execution = false;
  std::string quickjs_execution_backend;
  bool quickjs_host_js_fallback_allowed = false;
  bool quickjs_release_fail_closed = false;
  std::size_t quickjs_wasm_chunks = 0;
  bool quickjs_opaque_bytecode_transfer = false;
  bool quickjs_execute_bytecode_export = false;
  bool quickjs_abi12_runtime = false;
  bool quickjs_status_exports = false;
  bool quickjs_limit_exports = false;
  bool quickjs_bytecode_validation_exports = false;
  bool quickjs_backend_contract_export = false;
  bool quickjs_bytecode_validate_export = false;
  bool quickjs_interpreter_dispatch = false;
  bool quickjs_interpreter_exports = false;
  bool quickjs_interpreter_contract_export = false;
  bool quickjs_semantic_runtime = false;
  bool quickjs_semantic_runtime_exports = false;
  bool quickjs_upstream_parity = false;
  bool quickjs_upstream_exports = false;
  bool quickjs_upstream_runtime_exports = false;
  bool quickjs_upstream_bytecode_semantics_exports = false;
  bool quickjs_upstream_bytecode_semantics = false;
  bool quickjs_upstream_intrinsic_semantics_exports = false;
  bool quickjs_upstream_intrinsic_semantics = false;
  std::string quickjs_upstream_core;
  std::uint32_t quickjs_runtime_abi = 0;
  std::uint32_t quickjs_runtime_package_version = 0;
  std::string quickjs_abi_contract;
  std::string quickjs_source_transfer_mode;
  std::string quickjs_oversized_record_path;
  std::size_t quickjs_oversized_record_threshold = 0;
  std::string quickjs_wasm_runtime_mode;
  std::string quickjs_runtime_implementation;
  std::string quickjs_runtime_claim;
  bool quickjs_runtime_contract_only = false;
  bool quickjs_runtime_scaffold = false;
  bool quickjs_runtime_real_engine_candidate = false;
  bool quickjs_runtime_full_upstream_quickjs = false;
  bool quickjs_runtime_fallback_required = false;
  std::string quickjs_runtime_finish_blocker;
  std::string quickjs_runtime_artifact_kind;
  std::string quickjs_runtime_wasm_sha256;
  bool quickjs_runtime_required_exports_satisfied = false;
  std::size_t quickjs_runtime_missing_export_count = 0;
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
    const auto index_path = report.dist_root / "index.html";
    const auto loader_path = find_loader_asset(report.dist_root);
    const auto style_path = find_style_asset(report.dist_root);
    if (!std::filesystem::is_regular_file(index_path)) {
      add_failure(report, "production dist is missing index.html for SRI verification");
    } else {
      const auto html = read_text_file(index_path);
      if (loader_path.empty()) {
        add_failure(report, "production dist is missing loader asset for SRI verification");
      } else {
        const auto declared = extract_integrity_for_asset(html, public_asset_path(loader_path, report.dist_root));
        const auto expected = venom::package::sha256_sri(read_binary_file(loader_path));
        if (declared.empty()) add_failure(report, "index.html is missing loader subresource integrity");
        else if (declared != expected) add_failure(report, "loader subresource integrity mismatch");
        else report.loader_sri = true;
      }
      if (style_path.empty()) {
        add_failure(report, "production dist is missing style asset for SRI verification");
      } else {
        const auto declared = extract_integrity_for_asset(html, public_asset_path(style_path, report.dist_root));
        const auto expected = venom::package::sha256_sri(read_binary_file(style_path));
        if (declared.empty()) add_failure(report, "index.html is missing stylesheet subresource integrity");
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
    if (section.type == venom::package::SectionType::JavaScript) {
      report.quickjs_bytecode_records += count_bytes(section.data, "VQJSBC03");
      if (contains_bytes(section.data, "VQJSBC01") || contains_bytes(section.data, "VQJSBC02") ||
          contains_bytes(section.data, "source-preserving-byte-buffer-record")) {
        add_failure(report, "JavaScript payload contains legacy source-preserving QuickJS byte buffer records");
      }
      constexpr std::uint32_t js_chunk_bytecode_encoded = 1u << 6u;
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

  // Hybrid and browser-only applications are valid release targets. A package
  // may intentionally contain zero protected JavaScript records when every
  // script is selected for the native browser realm. QuickJS-specific gates
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
    add_failure(report, "legacy VSEAL001 sections are not allowed in production release-check");
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

void print_release_report(const ReleaseCheckReport& report, const ReleaseCheckOptions& options) {
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
  for (unsigned char byte : key) {
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
  const auto report = analyze_package_for_release(options);
  print_release_report(report, options);
  return report.failures.empty();
}

} // namespace venom::compiler
