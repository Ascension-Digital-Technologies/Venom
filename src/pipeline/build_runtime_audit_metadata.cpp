#include "base/error.hpp"
#include "pipeline/build_runtime_metadata.hpp"
#include "pipeline/build_support.hpp"
#include "pipeline/assets.hpp"
#include "package/hash.hpp"

#include "turbojs/abi.hpp"
#include "turbojs/bridge.hpp"
#include "turbojs/engine.hpp"

#include <cctype>
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

std::string make_turbojs_execution_journal_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::execution_journal_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                         venom::turbojs::kRuntimePackageVersion,
                                                         bridge.chunks.size(),
                                                         strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    out << "journal_seed\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk)
        << "\tsource_hash=0x" << std::hex << protected_source_hash(profile, chunk) << std::dec
        << "\tcheckpoint=prepare\n";
  }
  return out.str();
}

std::string make_turbojs_checkpoint_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::checkpoint_policy_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                         venom::turbojs::kRuntimePackageVersion,
                                                         strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_replay_cursor_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::replay_cursor_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                     venom::turbojs::kRuntimePackageVersion,
                                                     strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_resume_state_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::resume_state_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                    venom::turbojs::kRuntimePackageVersion,
                                                    strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_determinism_audit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::determinism_audit_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                        venom::turbojs::kRuntimePackageVersion,
                                                        strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_snapshot_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::snapshot_policy_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                       venom::turbojs::kRuntimePackageVersion,
                                                       strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_snapshot_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::snapshot_records_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                        venom::turbojs::kRuntimePackageVersion,
                                                        bridge.chunks.size(),
                                                        strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  for (const auto& chunk : bridge.chunks) {
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    out << "snapshot_seed\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk)
        << "\tsource_hash=0x" << std::hex << protected_source_hash(profile, chunk) << std::dec
        << "\tstage=post-evaluate\n";
  }
  return out.str();
}

std::string make_turbojs_replay_validation_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::replay_validation_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                         venom::turbojs::kRuntimePackageVersion,
                                                         strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_determinism_ledger_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::determinism_ledger_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                          venom::turbojs::kRuntimePackageVersion,
                                                          strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_audit_seal_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::audit_seal_contract_text(venom::turbojs::kRuntimeAbiVersion,
                                                  venom::turbojs::kRuntimePackageVersion,
                                                  strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_execution_commit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::execution_commit_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_rollback_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::rollback_policy_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_host_call_receipts_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::host_call_receipts_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_release_acceptance_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::release_acceptance_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_commit_audit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::commit_audit_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_capability_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::capability_policy_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_host_io_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::host_io_policy_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_permission_seal_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::permission_seal_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_policy_receipts_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::policy_receipts_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_release_gate_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::release_gate_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_host_io_decision_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::host_io_decision_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_host_io_deny_trace_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::host_io_deny_trace_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_capability_ledger_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::capability_ledger_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_policy_seal_audit_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::policy_seal_audit_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_runtime_denylist_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << venom::turbojs::runtime_denylist_contract_text(venom::turbojs::kRuntimeAbiVersion, venom::turbojs::kRuntimePackageVersion, strict)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n";
  return out.str();
}

std::string make_turbojs_context_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_CONTEXT_LIFECYCLE_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "model=route-scoped-reusable\n"
      << "key=route|source|order\n"
      << "create_host_call=turbojs.contextCreate\n"
      << "reuse_host_call=turbojs.contextReuse\n"
      << "destroy_host_call=turbojs.contextDestroy\n"
      << "module_create_host_call=turbojs.moduleContextCreate\n"
      << "module_destroy_host_call=turbojs.moduleContextDestroy\n"
      << "snapshot=true\n"
      << "max_contexts=" << venom::turbojs::kDefaultContextLimit << "\n"
      << "heap_limit=turbojs-heap-limits.vqhl\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "context\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\treuse=true\tdestroy=route-unload\n";
  }
  return out.str();
}

std::string make_host_capabilities_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  auto contains_token = [](const std::string& code, const std::string& token) {
    std::size_t pos = 0;
    while ((pos = code.find(token, pos)) != std::string::npos) {
      const bool left_ok = pos == 0 || !(std::isalnum(static_cast<unsigned char>(code[pos - 1])) || code[pos - 1] == '_' || code[pos - 1] == '$');
      const std::size_t right = pos + token.size();
      const bool right_ok = right >= code.size() || !(std::isalnum(static_cast<unsigned char>(code[right])) || code[right] == '_' || code[right] == '$');
      if (left_ok && right_ok) return true;
      pos = right;
    }
    return false;
  };
  auto capabilities_for = [&](const JsChunk& chunk) {
    std::vector<std::string> caps;
    const auto& code = chunk.code;
    if (contains_token(code, "console")) caps.emplace_back("console");
    if (contains_token(code, "document")) caps.emplace_back("document");
    if (contains_token(code, "window")) caps.emplace_back("window");
    if (contains_token(code, "navigator")) caps.emplace_back("navigator");
    if (contains_token(code, "fetch") || contains_token(code, "XMLHttpRequest")) caps.emplace_back("fetch");
    if (contains_token(code, "setTimeout") || contains_token(code, "clearTimeout") ||
        contains_token(code, "setInterval") || contains_token(code, "clearInterval")) caps.emplace_back("timers");
    if (contains_token(code, "addEventListener") || contains_token(code, "removeEventListener") ||
        contains_token(code, "dispatchEvent")) caps.emplace_back("events");
    if (contains_token(code, "localStorage") || contains_token(code, "sessionStorage") || contains_token(code, "indexedDB")) caps.emplace_back("storage");
    if (contains_token(code, "crypto") || contains_token(code, "SubtleCrypto")) caps.emplace_back("crypto");
    caps.emplace_back("__venomRuntime");
    return caps;
  };
  auto join = [](const std::vector<std::string>& values) {
    std::ostringstream text;
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (i) text << ',';
      text << values[i];
    }
    return text.str();
  };

  std::ostringstream out;
  out << "VENOM_HOST_CAPABILITIES_V3\n"
      << "version=3\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "policy=least-privilege-per-chunk\n"
      << "undeclared_capability=deny\n"
      << "inject_host_call=turbojs.hostCapabilityInject\n"
      << "capability\tconsole\treadonly\n"
      << "capability\tdocument\tbridge\n"
      << "capability\twindow\tbridge\n"
      << "capability\tnavigator\tguarded-browser-api-shim\n"
      << "capability\tfetch\tasync-host-call\n"
      << "capability\ttimers\tasync-host-call\n"
      << "capability\tevents\tqueue\n"
      << "capability\tstorage\tpolicy-controlled-browser-storage\n"
      << "capability\tcrypto\tpolicy-controlled-webcrypto\n"
      << "capability\t__venomRuntime\tfrozen-bridge\n"
      << "default_capability_count=1\n"
      << "default_capabilities=__venomRuntime\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    const auto caps = capabilities_for(chunk);
    out << "chunk_capabilities\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\t" << join(caps) << "\n";
  }
  return out.str();
}

std::string make_turbojs_adapter_diagnostics_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_ADAPTER_DIAGNOSTICS_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "engine_asset=" << engine_asset_name << "\n"
      << "adapter_contract=createContext|executeChunk|destroyContext|contextSnapshot|status|parityProbe|abiTable\n"
      << "fallback=deny-host-js-source-eval\n"
      << "abi_contract=turbojs-wasm-abi12-runtime\n"
      << "status_exports=venom_tjs_status_code,venom_tjs_status_record_ptr,venom_tjs_status_record_size,venom_tjs_release_status\n"
      << "limit_exports=venom_tjs_context_heap_limit,venom_tjs_context_heap_used,venom_tjs_context_stack_limit\n"
      << "bytecode_validation_exports=venom_tjs_bytecode_validate,venom_tjs_bytecode_record_hash32,venom_tjs_bytecode_payload_size,venom_tjs_bytecode_expected_source_hash32\n"
      << "records=engineBoot,abiTable,contextCreate,contextSetLimits,scriptBufferAlloc,consoleCallbackAbi,hostImportTable,parityProbe,contextReuse,hostCapabilityInject,executeChunk,executionResult,contextDestroy,adapterStatus\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "adapter_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}


std::string make_turbojs_execution_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_EXECUTION_RECORDS_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "record_host_call=turbojs.executionRecord\n"
      << "snapshot_host_call=turbojs.executionSnapshot\n"
      << "record_fields=context,route,source,order,engine,wasmAccepted,fallbackUsed,consoleEvents,resultHash,abi,heapUsed,heapLimit,scriptBufferBytes,parityProbe\n"
      << "retention=runtime-session\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "execution_record\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tengine=turbojs-wasm-backend-candidate\tfallback=policy-gated\n";
  }
  return out.str();
}

std::string make_turbojs_result_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_RESULT_BRIDGE_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "result_host_call=turbojs.resultBridge\n"
      << "wasm_decode_host_call=turbojs.wasmResultDecode\n"
      << "console_event_host_call=turbojs.consoleEvent\n"
      << "console_flush_host_call=turbojs.consoleFlush\n"
      << "format=json-record-v1\n"
      << "fields=ok,engine,version,abi,context,sourceBytes,sourceHash,consoleCount,fallbackRequired,heapUsed,heapLimit,scriptBufferBytes,parityProbe\n";
  return out.str();
}

std::string make_turbojs_fallback_policy_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << "VENOM_TURBOJS_FALLBACK_POLICY_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=explicit-policy-gated\n"
      << "decision_host_call=script.fallbackDecision\n"
      << "policy_check_host_call=turbojs.fallbackPolicyCheck\n"
      << "allow_when=" << (strict ? "empty-chunk|real-engine-ready" : "engine-unavailable|wasm-interpreter-unavailable|empty-chunk") << "\n"
      << "deny_when=" << (strict ? "release-strict-no-fallback|engine-module-unavailable-or-compatible-fallback|wasm-interpreter-unavailable" : "release-strict-no-fallback") << "\n"
      << "current_release_policy=" << (strict ? "deny-host-fallback-unless-real-engine-ready" : "allow-compatible-fallback-with-record") << "\n"
      << "strict_release=" << (strict ? "true" : "false") << "\n"
      << "require_parity_probe=true\n"
      << "audit_records=turbojs-execution-records.vqer\n";
  return out.str();
}


std::string make_turbojs_engine_backend_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_ENGINE_BACKEND_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "selected_backend=turbojs-wasm-real\n"
      << "native_backend=vendored-turbojs-c\n"
      << "browser_backend=turbojs-runtime-wasm-upstream-global-host-api-shims\n"
      << "scaffold_status=replaced-by-upstream-global-host-api-shims\n"
      << "replacement_path=active-upstream-global-host-api-shims-bridge\n"
      << "host_js_fallback_allowed=false\n"
      << "wasm_execution_contract=turbojs-wasm-execution.vqwe\n"
      << "backend_select_host_call=turbojs.backendSelect\n"
      << "real_engine_candidate_host_call=turbojs.realEngineCandidate\n"
      << "fallback_decision_host_call=turbojs.interpreterFallbackDecision\n"
      << "bytecode_chunks=route-scoped-script-chunk.*.vjsb\n"
      << "host_capabilities=host-capabilities.vhcap\n"
      << "native_parity=turbojs-native-parity.vqnp\n"
      << "execution_mode=turbojs-execution-mode.vqxm\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "backend_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbackend=turbojs-wasm-real\tfallback=deny-host-js\n";
  }
  return out.str();
}

std::string make_turbojs_native_parity_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& probe) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_NATIVE_PARITY_V3\n"
      << "version=3\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "native_probe=" << probe << "\n"
      << "native_engine=vendored-turbojs-c\n"
      << "browser_engine=turbojs-runtime-wasm-upstream-global-host-api-shims\n"
      << "upstream_turbojs_source=third_party/turbojs\n"
      << "upstream_bridge=enabled\n"
      << "upstream_interpreter_core=turbojs-upstream-global-host-api-shims-v7\n"
      << "upstream_parity_record=venom_tjs_upstream_parity_record_ptr|venom_tjs_upstream_parity_record_size|venom_tjs_upstream_parity_record_hash\n"
      << "upstream_runtime_records=venom_tjs_upstream_object_record_ptr|venom_tjs_upstream_exception_record_ptr|venom_tjs_upstream_module_record_ptr|venom_tjs_upstream_runtime_bridge_score\n"
      << "upstream_bytecode_semantics=enabled\n"
      << "upstream_bytecode_semantics_records=venom_tjs_upstream_bytecode_semantics_record_ptr|venom_tjs_upstream_lexical_scope_record_ptr|venom_tjs_upstream_closure_record_ptr|venom_tjs_upstream_iterator_record_ptr|venom_tjs_upstream_bytecode_semantics_score\n"
      << "upstream_intrinsic_semantics=enabled\n"
      << "upstream_intrinsic_semantics_records=venom_tjs_upstream_intrinsic_record_ptr|venom_tjs_upstream_property_descriptor_record_ptr|venom_tjs_upstream_prototype_chain_record_ptr|venom_tjs_upstream_call_frame_record_ptr|venom_tjs_upstream_intrinsic_semantics_score\n"
      << "upstream_feature_exports=venom_tjs_upstream_feature_count|venom_tjs_upstream_builtin_count|venom_tjs_upstream_object_model_score|venom_tjs_upstream_exception_model_score|venom_tjs_upstream_module_model_score\n"
      << "parity_scope=context-create|bytecode-transfer|result-record|console-count|fallback-denied|global-slot-record|builtin-probe|object-model-score|module-model-score|intrinsic-semantics|property-descriptor|prototype-chain|call-frame\n"
      << "full_ecmascript_parity=false\n"
      << "full_bytecode_execution=upstream-global-host-api-shims-boundary\n"
      << "cutover_status=upstream-intrinsic-semantics-adapter-active\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "parity_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags)
        << "\tupstream_bridge=turbojs-upstream-global-host-api-shims-v7\n";
  }
  return out.str();
}

std::string make_turbojs_execution_mode_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TURBOJS_EXECUTION_MODE_V3\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "mode=turbojs-wasm-real\n"
      << "fallback=fail-closed-no-host-js\n"
      << "deny_silent_fallback=true\n"
      << "record_backend_selection=true\n"
      << "require_source_transfer=true\n"
      << "require_result_bridge=true\n"
      << "require_console_bridge=true\n"
      << "release_policy=deny-host-fallback-require-wasm-backend\n";
  return out.str();
}


std::string make_turbojs_runtime_abi_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << venom::turbojs::runtime_abi_table_text()
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "abi_hash=0x" << std::hex << venom::turbojs::abi_table_hash() << std::dec << "\n"
      << "metadata_section=turbojs-runtime-abi.vqra\n";
  return out.str();
}

std::string make_turbojs_host_imports_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << venom::turbojs::host_import_table_text()
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "table_hash=0x" << std::hex << venom::turbojs::host_import_table_hash() << std::dec << "\n"
      << "host_call_import_table=turbojs-host-imports.vqhi\n";
  return out.str();
}

std::string make_turbojs_heap_limits_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TJS_CONTEXT_LIMITS_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "default_heap_limit=" << venom::turbojs::kDefaultHeapLimitBytes << "\n"
      << "default_stack_limit=" << venom::turbojs::kDefaultStackLimitBytes << "\n"
      << "max_contexts=" << venom::turbojs::kDefaultContextLimit << "\n"
      << "set_limits_export=venom_tjs_context_set_limits\n"
      << "heap_limit_export=venom_tjs_context_heap_limit\n"
      << "heap_used_export=venom_tjs_context_heap_used\n"
      << "stack_limit_export=venom_tjs_context_stack_limit\n"
      << "release_fail_closed=" << (profile.fail_closed ? "true" : "false") << "\n";
  return out.str();
}

std::string make_turbojs_script_buffer_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TJS_SCRIPT_BUFFER_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "encoding=utf-8\n"
      << "max_script_bytes=" << venom::turbojs::kDefaultScriptBufferLimitBytes << "\n"
      << "alloc_export=venom_tjs_script_buffer_alloc\n"
      << "ptr_export=venom_tjs_script_buffer_ptr\n"
      << "size_export=venom_tjs_script_buffer_size\n"
      << "capacity_export=venom_tjs_script_buffer_capacity\n"
      << "free_export=venom_tjs_script_buffer_free\n"
      << "legacy_source_ptr_export=venom_tjs_source_ptr\n";
  return out.str();
}

std::string make_turbojs_console_abi_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TJS_CONSOLE_CALLBACK_ABI_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "callback_abi_export=venom_tjs_console_callback_abi\n"
      << "event_ptr_export=venom_tjs_console_event_ptr\n"
      << "event_size_export=venom_tjs_console_event_size\n"
      << "event_count_export=venom_tjs_console_event_count\n"
      << "event_clear_export=venom_tjs_console_clear\n"
      << "event_schema=level|route|source|order|message\n"
      << "host_call=turbojs.consoleCallbackAbi\n";
  return out.str();
}

std::string make_turbojs_parity_probe_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& native_eval) {
  venom::turbojs::ParityProbe probe;
  probe.abi_hash = venom::turbojs::abi_table_hash();
  probe.host_import_hash = venom::turbojs::host_import_table_hash();
  probe.native_eval = native_eval;
  std::ostringstream out;
  out << venom::turbojs::parity_probe_text(probe)
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "native_engine=vendored-turbojs-c\n"
      << "wasm_engine=turbojs-runtime.wasm\n"
      << "host_call=turbojs.wasmParityProbe\n";
  return out.str();
}

std::string make_turbojs_release_fallback_metadata(const Profile& profile, const std::string& runtime_mode) {
  const bool strict = profile.fail_closed || profile.kind == ProfileKind::Prod;
  std::ostringstream out;
  out << "VENOM_TJS_RELEASE_FALLBACK_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "strict_release=" << (strict ? "true" : "false") << "\n"
      << "deny_silent_fallback=true\n"
      << "allow_host_fallback=false\n"
      << "require_backend_record=true\n"
      << "require_parity_probe=true\n"
      << "host_call=turbojs.releaseFallbackDeny\n"
      << "policy=deny-host-fallback-require-wasm-backend\n";
  return out.str();
}

std::vector<venom::turbojs::BytecodeChunkRecord> make_turbojs_bytecode_records(const JsBridge& bridge, bool redact_metadata) {
  std::vector<venom::turbojs::BytecodeChunkRecord> records;
  std::vector<venom::turbojs::ModuleSourceRecord> module_sources;
  records.reserve(bridge.chunks.size());
  module_sources.reserve(bridge.chunks.size());
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u || (chunk.flags & JsChunkModule) == 0u) continue;
    const std::string compile_name = redact_metadata
        ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16))
        : chunk.source;
    module_sources.push_back({chunk.source, compile_name, chunk.code});
  }
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u) continue;
    const auto limit = static_cast<std::size_t>(venom::turbojs::kDefaultScriptBufferLimitBytes);
    if (chunk.code.size() > limit) {
      raise_error("VENOM-E5000", "TurboJS script exceeds protected transfer limit: " + chunk.source +
                               " (" + std::to_string(chunk.code.size()) + " bytes > " +
                               std::to_string(limit) + " bytes)");
    }
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    const std::string record_source = redact_metadata ? ("s_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("source|" + chunk.route + "|" + chunk.source + "|" + std::to_string(chunk.order))), 16)) : chunk.source;
    const bool module_requires_registry = is_module &&
        (chunk.code.find("import ") != std::string::npos ||
         chunk.code.find("export {") != std::string::npos ||
         chunk.code.find("export *") != std::string::npos);
    const auto bytecode = module_requires_registry && !bridge.bridge_registry_bytecode.empty()
        ? bridge.bridge_registry_bytecode
        : venom::turbojs::compile_native_turbojs_bytecode(
            chunk.code,
            record_source,
            is_module,
            redact_metadata,
            is_module ? &module_sources : nullptr);
    if (bytecode.size() > limit) {
      raise_error("VENOM-E5000", "TurboJS bytecode exceeds protected transfer limit: " + chunk.source +
                               " (" + std::to_string(bytecode.size()) + " bytes > " +
                               std::to_string(limit) + " bytes)");
    }
    std::vector<unsigned char> source_bytes(chunk.code.begin(), chunk.code.end());
    const std::string record_route = redact_metadata ? ("r_" + short_hash_hex(venom::package::fnv1a64(bytes_from_string("route|" + chunk.route)), 16)) : chunk.route;
    records.push_back({chunk.order,
                       record_route,
                       record_source,
                       redact_metadata ? 0u : static_cast<std::uint32_t>(chunk.code.size()),
                       redact_metadata ? 0u : venom::package::fnv1a64(source_bytes),
                       static_cast<std::uint32_t>(bytecode.size()),
                       venom::turbojs::BytecodeFormat::NativeTurboJsObjectV3});
  }
  return records;
}

} // namespace venom::compiler::build_runtime_detail
