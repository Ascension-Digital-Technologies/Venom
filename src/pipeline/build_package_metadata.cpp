#include "venom/base/error.hpp"
#include "venom/internal/pipeline/build_package_metadata.hpp"
#include "venom/internal/pipeline/build_support.hpp"
#include "venom/internal/pipeline/assets.hpp"
#include "venom/graph/module_graph.hpp"

#include "venom/package/crypto.hpp"
#include "venom/package/hash.hpp"
#include "venom/quickjs/abi.hpp"
#include "venom/quickjs/bytecode.hpp"
#include "venom/vm/encoder.hpp"

#include <algorithm>
#include <sstream>
#include <random>
#include <stdexcept>
#include <unordered_map>

namespace venom::compiler::build_package_detail {
using namespace venom::compiler::build_detail;

ReleaseBuildPolicy resolve_release_build_policy(const Profile& profile,
                                                const BuildOptions& options,
                                                std::size_t script_count) {
  ReleaseBuildPolicy policy;
  policy.release_like = profile.kind == ProfileKind::Prod || profile.fail_closed || options.strict_release;
  policy.has_scripts = script_count != 0;
  policy.backend = options.quickjs_backend.empty() ? "scaffold" : options.quickjs_backend;
  policy.backend_is_scaffold = policy.backend == "scaffold";
  policy.backend_is_real_browser = policy.backend == "wasm-real";
  policy.fallback_allowed = options.allow_host_fallback && !policy.release_like && !options.deny_host_fallback && !options.strict_release;
  policy.fallback_denied = options.deny_host_fallback || options.strict_release || profile.fail_closed || (profile.kind == ProfileKind::Prod && !policy.fallback_allowed);
  policy.unsafe_release_fallback = policy.release_like && policy.fallback_allowed && policy.has_scripts && policy.backend_is_scaffold;

  if (policy.release_like && policy.has_scripts && policy.backend_is_scaffold && !policy.fallback_allowed) {
    policy.decision = "deny-build";
    policy.reason = "script-bearing release package requires host-JS fallback because QuickJS backend is scaffold";
  } else if (policy.release_like && policy.has_scripts && policy.backend == "native") {
    policy.decision = "deny-build";
    policy.reason = "native QuickJS is compile-time only and cannot execute browser scripts in release output";
  } else if (policy.release_like && policy.has_scripts && policy.backend == "wasm-real") {
    policy.decision = "allow-build";
    policy.reason = "script-bearing release package uses fail-closed QuickJS/WASM backend contract";
  } else if (policy.unsafe_release_fallback) {
    policy.decision = "allow-unsafe-fallback";
    policy.reason = "explicit host fallback override accepted for compatibility package";
  } else {
    policy.decision = "allow-build";
    policy.reason = policy.has_scripts ? "fallback policy is compatible with non-release profile" : "site has no packaged script chunks";
  }
  return policy;
}

void enforce_release_build_policy(const ReleaseBuildPolicy& policy, std::size_t script_count) {
  if (policy.decision == "deny-build") {
    std::ostringstream err;
    err << "release build denied: site contains " << script_count
        << " script chunk" << (script_count == 1 ? "" : "s")
        << ", quickjs backend '" << policy.backend << "' cannot satisfy strict release execution ("
        << policy.reason << "). Protected and release profiles cannot enable host fallback; use --quickjs-backend wasm-real for the protected QuickJS/WASM execution backend.";
    raise_error("VENOM-E5000", err.str());
  }
}

std::string make_release_build_policy_metadata(const Profile& profile,
                                               const BuildOptions& options,
                                               const ReleaseBuildPolicy& policy,
                                               std::size_t script_count) {
  std::ostringstream out;
  out << "VENOM_RELEASE_BUILD_POLICY_V1\n"
      << "version=1\n"
      << "package_version=" << venom::package::kVersion << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "quickjs_backend=" << policy.backend << "\n"
      << "script_count=" << script_count << "\n"
      << "release_like=" << (policy.release_like ? "true" : "false") << "\n"
      << "fallback_allowed=" << (policy.fallback_allowed ? "true" : "false") << "\n"
      << "fallback_denied=" << (policy.fallback_denied ? "true" : "false") << "\n"
      << "unsafe_release_fallback=" << (policy.unsafe_release_fallback ? "true" : "false") << "\n"
      << "decision=" << policy.decision << "\n"
      << "reason=" << policy.reason << "\n"
      << "strict_release_option=" << (options.strict_release ? "true" : "false") << "\n"
      << "allow_host_fallback_option=" << (options.allow_host_fallback ? "true" : "false") << "\n"
      << "deny_host_fallback_option=" << (options.deny_host_fallback ? "true" : "false") << "\n";
  return out.str();
}

std::string selected_section_sealer_name(const Profile& profile, const std::string& crypto_provider) {
  if (!profile.crypto_provider_ready) {
    return "none";
  }
  if (crypto_provider == "libsodium") {
    return venom::package::libsodium_aead_provider_name();
  }
  return "venom-aead-section-v1";
}

venom::package::SectionCryptoProvider selected_writer_crypto_provider(const std::string& crypto_provider) {
  if (crypto_provider == "libsodium") {
    return venom::package::SectionCryptoProvider::LibsodiumXChaCha20Poly1305;
  }
  return venom::package::SectionCryptoProvider::RuntimeSoftware;
}

std::string stored_section_name_for_metadata(const Profile& profile, venom::package::SectionType type, const std::string& name) {
  if (profile.strip_debug_metadata && venom::package::should_redact_section_name(type)) {
    return venom::package::protected_section_alias(type, name);
  }
  return name;
}

std::string make_package_binding_token(bool deterministic, const std::string& seed) {
  if (deterministic) {
    return venom::package::sha256_hex(bytes_from_string("venom-binding-token-v1|" + seed));
  }
  std::vector<unsigned char> bytes(32u);
  std::random_device rd;
  for (auto& byte : bytes) {
    byte = static_cast<unsigned char>(rd() & 0xffu);
  }
  return venom::package::sha256_hex(bytes);
}

std::string package_section_order_digest(const Profile& profile, const std::vector<PendingPackageSection>& sections) {
  std::ostringstream canonical;
  for (const auto& section : sections) {
    canonical << static_cast<std::uint32_t>(section.type) << '\t'
              << stored_section_name_for_metadata(profile, section.type, section.name) << '\t'
              << section.data.size() << '\t'
              << venom::package::sha256_hex(section.data) << '\n';
  }
  const auto text = canonical.str();
  return venom::package::sha256_hex(bytes_from_string(text));
}

std::string make_package_layout_metadata(const Profile& profile,
                                         const BuildOptions& options,
                                         const venom::vm::PolymorphicPlan& poly,
                                         const std::vector<PendingPackageSection>& sections,
                                         std::uint32_t max_padding) {
  std::ostringstream out;
  out << "VENOM_PACKAGE_LAYOUT_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "enabled=" << (profile.polymorphic ? "true" : "false") << "\n"
      << "section_shuffle=" << (profile.shuffle_sections ? "true" : "false") << "\n"
      << "payload_jitter=true\n"
      << "payload_jitter_max=" << max_padding << "\n"
      << "layout_seed_commitment=" << poly.seed_commitment << "\n"
      << "section_name_redaction=" << ((profile.strip_debug_metadata) ? "true" : "false") << "\n"
      << "section_count_before_layout=" << sections.size() << "\n"
      << "section_order_digest=" << package_section_order_digest(profile, sections) << "\n";
  return out.str();
}

void append_u32_local(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

std::vector<unsigned char> make_quickjs_abi_fingerprint() {
  // VQAF0001 + version + FNV-1a fingerprint of the one supported release ABI.
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V','Q','A','F','0','0','0','1'});
  append_u32_local(out, 1u);
  append_u32_local(out, 0x3b9ace76u);
  append_u32_local(out, 12u);
  append_u32_local(out, 16u);
  return out;
}

std::vector<unsigned char> make_release_diversification_table(const venom::vm::PolymorphicPlan& poly) {
  // Binary-only release contract. Logical host-call and field identifiers are
  // mapped to per-build wire identifiers, preventing stable cross-build ABI
  // signatures while retaining deterministic builds when requested.
  constexpr std::uint32_t kHostCallCount = 89u;
  constexpr std::uint32_t kResultFieldCount = 8u;
  std::vector<std::uint32_t> host_ids(kHostCallCount);
  std::vector<std::uint32_t> field_ids(kResultFieldCount);
  for (std::uint32_t i = 0; i < kHostCallCount; ++i) host_ids[i] = i + 1u;
  for (std::uint32_t i = 0; i < kResultFieldCount; ++i) field_ids[i] = i;
  venom::vm::DiversificationRng host_rng(poly, "quickjs-host-wire-map");
  venom::vm::DiversificationRng field_rng(poly, "quickjs-result-field-map");
  std::shuffle(host_ids.begin(), host_ids.end(), host_rng);
  std::shuffle(field_ids.begin(), field_ids.end(), field_rng);

  std::vector<unsigned char> out;
  out.insert(out.end(), {'V','R','D','I','V','0','0','3'});
  append_u32_local(out, 3u);
  append_u32_local(out, poly.seed ^ 0xD1A5EED3u);
  append_u32_local(out, kHostCallCount);
  append_u32_local(out, kResultFieldCount);
  for (std::uint32_t logical = 0; logical < kHostCallCount; ++logical) {
    append_u32_local(out, logical + 1u);
    append_u32_local(out, host_ids[logical]);
  }
  for (std::uint32_t logical = 0; logical < kResultFieldCount; ++logical) {
    append_u32_local(out, logical);
    append_u32_local(out, field_ids[logical]);
  }
  return out;
}

std::vector<unsigned char> make_single_route_bytecode_section(const venom::vm::Program& program,
                                                              const venom::vm::PolymorphicPlan& poly) {
  const auto encoded = venom::vm::encode_program(program, poly);
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'B', 'C', 'O', 'D', 'E', '0', '3'});
  append_u32_local(out, 1);
  append_u32_local(out, 16);
  append_u32_local(out, static_cast<std::uint32_t>(program.size()));
  append_u32_local(out, 0);
  out.insert(out.end(), encoded.begin(), encoded.end());
  return out;
}


std::vector<unsigned char> encode_route_script_bundle(const std::vector<JsChunk>& chunks, const graph::CanonicalModuleGraph& module_graph, bool opaque_metadata) {
  struct Entry {
    std::uint32_t route_offset = 0;
    std::uint32_t route_size = 0;
    std::uint32_t source_offset = 0;
    std::uint32_t source_size = 0;
    std::uint32_t code_offset = 0;
    std::uint32_t code_size = 0;
    std::uint32_t order = 0;
    std::uint32_t flags = 0;
    std::uint32_t kind = 0;
    std::uint32_t reserved = 0;
  };
  std::vector<Entry> entries;
  std::vector<unsigned char> text_blob;
  std::vector<unsigned char> code_blob;
  std::vector<venom::quickjs::ModuleSourceRecord> module_sources;
  entries.reserve(chunks.size());
  module_sources.reserve(chunks.size());

  std::vector<std::string> stored_sources;
  stored_sources.reserve(chunks.size());
  for (const auto& chunk : chunks) {
    const auto* module = module_graph.find_by_source(chunk.route, chunk.source);
    if (module == nullptr) raise_error("VENOM-E5000", "canonical module graph is missing route source: " + chunk.source);
    stored_sources.push_back(module->runtime_source);
  }

  std::vector<std::string> compiled_sources;
  compiled_sources.reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u && (chunk.flags & JsChunkModule) != 0u) {
      const auto* module = module_graph.find_by_source(chunk.route, chunk.source);
      if (module == nullptr) raise_error("VENOM-E5000", "canonical module graph is missing browser route source: " + chunk.source);
      compiled_sources.push_back(graph::browser_module_identity_prefix(*module) + graph::browser_module_map_prefix(module_graph, *module) + chunk.code);
    } else {
      compiled_sources.push_back(chunk.code);
    }
  }

  for (std::size_t chunk_index = 0; chunk_index < chunks.size(); ++chunk_index) {
    const auto& chunk = chunks[chunk_index];
    if ((chunk.flags & JsChunkModule) == 0u) continue;
    const std::string& compile_name = stored_sources[chunk_index];
    module_sources.push_back({chunk.source, compile_name, compiled_sources[chunk_index]});
  }
  for (std::size_t chunk_index = 0; chunk_index < chunks.size(); ++chunk_index) {
    const auto& chunk = chunks[chunk_index];
    Entry entry;
    // Route labels remain executable routing keys. Hiding them without a separate
    // authenticated route-id map prevents the browser runtime from selecting the
    // route's script bundle. Source labels are hardened independently below.
    const std::string route_label = chunk.route;
    const std::string& source_label = stored_sources[chunk_index];
    entry.route_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.route_size = static_cast<std::uint32_t>(route_label.size());
    text_blob.insert(text_blob.end(), route_label.begin(), route_label.end());
    entry.source_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.source_size = static_cast<std::uint32_t>(source_label.size());
    text_blob.insert(text_blob.end(), source_label.begin(), source_label.end());
    const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    std::vector<unsigned char> payload;
    if (browser_side) {
      // Browser-runtime chunks must remain browser-executable source. Encoding
      // them as QuickJS bytecode leaves chunk.code empty at runtime and causes
      // the browser executor to silently skip the script.
      const auto& browser_source = compiled_sources[chunk_index];
      payload.assign(browser_source.begin(), browser_source.end());
    } else {
      payload = venom::quickjs::compile_native_quickjs_bytecode(
          chunk.code,
          source_label,
          is_module,
          opaque_metadata,
          is_module ? &module_sources : nullptr);
    }
    entry.code_offset = static_cast<std::uint32_t>(code_blob.size());
    entry.code_size = static_cast<std::uint32_t>(payload.size());
    code_blob.insert(code_blob.end(), payload.begin(), payload.end());
    entry.order = chunk.order;
    entry.flags = browser_side ? (chunk.flags & ~JsChunkBytecodeEncoded)
                               : (chunk.flags | JsChunkBytecodeEncoded);
    entry.kind = chunk.kind;
    entries.push_back(entry);
  }
  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'J', 'S', 'B', '0', '0', '0', '6'});
  append_u32_local(out, 1);
  append_u32_local(out, static_cast<std::uint32_t>(entries.size()));
  append_u32_local(out, static_cast<std::uint32_t>(text_blob.size()));
  append_u32_local(out, 0);
  for (const auto& entry : entries) {
    append_u32_local(out, entry.route_offset);
    append_u32_local(out, entry.route_size);
    append_u32_local(out, entry.source_offset);
    append_u32_local(out, entry.source_size);
    append_u32_local(out, entry.code_offset);
    append_u32_local(out, entry.code_size);
    append_u32_local(out, entry.order);
    append_u32_local(out, entry.flags);
    append_u32_local(out, entry.kind);
    append_u32_local(out, entry.reserved);
  }
  out.insert(out.end(), text_blob.begin(), text_blob.end());
  out.insert(out.end(), code_blob.begin(), code_blob.end());
  return out;
}

std::string lazy_route_section_name(const std::string& route) {
  return "route-chunk." + short_hash_hex(venom::package::fnv1a64(bytes_from_string(route)), 16) + ".vmbc";
}

std::string lazy_script_section_name(const std::string& route) {
  return "script-chunk." + short_hash_hex(venom::package::fnv1a64(bytes_from_string("script|" + route)), 16) + ".vjsb";
}

std::string make_lazy_sections_metadata(const Profile& profile,
                                        const BuildOptions& options,
                                        const std::vector<LazySectionPlanRow>& rows) {
  std::uint32_t route_count = 0;
  std::uint32_t script_route_count = 0;
  for (const auto& row : rows) {
    if (!row.route_section.empty()) ++route_count;
    if (!row.script_section.empty()) ++script_route_count;
  }
  std::ostringstream out;
  out << "VENOM_LAZY_SECTIONS_V1\n"
      << "version=1\n"
      << "enabled=true\n"
      << "mode=route-scoped-decode-on-first-use\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "verify_on_decode=true\n"
      << "route_count=" << route_count << "\n"
      << "script_route_count=" << script_route_count << "\n";
  for (const auto& row : rows) {
    if (!row.route_section.empty()) {
      out << "route\t" << row.route << "\t" << row.route_section << "\t"
          << row.instruction_count << "\t" << row.route_bytecode_size << "\n";
    }
    if (!row.script_section.empty()) {
      out << "script\t" << row.route << "\t" << row.script_section << "\t" << row.script_count << "\n";
    }
  }
  return out.str();
}

std::string release_threat_model_id(const BuildOptions& options) {
  if (options.security_target == "native" || options.crypto_provider == "libsodium" || options.require_audited_crypto) {
    return "native-private-aead-v1";
  }
  return "browser-client-protection-v1";
}

std::string release_secret_model(const BuildOptions& options) {
  if (options.security_target == "native" || options.crypto_provider == "libsodium" || options.require_audited_crypto) {
    return "external-32-byte-package-key-required";
  }
  return "browser-runtime-no-hidden-secret";
}

std::string make_release_profile_metadata(const Profile& profile,
                                          const BuildOptions& options,
                                          const ReleaseBuildPolicy& release_policy,
                                          std::size_t script_count) {
  const bool native_target = options.security_target == "native" || options.crypto_provider == "libsodium" || options.require_audited_crypto;
  const bool browser_target = !native_target;
  const bool audited_crypto = options.crypto_provider == "libsodium";
  std::ostringstream out;
  out << "VENOM_RELEASE_PROFILE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "security_target=" << (native_target ? "native" : (options.security_target == "browser" ? "browser" : options.security_target)) << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "quickjs_backend=" << release_policy.backend << "\n"
      << "quickjs_execution_backend=" << (release_policy.backend == "wasm-real" ? "quickjs-wasm-real" : release_policy.backend) << "\n"
      << "wasm_backend_required=" << ((release_policy.backend == "wasm-real" && script_count != 0u) ? "true" : "false") << "\n"
      << "host_js_fallback_allowed=" << ((release_policy.fallback_allowed && release_policy.backend != "wasm-real") ? "true" : "false") << "\n"
      << "script_count=" << script_count << "\n"
      << "threat_model=" << release_threat_model_id(options) << "\n"
      << "secret_material_model=" << release_secret_model(options) << "\n"
      << "crypto_provider=" << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
      << "audited_crypto=" << (audited_crypto ? "true" : "false") << "\n"
      << "browser_executable=" << (browser_target ? "true" : "false") << "\n"
      << "native_private_crypto=" << (audited_crypto ? "true" : "false") << "\n"
      << "runtime_decrypts_package=" << (browser_target ? "true" : "false") << "\n"
      << "external_key_required=" << (native_target ? "true" : "false") << "\n"
      << "release_check_required=true\n"
      << "debug_metadata_allowed=false\n"
      << "external_manifest_allowed=false\n"
      << "source_preserving_js_allowed=false\n"
      << "remote_script_policy=build-time-vendor\n"
      << "unvendored_remote_scripts_allowed=false\n"
      << "runtime_remote_script_network_required=false\n"
      << "package_binding_required=true\n"
      << "layout_polymorphism_required=true\n"
      << "lazy_sections_required=true\n"
      << "readable_internal_section_names_allowed=false\n"
      << "legacy_sealer_allowed=false\n"
      << "server_secret_claim=false\n"
      << "tamper_response=fail-closed\n"
      << "native_key_handling=" << (native_target ? "key-file-or-env-only" : "not-applicable") << "\n"
      << "browser_secret_warning=" << (browser_target ? "browser-protect cannot hide secrets from the browser runtime" : "not-browser-runnable") << "\n";
  return out.str();
}

std::string make_package_binding_metadata(const Profile& profile,
                                          const BuildOptions& options,
                                          const std::string& binding_token,
                                          const std::string& runtime_name,
                                          const std::string& runtime,
                                          const std::string& runtime_wasm_name,
                                          const std::vector<unsigned char>& runtime_wasm_bytes,
                                          const std::string& style_name,
                                          const std::string& css,
                                          const std::string& quickjs_engine_name,
                                          const std::string& quickjs_engine_module,
                                          const std::string& quickjs_wasm_name,
                                          const std::vector<unsigned char>& quickjs_wasm_bytes,
                                          const std::string& worker_name,
                                          const std::string& worker_runtime) {
  struct BoundAsset {
    std::string role;
    std::string name;
    std::string digest;
  };
  std::vector<BoundAsset> assets;
  assets.push_back({"runtime", runtime_name, venom::package::sha256_hex(bytes_from_string(runtime))});
  if (!runtime_wasm_name.empty()) assets.push_back({"runtime_wasm", runtime_wasm_name, venom::package::sha256_hex(runtime_wasm_bytes)});
  assets.push_back({"style", style_name, venom::package::sha256_hex(bytes_from_string(css))});
  assets.push_back({"quickjs_engine", quickjs_engine_name, venom::package::sha256_hex(bytes_from_string(quickjs_engine_module))});
  assets.push_back({"quickjs_wasm", quickjs_wasm_name, venom::package::sha256_hex(quickjs_wasm_bytes)});
  if (!worker_name.empty()) assets.push_back({"worker", worker_name, venom::package::sha256_hex(bytes_from_string(worker_runtime))});

  std::ostringstream out;
  out << "VENOM_PACKAGE_BINDING_V1\n"
      << "version=1\n"
      << "enabled=true\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "crypto_provider=" << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
      << "hardened=" << (profile.runtime_hardened || profile.fail_closed ? "true" : "false") << "\n"
      << "binding_token_sha256=" << venom::package::sha256_hex(bytes_from_string(binding_token)) << "\n"
      << "asset_count=" << assets.size() << "\n";
  for (const auto& asset : assets) {
    out << "asset\t" << asset.role << "\t" << asset.name << "\t" << asset.digest << "\n";
  }
  return out.str();
}

std::string make_integrity_metadata(const Profile& profile, const std::string& runtime_mode, const std::vector<PendingPackageSection>& sections, const std::string& crypto_provider) {
  std::ostringstream out;
  out << "VENOM_INTEGRITY_V1\n"
      << "provider=sha256-software-v1\n"
      << "scope=decoded-sections\n"
      << "aead_provider=" << selected_section_sealer_name(profile, crypto_provider) << "\n"
      << "section_sealer=" << selected_section_sealer_name(profile, crypto_provider) << "\n"
      << "section_name_redaction=" << ((profile.strip_debug_metadata) ? "true" : "false") << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "wasm_runtime=" << (runtime_mode == "wasm" ? "true" : "false") << "\n"
      << "section_count=" << sections.size() << "\n";
  for (const auto& section : sections) {
    out << "section\t"
        << static_cast<std::uint32_t>(section.type) << "\t"
        << stored_section_name_for_metadata(profile, section.type, section.name) << "\t"
        << section.data.size() << "\t"
        << venom::package::sha256_hex(section.data) << "\n";
  }
  return out.str();
}

bool package_feature_required_in_release(const PendingPackageSection& section) {
  if (section.type == venom::package::SectionType::Asset ||
      section.type == venom::package::SectionType::Css ||
      section.type == venom::package::SectionType::DomTemplates) {
    return false;
  }
  if (section.name == "profile-diagnostics.txt" || section.name == "script-diagnostics.txt" ||
      section.name == "bundle-preview.js" || section.name == "quickjs-probe.txt" ||
      section.name == "quickjs-bridge-plan.txt") {
    return false;
  }
  return true;
}

std::string make_package_features_metadata(const Profile& profile,
                                           const std::string& runtime_mode,
                                           const std::vector<PendingPackageSection>& sections,
                                           const std::string& crypto_provider) {
  std::ostringstream out;
  std::uint32_t feature_count = 0;
  std::uint32_t release_required_count = 0;
  for (const auto& section : sections) {
    if (section.type == venom::package::SectionType::PackageFeatures ||
        (section.type == venom::package::SectionType::Integrity && section.name == "integrity-auth.vsig")) {
      continue;
    }
    ++feature_count;
    if (package_feature_required_in_release(section)) {
      ++release_required_count;
    }
  }

  out << "VENOM_PACKAGE_FEATURES_V2\n"
      << "version=2\n"
      << "package_version=" << venom::package::kVersion << "\n"
      << "runtime_abi=" << venom::package::kRuntimeAbi << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "flag_model=profile-flags-plus-feature-table\n"
      << "legacy_flags_scope=coarse-profile-and-runtime-only\n"
      << "integrity_provider_ready=" << (profile.integrity_metadata ? "true" : "false") << "\n"
      << "aead_provider_ready=" << (profile.aead_provider_ready ? "true" : "false") << "\n"
      << "section_sealer=" << selected_section_sealer_name(profile, crypto_provider) << "\n"
      << "sealed_sections=" << ((profile.crypto_provider_ready) ? "true" : "false") << "\n"
      << "section_name_redaction=" << ((profile.strip_debug_metadata) ? "true" : "false") << "\n"
      << "signature_provider_ready=false\n"
      << "production_metadata_profile=" << ((profile.strip_debug_metadata) ? "minimal-v1" : "diagnostic-v1") << "\n"
      << "feature_count=" << feature_count << "\n"
      << "required_in_release_count=" << release_required_count << "\n";

  std::uint32_t id = 1;
  for (const auto& section : sections) {
    if (section.type == venom::package::SectionType::PackageFeatures ||
        (section.type == venom::package::SectionType::Integrity && section.name == "integrity-auth.vsig")) {
      continue;
    }
    out << "feature\t"
        << id++ << "\t"
        << venom::package::section_type_name(section.type) << "\t"
        << "1\t"
        << (package_feature_required_in_release(section) ? "true" : "false") << "\t"
        << static_cast<std::uint32_t>(section.type) << "\t"
        << stored_section_name_for_metadata(profile, section.type, section.name) << "\t"
        << venom::package::sha256_hex(section.data) << "\n";
  }
  return out.str();
}

} // namespace venom::compiler::build_package_detail
