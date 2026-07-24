#include "base/error.hpp"
#include "pipeline/planning/section_plan.hpp"

#include "pipeline/build_support.hpp"
#include "pipeline/build_runtime_metadata.hpp"
#include "pipeline/build_report.hpp"
#include "package/format.hpp"

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
  const auto& turbojs_engine_name = input.turbojs_engine_name;
  const auto& turbojs_engine_module = input.turbojs_engine_source;
  const auto& turbojs_wasm_name = input.turbojs_wasm_name;
  const auto& turbojs_wasm_bytes = input.turbojs_wasm_bytes;
  const auto& worker_name = input.worker_name;
  const auto& worker_runtime = input.worker_source;
  const auto& tjs_probe = input.turbojs_probe;
  const auto& js_preview = input.js_preview;
  const bool wasm_runtime = input.wasm_runtime;
  const bool hardened_release_asset = input.hardened_release_asset;
  const bool production_hardening = input.production_hardening;
  const bool external_lazy_registry = input.external_lazy_registry;
  const auto package_binding_token = make_package_binding_token(options.diversification_seed != 0u || profile.deterministic, profile.name + "|" + options.runtime + "|" + runtime_name + "|" + wasm_name + "|" + turbojs_engine_name + "|" + turbojs_wasm_name + "|" + worker_name);
  const auto package_flags = profile.package_flags |
      ((options.strict_release || profile.kind == ProfileKind::Prod) ? venom::package::PackageFlagReleaseProfile : 0u) |
      venom::package::PackageFlagTimerBridge |
      venom::package::PackageFlagEventQueue |
      venom::package::PackageFlagTurboJsBridge |
      venom::package::PackageFlagScriptIsolation |
      venom::package::PackageFlagScriptPolicy |
      venom::package::PackageFlagTurboJsChunks |
      venom::package::PackageFlagTurboJsEngine |
      venom::package::PackageFlagScriptEngineFallback |
      venom::package::PackageFlagTurboJsEngineModule |
      venom::package::PackageFlagTurboJsContextLifecycle |
      venom::package::PackageFlagHostCapabilities |
      venom::package::PackageFlagTurboJsAdapterDiagnostics |
      venom::package::PackageFlagTurboJsWasmRuntime |
      venom::package::PackageFlagTurboJsSourceTransfer |
      venom::package::PackageFlagTurboJsConsoleBridge |
      venom::package::PackageFlagTurboJsExecutionRecords |
      venom::package::PackageFlagTurboJsResultBridge |
      venom::package::PackageFlagTurboJsFallbackPolicy |
      venom::package::PackageFlagTurboJsEngineBackend |
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
  add_package_section(venom::package::SectionType::Integrity, "package-binding.vbind", bytes_from_string(make_package_binding_metadata(profile, options, package_binding_token, runtime_name, runtime, wasm_name, wasm_bytes, style_name, css, turbojs_engine_name, turbojs_engine_module, turbojs_wasm_name, turbojs_wasm_bytes, worker_name, worker_runtime)));
  {
    std::ostringstream closure;
    closure << "VENOM_PROTECTION_CLOSURE_V1\n"
            << "requested=" << js_bridge.protected_intents_requested << "\n"
            << "resolved=" << js_bridge.protected_intents_resolved << "\n"
            << "expected_turbojs_records=" << js_bridge.expected_turbojs_records << "\n";
    add_package_section(venom::package::SectionType::Integrity, "protection-closure.vpcl", bytes_from_string(closure.str()));
  }
  add_package_section(venom::package::SectionType::Integrity, "release-profile.vrpf", bytes_from_string(make_release_profile_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::Integrity, "remote-vendors.vrvd", bytes_from_string(make_remote_vendor_metadata(js_bridge.remote_vendors, remote_vendor_options.offline, remote_vendor_options.cache_directory, vendor_lock_path, vendor_lock_digest, vendor_lock_mode)));
  add_package_section(venom::package::SectionType::TurboJsReleaseFailClosed, "turbojs-release-build-policy.vqbp", bytes_from_string(make_release_build_policy_metadata(profile, options, release_policy, js_bridge.chunks.size())));
  add_package_section(venom::package::SectionType::HostBridge, "host-calls.vhcb", bytes_from_string(make_host_bridge_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::FetchBridge, "fetch-bridge.vfch", bytes_from_string(make_fetch_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TimerBridge, "timer-bridge.vtmr", bytes_from_string(make_timer_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::EventQueue, "event-queue.vevq", bytes_from_string(make_event_queue_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsBridge, "turbojs-bridge.vtjs", bytes_from_string(make_turbojs_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::ScriptIsolation, "script-isolation.vsis", bytes_from_string(make_script_isolation_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::ScriptPolicy, "script-policy.vscp", bytes_from_string(make_script_policy_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsChunks, "turbojs-chunks.vqbc", bytes_from_string(make_turbojs_chunk_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsEngine, "turbojs-engine.vqse", bytes_from_string(make_turbojs_engine_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsEngineModule, "turbojs-engine-module.vqem", bytes_from_string(make_turbojs_engine_module_metadata(profile, options.runtime, js_bridge, turbojs_engine_name, turbojs_wasm_name)));
  add_package_section(venom::package::SectionType::TurboJsWasmRuntime, "turbojs-wasm-runtime.vqwr", bytes_from_string(make_turbojs_wasm_runtime_metadata(profile, options.runtime, turbojs_wasm_name)));
  add_package_section(venom::package::SectionType::TurboJsEngineBackend, "turbojs-wasm-execution.vqwe", bytes_from_string(make_turbojs_wasm_execution_metadata(profile, options, release_policy, js_bridge, turbojs_wasm_name)));
  add_package_section(venom::package::SectionType::TurboJsSourceTransfer, "turbojs-source-transfer.vqst", bytes_from_string(make_turbojs_source_transfer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsConsoleBridge, "turbojs-console-bridge.vqcb", bytes_from_string(make_turbojs_console_bridge_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsExecutionRecords, "turbojs-execution-records.vqer", bytes_from_string(make_turbojs_execution_records_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::TurboJsResultBridge, "turbojs-result-bridge.vqrb", bytes_from_string(make_turbojs_result_bridge_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsFallbackPolicy, "turbojs-fallback-policy.vqfp", bytes_from_string(make_turbojs_fallback_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsEngineBackend, "turbojs-engine-backend.vqeb", bytes_from_string(make_turbojs_engine_backend_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsNativeParity, "turbojs-native-parity.vqnp", bytes_from_string(make_turbojs_native_parity_metadata(profile, options.runtime, js_bridge, tjs_probe)));
  add_package_section(venom::package::SectionType::TurboJsExecutionMode, "turbojs-execution-mode.vqxm", bytes_from_string(make_turbojs_execution_mode_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsRuntimeAbi, "turbojs-runtime-abi.vqra", bytes_from_string(make_turbojs_runtime_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsHostImports, "turbojs-host-imports.vqhi", bytes_from_string(make_turbojs_host_imports_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsHeapLimits, "turbojs-heap-limits.vqhl", bytes_from_string(make_turbojs_heap_limits_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsScriptBuffer, "turbojs-script-buffer.vqsb", bytes_from_string(make_turbojs_script_buffer_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsConsoleAbi, "turbojs-console-abi.vqca", bytes_from_string(make_turbojs_console_abi_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsParityProbe, "turbojs-parity-probe.vqpp", bytes_from_string(make_turbojs_parity_probe_metadata(profile, options.runtime, tjs_probe)));
  }
  }
  add_package_section(venom::package::SectionType::TurboJsReleaseFallback, "turbojs-release-fallback.vqrf", bytes_from_string(make_turbojs_release_fallback_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsBytecodeManifest, "turbojs-bytecode-manifest.vqbm", bytes_from_string(make_turbojs_bytecode_manifest_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsModuleResolver, "turbojs-module-resolver.vqmr", bytes_from_string(make_turbojs_module_resolver_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsExceptionAbi, "turbojs-exception-abi.vqex", bytes_from_string(make_turbojs_exception_abi_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsHostTrapPolicy, "turbojs-host-trap-policy.vqtp", bytes_from_string(make_turbojs_host_trap_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsExecutionLifecycle, "turbojs-execution-lifecycle.vqel", bytes_from_string(make_turbojs_execution_lifecycle_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsScriptBufferPolicy, "turbojs-script-buffer-policy.vqsp", bytes_from_string(make_turbojs_script_buffer_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsContextLimitPolicy, "turbojs-context-limit-policy.vqlp", bytes_from_string(make_turbojs_context_limit_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsHostCallDispatch, "turbojs-host-call-dispatch.vqhd", bytes_from_string(make_turbojs_host_call_dispatch_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsParityContract, "turbojs-parity-contract.vqpc", bytes_from_string(make_turbojs_parity_contract_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::TurboJsReleaseFailClosed, "turbojs-release-failclosed.vqfc", bytes_from_string(make_turbojs_release_failclosed_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsModuleGraph, "turbojs-module-graph.vqmg", bytes_from_string(make_turbojs_module_graph_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::TurboJsModuleExecution, "turbojs-module-execution.vqmx", bytes_from_string(make_turbojs_module_execution_metadata(profile, options.runtime, js_bridge)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsModuleCache, "turbojs-module-cache.vqmc", bytes_from_string(make_turbojs_module_cache_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsResolverAudit, "turbojs-resolver-audit.vqra", bytes_from_string(make_turbojs_resolver_audit_metadata(profile, options.runtime, js_bridge)));
  }
  }
  add_package_section(venom::package::SectionType::TurboJsInteropFallback, "turbojs-interop-fallback.vqif", bytes_from_string(make_turbojs_interop_fallback_metadata(profile, options.runtime)));
  if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsExecutionPipeline, "turbojs-execution-pipeline.vqxp", bytes_from_string(make_turbojs_execution_pipeline_metadata(profile, options.runtime, js_bridge)));
  }
  add_package_section(venom::package::SectionType::TurboJsScriptRecords, "turbojs-script-records.vqsr", bytes_from_string(make_turbojs_script_records_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsEvalResults, "turbojs-eval-results.vqev", bytes_from_string(make_turbojs_eval_results_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsConsoleCapture, "turbojs-console-capture.vqcc", bytes_from_string(make_turbojs_console_capture_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    if (!hardened_release_asset) {
    add_package_section(venom::package::SectionType::TurboJsFailureReports, "turbojs-failure-reports.vqfr", bytes_from_string(make_turbojs_failure_reports_metadata(profile, options.runtime)));
  }
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsExecutionJournal, "turbojs-execution-journal.vqxj", bytes_from_string(make_turbojs_execution_journal_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsCheckpointPolicy, "turbojs-checkpoint-policy.vqcp", bytes_from_string(make_turbojs_checkpoint_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsReplayCursor, "turbojs-replay-cursor.vqrc", bytes_from_string(make_turbojs_replay_cursor_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsResumeState, "turbojs-resume-state.vqrs", bytes_from_string(make_turbojs_resume_state_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsDeterminismAudit, "turbojs-determinism-audit.vqda", bytes_from_string(make_turbojs_determinism_audit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsSnapshotPolicy, "turbojs-snapshot-policy.vqsk", bytes_from_string(make_turbojs_snapshot_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsSnapshotRecords, "turbojs-snapshot-records.vqsn", bytes_from_string(make_turbojs_snapshot_records_metadata(profile, options.runtime, js_bridge)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsReplayValidation, "turbojs-replay-validation.vqrv", bytes_from_string(make_turbojs_replay_validation_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsDeterminismLedger, "turbojs-determinism-ledger.vqdl", bytes_from_string(make_turbojs_determinism_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsAuditSeal, "turbojs-audit-seal.vqas", bytes_from_string(make_turbojs_audit_seal_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsExecutionCommit, "turbojs-execution-commit.vqxc", bytes_from_string(make_turbojs_execution_commit_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsRollbackPolicy, "turbojs-rollback-policy.vqrp", bytes_from_string(make_turbojs_rollback_policy_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsHostCallReceipts, "turbojs-host-call-receipts.vqhr", bytes_from_string(make_turbojs_host_call_receipts_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsReleaseAcceptance, "turbojs-release-acceptance.vqac", bytes_from_string(make_turbojs_release_acceptance_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsCommitAudit, "turbojs-commit-audit.vqca", bytes_from_string(make_turbojs_commit_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::TurboJsCapabilityPolicy, "turbojs-capability-policy.vqcpol", bytes_from_string(make_turbojs_capability_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsHostIoPolicy, "turbojs-host-io-policy.vqio", bytes_from_string(make_turbojs_host_io_policy_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsPermissionSeal, "turbojs-permission-seal.vqps", bytes_from_string(make_turbojs_permission_seal_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsPolicyReceipts, "turbojs-policy-receipts.vqpr", bytes_from_string(make_turbojs_policy_receipts_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::TurboJsReleaseGate, "turbojs-release-gate.vqrg", bytes_from_string(make_turbojs_release_gate_metadata(profile, options.runtime)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsHostIoDecision, "turbojs-host-io-decision.vqid", bytes_from_string(make_turbojs_host_io_decision_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsHostIoDenyTrace, "turbojs-host-io-deny-trace.vqdt", bytes_from_string(make_turbojs_host_io_deny_trace_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsCapabilityLedger, "turbojs-capability-ledger.vqclg", bytes_from_string(make_turbojs_capability_ledger_metadata(profile, options.runtime)));
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsPolicySealAudit, "turbojs-policy-seal-audit.vqpsa", bytes_from_string(make_turbojs_policy_seal_audit_metadata(profile, options.runtime)));
  }
  add_package_section(venom::package::SectionType::TurboJsRuntimeDenylist, "turbojs-runtime-denylist.vqrd", bytes_from_string(make_turbojs_runtime_denylist_metadata(profile, options.runtime)));
  add_package_section(venom::package::SectionType::TurboJsContextLifecycle, "turbojs-context-lifecycle.vqcl", bytes_from_string(make_turbojs_context_lifecycle_metadata(profile, options.runtime, js_bridge)));
  add_package_section(venom::package::SectionType::HostCapabilities, "host-capabilities.vhcap", bytes_from_string(make_host_capabilities_metadata(profile, options.runtime, js_bridge)));
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJsAdapterDiagnostics, "turbojs-adapter-diagnostics.vqad", bytes_from_string(make_turbojs_adapter_diagnostics_metadata(profile, options.runtime, js_bridge, turbojs_engine_name)));
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
    if (protected_registry.empty() || count_marker(protected_registry, "VTJSE006") != 1u) {
      raise_error("VENOM-E5000", "protected bridge registry was not emitted as exactly one VTJSE006 record");
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
                        "turbojs-abi-fingerprint.vqaf",
                        make_turbojs_abi_fingerprint());
  }
  if (!profile.strip_debug_metadata) {
    add_package_section(venom::package::SectionType::TurboJs, "turbojs-probe.txt", bytes_from_string(tjs_probe));
    add_package_section(venom::package::SectionType::TurboJs, "turbojs-bridge-plan.txt", bytes_from_string("TurboJS native embedding is available for compile-time probes. Browser TurboJS execution now loads the v0.46 bytecode-record pipeline with the release-policy-gated TurboJS/WASM host-I/O decision enforcement boundary with ABI-table-driven route-scoped script context records, heap/accounting limits, script-buffer allocation, console callback ABI, and parity probes, bytecode manifests, module resolver contracts, exception records, and host-trap policy; the AEAD provider status remains honest.\n"));
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
