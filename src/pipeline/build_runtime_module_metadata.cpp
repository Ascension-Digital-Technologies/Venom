#include "pipeline/build_runtime_metadata.hpp"
#include "pipeline/build_support.hpp"
#include "pipeline/assets.hpp"
#include "package/hash.hpp"

#include "turbojs/abi.hpp"
#include "turbojs/bridge.hpp"
#include "turbojs/engine.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace venom::compiler::build_runtime_detail {

using namespace build_detail;
using namespace build_package_detail;

std::string make_turbojs_bytecode_manifest_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool redact_metadata = profile.kind == ProfileKind::Prod || profile.strip_debug_metadata;
  auto records = make_turbojs_bytecode_records(bridge, redact_metadata);
  std::ostringstream out;
  out << venom::turbojs::bytecode_manifest_text(records, venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "section=turbojs-bytecode-manifest.vqbm\n"
      << "package_envelope=build-specific-turbojs-envelope-v2\n"
      << "package_envelope_magic=VTJSE006\n"
      << "package_envelope_inner=VTJSBC03|VTJSMB04\n"
      << "package_envelope_binding=build-salt|route|source|order|lane-map\n"
      << "package_envelope_integrity=fnv1a64-inner-record\n"
      << "package_envelope_permutation=per-build-16-byte-lane-map\n";
  return out.str();
}

std::string make_turbojs_module_resolver_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << venom::turbojs::module_resolver_metadata_text(venom::turbojs::kRuntimeAbiVersion,
                                                       venom::turbojs::kRuntimePackageVersion,
                                                       bridge.chunks.size())
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_exception_abi_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << venom::turbojs::exception_abi_metadata_text(venom::turbojs::kRuntimeAbiVersion,
                                                     venom::turbojs::kRuntimePackageVersion)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_host_trap_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::host_trap_policy_metadata_text(strict,
                                                        venom::turbojs::kRuntimeAbiVersion,
                                                        venom::turbojs::kRuntimePackageVersion)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_execution_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::execution_lifecycle_text(strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_script_buffer_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::script_buffer_policy_text(venom::turbojs::kDefaultScriptBufferLimitBytes, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_context_limit_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::context_limit_policy_text(venom::turbojs::kDefaultHeapLimitBytes,
                                                   venom::turbojs::kDefaultStackLimitBytes,
                                                   venom::turbojs::kDefaultContextLimit,
                                                   venom::turbojs::kDefaultScriptBufferLimitBytes,
                                                   venom::turbojs::kDefaultHostCallLimit,
                                                   venom::turbojs::kDefaultConsoleEventLimit,
                                                   venom::turbojs::kDefaultModuleRecordLimit,
                                                   strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_host_call_dispatch_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::host_call_dispatch_table_text(strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "dispatch_hash=0x" << std::hex << venom::turbojs::host_call_dispatch_table_hash(strict) << std::dec << "\n";
  return out.str();
}

std::string make_turbojs_parity_contract_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  const auto dispatch_hash = venom::turbojs::host_call_dispatch_table_hash(strict);
  std::ostringstream out;
  out << venom::turbojs::parity_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                              venom::turbojs::kRuntimePackageVersion,
                                              venom::turbojs::abi_table_hash(),
                                              venom::turbojs::host_import_table_hash(),
                                              dispatch_hash,
                                              strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_release_failclosed_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::release_failclosed_policy_text(strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}


std::size_t turbojs_module_count(const JsBridge& bridge) {
  return static_cast<std::size_t>(std::count_if(bridge.chunks.begin(), bridge.chunks.end(), [](const JsChunk& chunk) {
    return (chunk.flags & JsChunkModule) != 0u;
  }));
}

std::string make_turbojs_module_graph_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  const auto module_count = turbojs_module_count(bridge);
  const bool redact_metadata = profile.kind == ProfileKind::Prod || profile.strip_debug_metadata;
  out << "VENOM_TJS_MODULE_GRAPH_V1\n"
      << "version=1\n"
      << "runtime_abi=" << venom::turbojs::kRuntimeAbiVersion << "\n"
      << "package_version=" << venom::turbojs::kRuntimePackageVersion << "\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "graph_format=route-scoped-script-module-records\n"
      << "chunk_count=" << bridge.chunks.size() << "\n"
      << "module_count=" << module_count << "\n"
      << "resolver=package-relative-first\n"
      << "dynamic_import=protected-module-bundle-vtjsmb04\n"
      << "edge_count=" << bridge.module_edges.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    const std::string route_label = redact_metadata
        ? ("r_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("route|" + chunk.route)), 16))
        : chunk.route;
    const std::string source_label = redact_metadata
        ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16))
        : chunk.source;
    out << "module\t" << chunk.order << "\t" << route_label << "\t" << source_label
        << "\tbytes=" << (redact_metadata ? 0u : chunk.code.size())
        << "\thash=0x" << std::hex << (redact_metadata ? 0u : venom::package::fnv1a64(source_bytes)) << std::dec
        << "\tflags=" << js_flags_summary(chunk.flags)
        << "\tis_module=" << (((chunk.flags & JsChunkModule) != 0u) ? "true" : "false") << "\n";
  }
  for (const auto& edge : bridge.module_edges) {
    const std::string referrer = redact_metadata ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("referrer|" + edge.referrer)), 16)) : edge.referrer;
    const std::string resolved = redact_metadata ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("resolved|" + edge.resolved)), 16)) : edge.resolved;
    out << "edge\t" << (edge.dynamic ? "dynamic" : "static") << "\t" << referrer << "\t" << resolved << "\n";
  }
  return out.str();
}

std::string make_turbojs_module_execution_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::module_execution_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                        venom::turbojs::kRuntimePackageVersion,
                                                        turbojs_module_count(bridge),
                                                        strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_module_cache_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << venom::turbojs::module_cache_policy_text(venom::turbojs::kRuntimeAbiVersion,
                                                  venom::turbojs::kRuntimePackageVersion,
                                                  turbojs_module_count(bridge))
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_resolver_audit_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::resolver_audit_policy_text(venom::turbojs::kRuntimeAbiVersion,
                                                    venom::turbojs::kRuntimePackageVersion,
                                                    turbojs_module_count(bridge),
                                                    strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_interop_fallback_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::interop_fallback_policy_text(venom::turbojs::kRuntimeAbiVersion,
                                                      venom::turbojs::kRuntimePackageVersion,
                                                      strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_execution_pipeline_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::execution_pipeline_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                          venom::turbojs::kRuntimePackageVersion,
                                                          bridge.chunks.size(),
                                                          strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_script_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::script_records_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                      venom::turbojs::kRuntimePackageVersion,
                                                      bridge.chunks.size(),
                                                      strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    out << "script_record\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk)
        << "\thash=0x" << std::hex << protected_source_hash(profile, chunk) << std::dec
        << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_turbojs_eval_results_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::eval_results_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                    venom::turbojs::kRuntimePackageVersion,
                                                    strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_console_capture_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::console_capture_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                       venom::turbojs::kRuntimePackageVersion,
                                                       strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_failure_reports_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::failure_reports_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                       venom::turbojs::kRuntimePackageVersion,
                                                       strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_runtime_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  if (profile.kind == ProfileKind::Prod) {
    // Compact authenticated release policy. The fixed-width format avoids text
    // decoding, splitting, and string-key lookups in the protected loader.
    // Layout: magic[8]="VRPOL002", version:u32, flags:u32, profile_hash:u32,
    // runtime_hash:u32. All integer fields are little-endian.
    std::string out("VRPOL002", 8);
    auto append_u32 = [&out](std::uint32_t value) {
      out.push_back(static_cast<char>(value & 0xffu));
      out.push_back(static_cast<char>((value >> 8u) & 0xffu));
      out.push_back(static_cast<char>((value >> 16u) & 0xffu));
      out.push_back(static_cast<char>((value >> 24u) & 0xffu));
    };
    auto fnv32 = [](const std::string& text) {
      std::uint32_t h = 2166136261u;
      for (const char raw_c : text) {
    const auto c = static_cast<unsigned char>(raw_c); h ^= c; h *= 16777619u; }
      return h;
    };
    std::uint32_t flags = 0u;
    if (profile.fail_closed) flags |= 1u << 0u;
    if (profile.runtime_hardened) flags |= 1u << 1u;
    if (profile.strip_debug_metadata) flags |= 1u << 2u;
    if (profile.runtime_hardened) flags |= 1u << 3u; // freeze host bridge
    if (profile.integrity_metadata) flags |= 1u << 4u;
    if (runtime_mode == "wasm") flags |= 1u << 5u;
    flags |= 1u << 6u; // safe failure UI
    flags |= 1u << 7u; // fetch bridge
    flags |= 1u << 8u; // timer bridge
    flags |= 1u << 9u; // event queue
    flags |= 1u << 10u; // async host queue
    append_u32(2u);
    append_u32(flags);
    append_u32(fnv32(profile.name));
    append_u32(fnv32(runtime_mode));
    return out;
  }
  std::ostringstream out;
  out << "VENOM_RUNTIME_POLICY_V1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "wasm_runtime=" << (runtime_mode == "wasm" ? "true" : "false") << "\n"
      << "fail_closed=" << (profile.fail_closed ? "true" : "false") << "\n"
      << "runtime_hardened=" << (profile.runtime_hardened ? "true" : "false") << "\n"
      << "strip_debug_metadata=" << (profile.strip_debug_metadata ? "true" : "false") << "\n"
      << "freeze_host_bridge=" << (profile.runtime_hardened ? "true" : "false") << "\n"
      << "require_integrity=" << (profile.integrity_metadata ? "true" : "false") << "\n"
      << "package_hash_allowlist=loader\n"
      << "safe_failure_ui=true\n"
      << "binary_dom_ops=" << (runtime_mode == "wasm" ? "true" : "false") << "\n"
      << "fetch_bridge=true\n"
      << "timer_bridge=true\n"
      << "event_queue=true\n"
      << "async_host_queue=true\n"
      << "turbojs_bridge=engine-module\n"
      << "script_isolation=route-scoped\n"
      << "script_policy=capability-metadata\n"
      << "turbojs_chunks=engine-input\n"
      << "turbojs_engine=module-loader\n"
      << "turbojs_context_lifecycle=tracked\n"
      << "host_capabilities=injected\n"
      << "turbojs_adapter_diagnostics=enabled\n"
      << "script_engine_fallback=module-fallback-policy\n"
      << "turbojs_wasm_runtime=scaffold-asset\n"
      << "turbojs_source_transfer=wasm-memory\n"
      << "turbojs_console_bridge=host-console-forward\n"
      << "turbojs_execution_records=enabled\n"
      << "turbojs_result_bridge=enabled\n"
      << "turbojs_fallback_policy=enforced\n"
      << "turbojs_engine_backend=turbojs-wasm-backend-candidate\n"
      << "turbojs_native_parity=metadata\n"
      << "turbojs_execution_mode=prefer-backend-candidate\n"
      << "turbojs_runtime_abi=table-v4\n"
      << "turbojs_heap_limits=context-accounting\n"
      << "turbojs_script_buffer=alloc-api\n"
      << "turbojs_console_callback_abi=enabled\n"
      << "turbojs_host_import_table=designed\n"
      << "turbojs_parity_probe=native-wasm\n"
      << "turbojs_release_fallback=" << ((profile.fail_closed || profile.kind == ProfileKind::Prod) ? "strict-deny" : "policy-gated") << "\n"
      << "turbojs_bytecode_manifest=native-turbojs-object-bytecode-v3\n"
      << "turbojs_module_resolver=package-relative-map\n"
      << "turbojs_exception_abi=json-record-v1\n"
      << "turbojs_host_trap_policy=" << ((profile.fail_closed || profile.kind == ProfileKind::Prod) ? "deny-unknown-host-imports" : "record-unknown-host-imports") << "\n"
      << "turbojs_execution_lifecycle=state-machine-v1\n"
      << "turbojs_script_buffer_policy=validate-hash-before-execute\n"
      << "turbojs_context_limit_policy=enforced\n"
      << "turbojs_host_call_dispatch=stable-id-table-v1\n"
      << "turbojs_parity_contract=abi-hash-limit-trap-host-table\n"
      << "turbojs_release_failclosed=enabled\n"
      << "turbojs_execution_pipeline=records-v1\n"
      << "turbojs_script_records=prepared-records-v1\n"
      << "turbojs_eval_results=eval-json-records-v1\n"
      << "turbojs_console_capture=capture-drain-v1\n"
      << "turbojs_failure_reports=failclosed-json-records-v1\n";
  return out.str();
}

} // namespace venom::compiler::build_runtime_detail
