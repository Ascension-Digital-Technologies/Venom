#include "venom/base/error.hpp"
#include "planning/section_plan.hpp"

#include "build_support.hpp"
#include "build_runtime_metadata.hpp"
#include "build_report.hpp"
#include "venom/package/format.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace venom::compiler::package_sections {
namespace {
using namespace build_detail;
using namespace build_package_detail;
using namespace build_runtime_detail;
using namespace build_report_detail;

bool should_encrypt_section(const Profile& profile, venom::package::SectionType type) {
  if (!profile.crypto_provider_ready) return false;
  return type != venom::package::SectionType::Asset && type != venom::package::SectionType::Css;
}

bool should_ship_manifest_section(const Profile& profile, const BuildOptions& options) {
  return !profile.strip_debug_metadata || options.emit_debug_manifest;
}
} // namespace

std::vector<JsChunk> select_lazy_route_scripts(const std::vector<JsChunk>& chunks,
                                                const std::string& route) {
  std::vector<JsChunk> selected;
  std::unordered_set<std::string> keys;
  selected.reserve(chunks.size());
  keys.reserve(chunks.size());
  auto append_unique = [&](const JsChunk& chunk) mutable {
    const auto key = chunk.route + "\n" + chunk.source + "\n" + std::to_string(chunk.order);
    if (keys.insert(key).second) selected.push_back(chunk);
  };
  for (const auto& chunk : chunks) if (chunk.route == route) append_unique(chunk);
  for (const auto& chunk : chunks) {
    const bool browser_module_dependency =
        (chunk.flags & JsChunkBrowser) != 0u &&
        (chunk.flags & JsChunkModule) != 0u &&
        (chunk.flags & JsChunkDependency) != 0u;
    if (browser_module_dependency) append_unique(chunk);
  }
  std::sort(selected.begin(), selected.end(), [](const auto& left, const auto& right) {
    return left.order < right.order;
  });
  return selected;
}

Result make_plan(const Inputs& input) {
  const auto& options = input.options;
  const auto& profile = input.profile;
  const auto& graph = input.graph;
  const auto& compiled_routes = input.compiled_routes;
  const auto& js_bridge = input.js_bridge;
  const auto& remote_vendor_options = input.remote_vendor_options;
  const auto& vendor_lock_path = input.vendor_lock_path;
  const auto& vendor_lock_digest = input.vendor_lock_digest;
  const auto& vendor_lock_mode = input.vendor_lock_mode;
  const auto& release_policy = input.release_policy;
  const auto& assets = input.assets;
  const auto& poly = input.polymorphic_plan;
  const auto& runtime_name = input.runtime_name;
  const auto& runtime = input.runtime_source;
  const auto& wasm_name = input.wasm_name;
  const auto& wasm_bytes = input.wasm_bytes;
  const auto& style_name = input.style_name;
  const auto& css = input.css;
  const auto& quickjs_engine_name = input.quickjs_engine_name;
  const auto& quickjs_engine_module = input.quickjs_engine_source;
  const auto& quickjs_wasm_name = input.quickjs_wasm_name;
  const auto& quickjs_wasm_bytes = input.quickjs_wasm_bytes;
  const auto& worker_name = input.worker_name;
  const auto& worker_runtime = input.worker_source;
  const auto& qjs_probe = input.quickjs_probe;
  const auto& js_preview = input.js_preview;
  const bool wasm_runtime = input.wasm_runtime;
  const bool hardened_release_asset = input.hardened_release_asset;
  const bool production_hardening = input.production_hardening;
  const bool external_lazy_registry = input.external_lazy_registry;
  const auto package_binding_token = make_package_binding_token(options.diversification_seed != 0u || profile.deterministic, profile.name + "|" + options.runtime + "|" + runtime_name + "|" + wasm_name + "|" + quickjs_engine_name + "|" + quickjs_wasm_name + "|" + worker_name);
  const auto package_flags = profile.package_flags |
      ((options.strict_release || profile.kind == ProfileKind::Prod) ? venom::package::PackageFlagReleaseProfile : 0u) |
      venom::package::PackageFlagTimerBridge |
      venom::package::PackageFlagEventQueue |
      venom::package::PackageFlagQuickJsBridge |
      venom::package::PackageFlagScriptIsolation |
      venom::package::PackageFlagScriptPolicy |
      venom::package::PackageFlagQuickJsChunks |
      venom::package::PackageFlagQuickJsEngine |
      venom::package::PackageFlagScriptEngineFallback |
      venom::package::PackageFlagQuickJsEngineModule |
      venom::package::PackageFlagQuickJsContextLifecycle |
      venom::package::PackageFlagHostCapabilities |
      venom::package::PackageFlagQuickJsAdapterDiagnostics |
      venom::package::PackageFlagQuickJsWasmRuntime |
      venom::package::PackageFlagQuickJsSourceTransfer |
      venom::package::PackageFlagQuickJsConsoleBridge |
      venom::package::PackageFlagQuickJsExecutionRecords |
      venom::package::PackageFlagQuickJsResultBridge |
      venom::package::PackageFlagQuickJsFallbackPolicy |
      venom::package::PackageFlagQuickJsEngineBackend |
      (wasm_runtime ? (venom::package::PackageFlagWasmRuntime | venom::package::PackageFlagBinaryDomOps) : 0u);

  package_plan::Plan package_plan;
  const auto add_package_section = [&](venom::package::SectionType type,
                                       std::string name,
                                       std::vector<unsigned char> data,
                                       std::uint32_t flags = venom::package::SectionFlagNone) {
    if (should_encrypt_section(profile, type)) {
      flags |= venom::package::SectionFlagEncrypted;
    }
    package_plan.add(type, std::move(name), std::move(data), flags);
  };

  if (should_ship_manifest_section(profile, options)) {
    add_package_section(venom::package::SectionType::Manifest, "manifest.txt", bytes_from_string(html_manifest(graph, &compiled_routes, "asset-manifest.txt")));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::Integrity, "profile-diagnostics.txt", bytes_from_string(describe_profile(profile)));
  }
  add_package_section(venom::package::SectionType::Integrity, "runtime-policy.vhrd", bytes_from_string(make_runtime_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::Integrity, "package-binding.vbind", bytes_from_string(make_package_binding_metadata(profile, options, package_binding_token, runtime_name, runtime, wasm_name, wasm_bytes, style_name, css, quickjs_engine_name, quickjs_engine_module, quickjs_wasm_name, quickjs_wasm_bytes, worker_name, worker_runtime)));
  {
    std::ostringstream closure;
    closure << "VENOM_PROTECTION_CLOSURE_V1\n"
            << "requested=" << js_bridge.protected_intents_requested << "\n"
            << "resolved=" << js_bridge.protected_intents_resolved << "\n"
            << "expected_quickjs_records=" << js_bridge.expected_quickjs_records << "\n";
    add_package_section(venom::package::SectionType::Integrity, "protection-closure.vpcl", bytes_from_string(closure.str()));
  }
  add_package_section(venom::package::SectionType::Integrity, "release-profile.vrpf", bytes_from_string(make_release_profile_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::Integrity, "remote-vendors.vrvd", bytes_from_string(make_remote_vendor_metadata(js_bridge.remote_vendors, remote_vendor_options.offline, remote_vendor_options.cache_directory, vendor_lock_path, vendor_lock_digest, vendor_lock_mode)));
  add_package_section(venom::package::SectionType::QuickJsReleaseFailClosed, "quickjs-release-build-policy.vqbp", bytes_from_string(make_release_build_policy_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::HostBridge, "host-calls.vhcb", bytes_from_string(make_host_bridge_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::FetchBridge, "fetch-bridge.vfch", bytes_from_string(make_fetch_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TimerBridge, "timer-bridge.vtmr", bytes_from_string(make_timer_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::EventQueue, "event-queue.vevq", bytes_from_string(make_event_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsBridge, "quickjs-bridge.vqjs", bytes_from_string(make_quickjs_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::ScriptIsolation, "script-isolation.vsis", bytes_from_string(make_script_isolation_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::ScriptPolicy, "script-policy.vscp", bytes_from_string(make_script_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsChunks, "quickjs-chunks.vqbc", bytes_from_string(make_quickjs_chunk_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsEngine, "quickjs-engine.vqse", bytes_from_string(make_quickjs_engine_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsEngineModule, "quickjs-engine-module.vqem", bytes_from_string(make_quickjs_engine_module_metadata(profile, options.runtime, js_bridge, quickjs_engine_name, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsWasmRuntime, "quickjs-wasm-runtime.vqwr", bytes_from_string(make_quickjs_wasm_runtime_metadata(profile, options.runtime, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsEngineBackend, "quickjs-wasm-execution.vqwe", bytes_from_string(make_quickjs_wasm_execution_metadata(profile, options, release_policy, js_bridge, quickjs_wasm_name)));
  add_package_section(venom::package::SectionType::QuickJsSourceTransfer, "quickjs-source-transfer.vqst", bytes_from_string(make_quickjs_source_transfer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsConsoleBridge, "quickjs-console-bridge.vqcb", bytes_from_string(make_quickjs_console_bridge_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionRecords, "quickjs-execution-records.vqer", bytes_from_string(make_quickjs_execution_records_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::QuickJsResultBridge, "quickjs-result-bridge.vqrb", bytes_from_string(make_quickjs_result_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsFallbackPolicy, "quickjs-fallback-policy.vqfp", bytes_from_string(make_quickjs_fallback_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsEngineBackend, "quickjs-engine-backend.vqeb", bytes_from_string(make_quickjs_engine_backend_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsNativeParity, "quickjs-native-parity.vqnp", bytes_from_string(make_quickjs_native_parity_metadata(profile, options.runtime, js_bridge, qjs_probe)));
  add_package_section(venom::package::SectionType::QuickJsExecutionMode, "quickjs-execution-mode.vqxm", bytes_from_string(make_quickjs_execution_mode_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsRuntimeAbi, "quickjs-runtime-abi.vqra", bytes_from_string(make_quickjs_runtime_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostImports, "quickjs-host-imports.vqhi", bytes_from_string(make_quickjs_host_imports_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHeapLimits, "quickjs-heap-limits.vqhl", bytes_from_string(make_quickjs_heap_limits_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsScriptBuffer, "quickjs-script-buffer.vqsb", bytes_from_string(make_quickjs_script_buffer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsConsoleAbi, "quickjs-console-abi.vqca", bytes_from_string(make_quickjs_console_abi_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsParityProbe, "quickjs-parity-probe.vqpp", bytes_from_string(make_quickjs_parity_probe_metadata(profile, options.runtime, qjs_probe)));
  }
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseFallback, "quickjs-release-fallback.vqrf", bytes_from_string(make_quickjs_release_fallback_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsBytecodeManifest, "quickjs-bytecode-manifest.vqbm", bytes_from_string(make_quickjs_bytecode_manifest_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsModuleResolver, "quickjs-module-resolver.vqmr", bytes_from_string(make_quickjs_module_resolver_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsExceptionAbi, "quickjs-exception-abi.vqex", bytes_from_string(make_quickjs_exception_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostTrapPolicy, "quickjs-host-trap-policy.vqtp", bytes_from_string(make_quickjs_host_trap_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsExecutionLifecycle, "quickjs-execution-lifecycle.vqel", bytes_from_string(make_quickjs_execution_lifecycle_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsScriptBufferPolicy, "quickjs-script-buffer-policy.vqsp", bytes_from_string(make_quickjs_script_buffer_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsContextLimitPolicy, "quickjs-context-limit-policy.vqlp", bytes_from_string(make_quickjs_context_limit_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostCallDispatch, "quickjs-host-call-dispatch.vqhd", bytes_from_string(make_quickjs_host_call_dispatch_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsParityContract, "quickjs-parity-contract.vqpc", bytes_from_string(make_quickjs_parity_contract_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseFailClosed, "quickjs-release-failclosed.vqfc", bytes_from_string(make_quickjs_release_failclosed_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsModuleGraph, "quickjs-module-graph.vqmg", bytes_from_string(make_quickjs_module_graph_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::QuickJsModuleExecution, "quickjs-module-execution.vqmx", bytes_from_string(make_quickjs_module_execution_metadata(profile, options.runtime, js_bridge)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsModuleCache, "quickjs-module-cache.vqmc", bytes_from_string(make_quickjs_module_cache_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsResolverAudit, "quickjs-resolver-audit.vqra", bytes_from_string(make_quickjs_resolver_audit_metadata(profile, options.runtime, js_bridge)));
  }
  }
  add_package_section(venom::package::SectionType::QuickJsInteropFallback, "quickjs-interop-fallback.vqif", bytes_from_string(make_quickjs_interop_fallback_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsExecutionPipeline, "quickjs-execution-pipeline.vqxp", bytes_from_string(make_quickjs_execution_pipeline_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::QuickJsScriptRecords, "quickjs-script-records.vqsr", bytes_from_string(make_quickjs_script_records_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsEvalResults, "quickjs-eval-results.vqev", bytes_from_string(make_quickjs_eval_results_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsConsoleCapture, "quickjs-console-capture.vqcc", bytes_from_string(make_quickjs_console_capture_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::QuickJsFailureReports, "quickjs-failure-reports.vqfr", bytes_from_string(make_quickjs_failure_reports_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionJournal, "quickjs-execution-journal.vqxj", bytes_from_string(make_quickjs_execution_journal_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCheckpointPolicy, "quickjs-checkpoint-policy.vqcp", bytes_from_string(make_quickjs_checkpoint_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReplayCursor, "quickjs-replay-cursor.vqrc", bytes_from_string(make_quickjs_replay_cursor_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsResumeState, "quickjs-resume-state.vqrs", bytes_from_string(make_quickjs_resume_state_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsDeterminismAudit, "quickjs-determinism-audit.vqda", bytes_from_string(make_quickjs_determinism_audit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsSnapshotPolicy, "quickjs-snapshot-policy.vqsk", bytes_from_string(make_quickjs_snapshot_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsSnapshotRecords, "quickjs-snapshot-records.vqsn", bytes_from_string(make_quickjs_snapshot_records_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReplayValidation, "quickjs-replay-validation.vqrv", bytes_from_string(make_quickjs_replay_validation_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsDeterminismLedger, "quickjs-determinism-ledger.vqdl", bytes_from_string(make_quickjs_determinism_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsAuditSeal, "quickjs-audit-seal.vqas", bytes_from_string(make_quickjs_audit_seal_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsExecutionCommit, "quickjs-execution-commit.vqxc", bytes_from_string(make_quickjs_execution_commit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsRollbackPolicy, "quickjs-rollback-policy.vqrp", bytes_from_string(make_quickjs_rollback_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostCallReceipts, "quickjs-host-call-receipts.vqhr", bytes_from_string(make_quickjs_host_call_receipts_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsReleaseAcceptance, "quickjs-release-acceptance.vqac", bytes_from_string(make_quickjs_release_acceptance_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCommitAudit, "quickjs-commit-audit.vqca", bytes_from_string(make_quickjs_commit_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsCapabilityPolicy, "quickjs-capability-policy.vqcpol", bytes_from_string(make_quickjs_capability_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsHostIoPolicy, "quickjs-host-io-policy.vqio", bytes_from_string(make_quickjs_host_io_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsPermissionSeal, "quickjs-permission-seal.vqps", bytes_from_string(make_quickjs_permission_seal_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsPolicyReceipts, "quickjs-policy-receipts.vqpr", bytes_from_string(make_quickjs_policy_receipts_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsReleaseGate, "quickjs-release-gate.vqrg", bytes_from_string(make_quickjs_release_gate_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostIoDecision, "quickjs-host-io-decision.vqid", bytes_from_string(make_quickjs_host_io_decision_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsHostIoDenyTrace, "quickjs-host-io-deny-trace.vqdt", bytes_from_string(make_quickjs_host_io_deny_trace_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsCapabilityLedger, "quickjs-capability-ledger.vqclg", bytes_from_string(make_quickjs_capability_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsPolicySealAudit, "quickjs-policy-seal-audit.vqpsa", bytes_from_string(make_quickjs_policy_seal_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::QuickJsRuntimeDenylist, "quickjs-runtime-denylist.vqrd", bytes_from_string(make_quickjs_runtime_denylist_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::QuickJsContextLifecycle, "quickjs-context-lifecycle.vqcl", bytes_from_string(make_quickjs_context_lifecycle_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::HostCapabilities, "host-capabilities.vhcap", bytes_from_string(make_host_capabilities_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJsAdapterDiagnostics, "quickjs-adapter-diagnostics.vqad", bytes_from_string(make_quickjs_adapter_diagnostics_metadata(profile, options.runtime, js_bridge, quickjs_engine_name)));
  }
  add_package_section(venom::package::SectionType::ScriptEnginePolicy, "script-engine-policy.vsep", bytes_from_string(make_script_engine_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::AsyncHostQueue, "async-host-queue.vahq", bytes_from_string(make_async_host_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::Routes, "routes.vbrt", compiled_routes.route_table);
  add_package_section(venom::package::SectionType::Css, style_name, bytes_from_string(css));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::JavaScript, "scripts.vjsb", js_bridge.bundle);
  }
  if (!js_bridge.lazy_protected_manifest_json.empty()) {
    add_package_section(venom::package::SectionType::Integrity,
                        "lazy-protected-manifest.vlpm",
                        bytes_from_string(js_bridge.lazy_protected_manifest_json));
  }
  if (!external_lazy_registry && !js_bridge.bridge_registry_bytecode.empty()) {
    auto protected_registry = encode_protected_bridge_registry(js_bridge, remote_vendor_options.bridge_id_salt);
    if (protected_registry.empty() || count_marker(protected_registry, "VQJSE006") != 1u) {
      raise_error("VENOM-E5000", "protected bridge registry was not emitted as exactly one VQJSE006 record");
    }
    add_package_section(venom::package::SectionType::JavaScript,
                        "protected-bridge-registry.vqbc",
                        std::move(protected_registry));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::JavaScript, "script-diagnostics.txt", bytes_from_string(js_bridge.diagnostics));
    add_package_section(venom::package::SectionType::JavaScript, "bundle-preview.js", bytes_from_string(js_preview));
  }
  add_package_section(venom::package::SectionType::Strings, "strings.vstr", compiled_routes.string_table);
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::DomTemplates, "dom-templates.txt", bytes_from_string(compiled_routes.preview));
  }
  add_package_section(venom::package::SectionType::VmBytecode, "route-bytecode.vmbc", compiled_routes.bytecode);

  std::vector<LazySectionPlanRow> lazy_rows;
  lazy_rows.reserve(compiled_routes.routes.size());
  for (const auto& route : compiled_routes.routes) {
    LazySectionPlanRow row;
    row.route = route.route;
    row.instruction_count = route.instruction_count;
    if (compiled_routes.routes.size() == 1u) {
      row.route_section = "route-bytecode.vmbc";
      row.route_bytecode_size = static_cast<std::uint32_t>(compiled_routes.bytecode.size());
    } else {
      row.route_section = lazy_route_section_name(route.route);
      auto route_bytes = make_single_route_bytecode_section(route.program, poly);
      row.route_bytecode_size = static_cast<std::uint32_t>(route_bytes.size());
      add_package_section(venom::package::SectionType::VmBytecode, row.route_section, std::move(route_bytes));
    }

    auto route_scripts = select_lazy_route_scripts(js_bridge.chunks, route.route);
    if (!route_scripts.empty()) {
      row.script_section = lazy_script_section_name(route.route);
      row.script_count = static_cast<std::uint32_t>(route_scripts.size());
      add_package_section(venom::package::SectionType::JavaScript, row.script_section, encode_route_script_bundle(route_scripts, js_bridge.module_graph, profile.kind == ProfileKind::Prod || profile.strip_debug_metadata));
    }
    lazy_rows.push_back(std::move(row));
  }

  add_package_section(venom::package::SectionType::Integrity, "vm-polymorph.vpol", poly.encode_binary());
  if (profile.kind == ProfileKind::Prod || options.strict_release || profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::Integrity,
                        "release-diversification.vrd3",
                        make_release_diversification_table(poly));
    add_package_section(venom::package::SectionType::Integrity,
                        "quickjs-abi-fingerprint.vqaf",
                        make_quickjs_abi_fingerprint());
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::QuickJs, "quickjs-probe.txt", bytes_from_string(qjs_probe));
    add_package_section(venom::package::SectionType::QuickJs, "quickjs-bridge-plan.txt", bytes_from_string("QuickJS native embedding is available for compile-time probes. Browser QuickJS execution now loads the v0.46 bytecode-record pipeline with the release-policy-gated QuickJS/WASM host-I/O decision enforcement boundary with ABI-table-driven route-scoped script context records, heap/accounting limits, script-buffer allocation, console callback ABI, and parity probes, bytecode manifests, module resolver contracts, exception records, and host-trap policy; the AEAD provider status remains honest.\n"));
  }
  add_package_section(venom::package::SectionType::AssetManifest, "asset-manifest.txt", bytes_from_string(assets.manifest_text));

  for (const auto& file : graph.files) {
    const auto rel = file.relative;
    const auto lower = [&]() {
      std::string value = rel;
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return value;
    }();
    const bool browser_manifest = rel == "venom.browser.json";
    const bool notice = lower == "third_party_notices.txt" || lower == "third-party-notices.txt" ||
                        (lower.size() >= 24 && lower.compare(lower.size() - 24, 24, "/third_party_notices.txt") == 0) ||
                        (lower.size() >= 24 && lower.compare(lower.size() - 24, 24, "/third-party-notices.txt") == 0);
    if (!is_code_or_markup(file) && (!production_hardening || (!browser_manifest && !notice))) {
      add_package_section(venom::package::SectionType::Asset, file.relative, file.bytes);
    }
  }

  constexpr std::uint32_t kPolymorphicLayoutMaxPadding = 31u;
  add_package_section(venom::package::SectionType::Integrity,
                      "package-layout.vlay",
                      bytes_from_string(make_package_layout_metadata(profile, options, poly, package_plan.sections(), kPolymorphicLayoutMaxPadding)));

  add_package_section(venom::package::SectionType::Integrity,
                      "lazy-sections.vlazy",
                      bytes_from_string(make_lazy_sections_metadata(profile, options, lazy_rows)));

  add_package_section(venom::package::SectionType::PackageFeatures,
                      "package-features.vfeat",
                      bytes_from_string(make_package_features_metadata(profile, options.runtime, package_plan.sections(), options.crypto_provider)));

  if (profile.integrity_metadata) {
    add_package_section(venom::package::SectionType::Integrity,
                        "integrity-auth.vsig",
                        bytes_from_string(make_integrity_metadata(profile, options.runtime, package_plan.sections(), options.crypto_provider)));
  }

  if (profile.shuffle_sections) {
    package_plan.shuffle(poly);
  }

  Result result;
  result.plan = std::move(package_plan);
  result.package_binding_token = package_binding_token;
  result.package_flags = package_flags;
  result.layout_max_padding = kPolymorphicLayoutMaxPadding;
  return result;
}

} // namespace venom::compiler::package_sections
