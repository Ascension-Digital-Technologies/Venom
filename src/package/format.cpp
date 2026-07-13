#include "package/format.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace venom::package {

const char* section_type_name(SectionType type) {
  switch (type) {
    case SectionType::Manifest: return "manifest";
    case SectionType::Routes: return "routes";
    case SectionType::DomTemplates: return "dom_templates";
    case SectionType::Css: return "css";
    case SectionType::JavaScript: return "javascript";
    case SectionType::QuickJs: return "quickjs";
    case SectionType::VmBytecode: return "vm_bytecode";
    case SectionType::Asset: return "asset";
    case SectionType::Integrity: return "integrity";
    case SectionType::Strings: return "strings";
    case SectionType::AssetManifest: return "asset_manifest";
    case SectionType::HostBridge: return "host_bridge";
    case SectionType::FetchBridge: return "fetch_bridge";
    case SectionType::AsyncHostQueue: return "async_host_queue";
    case SectionType::TimerBridge: return "timer_bridge";
    case SectionType::EventQueue: return "event_queue";
    case SectionType::QuickJsBridge: return "quickjs_bridge";
    case SectionType::ScriptIsolation: return "script_isolation";
    case SectionType::ScriptPolicy: return "script_policy";
    case SectionType::QuickJsChunks: return "quickjs_chunks";
    case SectionType::QuickJsEngine: return "quickjs_engine";
    case SectionType::ScriptEnginePolicy: return "script_engine_policy";
    case SectionType::QuickJsEngineModule: return "quickjs_engine_module";
    case SectionType::QuickJsContextLifecycle: return "quickjs_context_lifecycle";
    case SectionType::HostCapabilities: return "host_capabilities";
    case SectionType::QuickJsAdapterDiagnostics: return "quickjs_adapter_diagnostics";
    case SectionType::QuickJsWasmRuntime: return "quickjs_wasm_runtime";
    case SectionType::QuickJsSourceTransfer: return "quickjs_source_transfer";
    case SectionType::QuickJsConsoleBridge: return "quickjs_console_bridge";
    case SectionType::QuickJsExecutionRecords: return "quickjs_execution_records";
    case SectionType::QuickJsResultBridge: return "quickjs_result_bridge";
    case SectionType::QuickJsFallbackPolicy: return "quickjs_fallback_policy";
    case SectionType::QuickJsEngineBackend: return "quickjs_engine_backend";
    case SectionType::QuickJsNativeParity: return "quickjs_native_parity";
    case SectionType::QuickJsExecutionMode: return "quickjs_execution_mode";
    case SectionType::QuickJsRuntimeAbi: return "quickjs_runtime_abi";
    case SectionType::QuickJsHostImports: return "quickjs_host_imports";
    case SectionType::QuickJsHeapLimits: return "quickjs_heap_limits";
    case SectionType::QuickJsScriptBuffer: return "quickjs_script_buffer";
    case SectionType::QuickJsConsoleAbi: return "quickjs_console_abi";
    case SectionType::QuickJsParityProbe: return "quickjs_parity_probe";
    case SectionType::QuickJsReleaseFallback: return "quickjs_release_fallback";
    case SectionType::QuickJsBytecodeManifest: return "quickjs_bytecode_manifest";
    case SectionType::QuickJsModuleResolver: return "quickjs_module_resolver";
    case SectionType::QuickJsExceptionAbi: return "quickjs_exception_abi";
    case SectionType::QuickJsHostTrapPolicy: return "quickjs_host_trap_policy";
    case SectionType::QuickJsExecutionLifecycle: return "quickjs_execution_lifecycle";
    case SectionType::QuickJsScriptBufferPolicy: return "quickjs_script_buffer_policy";
    case SectionType::QuickJsContextLimitPolicy: return "quickjs_context_limit_policy";
    case SectionType::QuickJsHostCallDispatch: return "quickjs_host_call_dispatch";
    case SectionType::QuickJsParityContract: return "quickjs_parity_contract";
    case SectionType::QuickJsReleaseFailClosed: return "quickjs_release_failclosed";
    case SectionType::QuickJsModuleGraph: return "quickjs_module_graph";
    case SectionType::QuickJsModuleExecution: return "quickjs_module_execution";
    case SectionType::QuickJsModuleCache: return "quickjs_module_cache";
    case SectionType::QuickJsResolverAudit: return "quickjs_resolver_audit";
    case SectionType::QuickJsInteropFallback: return "quickjs_interop_fallback";
    case SectionType::QuickJsExecutionPipeline: return "quickjs_execution_pipeline";
    case SectionType::QuickJsScriptRecords: return "quickjs_script_records";
    case SectionType::QuickJsEvalResults: return "quickjs_eval_results";
    case SectionType::QuickJsConsoleCapture: return "quickjs_console_capture";
    case SectionType::QuickJsFailureReports: return "quickjs_failure_reports";
    case SectionType::QuickJsExecutionJournal: return "quickjs_execution_journal";
    case SectionType::QuickJsCheckpointPolicy: return "quickjs_checkpoint_policy";
    case SectionType::QuickJsReplayCursor: return "quickjs_replay_cursor";
    case SectionType::QuickJsResumeState: return "quickjs_resume_state";
    case SectionType::QuickJsDeterminismAudit: return "quickjs_determinism_audit";
    case SectionType::QuickJsSnapshotPolicy: return "quickjs_snapshot_policy";
    case SectionType::QuickJsSnapshotRecords: return "quickjs_snapshot_records";
    case SectionType::QuickJsReplayValidation: return "quickjs_replay_validation";
    case SectionType::QuickJsDeterminismLedger: return "quickjs_determinism_ledger";
    case SectionType::QuickJsAuditSeal: return "quickjs_audit_seal";
    case SectionType::QuickJsExecutionCommit: return "quickjs_execution_commit";
    case SectionType::QuickJsRollbackPolicy: return "quickjs_rollback_policy";
    case SectionType::QuickJsHostCallReceipts: return "quickjs_host_call_receipts";
    case SectionType::QuickJsReleaseAcceptance: return "quickjs_release_acceptance";
    case SectionType::QuickJsCommitAudit: return "quickjs_commit_audit";
    case SectionType::QuickJsCapabilityPolicy: return "quickjs_capability_policy";
    case SectionType::QuickJsHostIoPolicy: return "quickjs_host_io_policy";
    case SectionType::QuickJsPermissionSeal: return "quickjs_permission_seal";
    case SectionType::QuickJsPolicyReceipts: return "quickjs_policy_receipts";
    case SectionType::QuickJsReleaseGate: return "quickjs_release_gate";
    case SectionType::QuickJsHostIoDecision: return "quickjs_host_io_decision";
    case SectionType::QuickJsHostIoDenyTrace: return "quickjs_host_io_deny_trace";
    case SectionType::QuickJsCapabilityLedger: return "quickjs_capability_ledger";
    case SectionType::QuickJsPolicySealAudit: return "quickjs_policy_seal_audit";
    case SectionType::QuickJsRuntimeDenylist: return "quickjs_runtime_denylist";
    case SectionType::PackageFeatures: return "package_features";
  }
  return "unknown";
}

bool is_known_section_type(std::uint32_t value) {
  switch (static_cast<SectionType>(value)) {
    case SectionType::Manifest:
    case SectionType::Routes:
    case SectionType::DomTemplates:
    case SectionType::Css:
    case SectionType::JavaScript:
    case SectionType::QuickJs:
    case SectionType::VmBytecode:
    case SectionType::Asset:
    case SectionType::Integrity:
    case SectionType::Strings:
    case SectionType::AssetManifest:
    case SectionType::HostBridge:
    case SectionType::FetchBridge:
    case SectionType::AsyncHostQueue:
    case SectionType::TimerBridge:
    case SectionType::EventQueue:
    case SectionType::QuickJsBridge:
    case SectionType::ScriptIsolation:
    case SectionType::ScriptPolicy:
    case SectionType::QuickJsChunks:
    case SectionType::QuickJsEngine:
    case SectionType::ScriptEnginePolicy:
    case SectionType::QuickJsEngineModule:
    case SectionType::QuickJsContextLifecycle:
    case SectionType::HostCapabilities:
    case SectionType::QuickJsAdapterDiagnostics:
    case SectionType::QuickJsWasmRuntime:
    case SectionType::QuickJsSourceTransfer:
    case SectionType::QuickJsConsoleBridge:
    case SectionType::QuickJsExecutionRecords:
    case SectionType::QuickJsResultBridge:
    case SectionType::QuickJsFallbackPolicy:
    case SectionType::QuickJsEngineBackend:
    case SectionType::QuickJsNativeParity:
    case SectionType::QuickJsExecutionMode:
    case SectionType::QuickJsRuntimeAbi:
    case SectionType::QuickJsHostImports:
    case SectionType::QuickJsHeapLimits:
    case SectionType::QuickJsScriptBuffer:
    case SectionType::QuickJsConsoleAbi:
    case SectionType::QuickJsParityProbe:
    case SectionType::QuickJsReleaseFallback:
    case SectionType::QuickJsBytecodeManifest:
    case SectionType::QuickJsModuleResolver:
    case SectionType::QuickJsExceptionAbi:
    case SectionType::QuickJsHostTrapPolicy:
    case SectionType::QuickJsExecutionLifecycle:
    case SectionType::QuickJsScriptBufferPolicy:
    case SectionType::QuickJsContextLimitPolicy:
    case SectionType::QuickJsHostCallDispatch:
    case SectionType::QuickJsParityContract:
    case SectionType::QuickJsReleaseFailClosed:
    case SectionType::QuickJsModuleGraph:
    case SectionType::QuickJsModuleExecution:
    case SectionType::QuickJsModuleCache:
    case SectionType::QuickJsResolverAudit:
    case SectionType::QuickJsInteropFallback:
    case SectionType::QuickJsExecutionPipeline:
    case SectionType::QuickJsScriptRecords:
    case SectionType::QuickJsEvalResults:
    case SectionType::QuickJsConsoleCapture:
    case SectionType::QuickJsFailureReports:
    case SectionType::QuickJsExecutionJournal:
    case SectionType::QuickJsCheckpointPolicy:
    case SectionType::QuickJsReplayCursor:
    case SectionType::QuickJsResumeState:
    case SectionType::QuickJsDeterminismAudit:
    case SectionType::QuickJsSnapshotPolicy:
    case SectionType::QuickJsSnapshotRecords:
    case SectionType::QuickJsReplayValidation:
    case SectionType::QuickJsDeterminismLedger:
    case SectionType::QuickJsAuditSeal:
    case SectionType::QuickJsExecutionCommit:
    case SectionType::QuickJsRollbackPolicy:
    case SectionType::QuickJsHostCallReceipts:
    case SectionType::QuickJsReleaseAcceptance:
    case SectionType::QuickJsCommitAudit:
    case SectionType::QuickJsCapabilityPolicy:
    case SectionType::QuickJsHostIoPolicy:
    case SectionType::QuickJsPermissionSeal:
    case SectionType::QuickJsPolicyReceipts:
    case SectionType::QuickJsReleaseGate:
    case SectionType::QuickJsHostIoDecision:
    case SectionType::QuickJsHostIoDenyTrace:
    case SectionType::QuickJsCapabilityLedger:
    case SectionType::QuickJsPolicySealAudit:
    case SectionType::QuickJsRuntimeDenylist:
    case SectionType::PackageFeatures:
      return true;
  }
  return false;
}

namespace {
std::uint64_t alias_fnv1a64(const unsigned char* data, std::size_t size, std::uint64_t seed = 1469598103934665603ull) {
  std::uint64_t h = seed;
  for (std::size_t i = 0; i < size; ++i) {
    h ^= static_cast<std::uint64_t>(data[i]);
    h *= 1099511628211ull;
  }
  return h;
}

char hex_nibble(unsigned value) {
  return static_cast<char>(value < 10u ? ('0' + value) : ('a' + (value - 10u)));
}

std::string hex64(std::uint64_t value) {
  std::string out(16, '0');
  for (int i = 15; i >= 0; --i) {
    out[static_cast<std::size_t>(i)] = hex_nibble(static_cast<unsigned>(value & 0x0full));
    value >>= 4u;
  }
  return out;
}
} // namespace

bool should_redact_section_name(SectionType type) {
  // Public assets are intentionally addressable by browser URL and therefore
  // keep their path names. Everything else can use a deterministic internal
  // alias in release/protect packages so the VBC name table does not advertise
  // route/script/policy section purposes.
  return type != SectionType::Asset;
}

std::string protected_section_alias(SectionType type, const std::string& canonical_name) {
  std::uint64_t h = 1469598103934665603ull;
  const auto raw_type = static_cast<std::uint32_t>(type);
  for (int i = 0; i < 4; ++i) {
    const unsigned char byte = static_cast<unsigned char>((raw_type >> (i * 8)) & 0xffu);
    h = alias_fnv1a64(&byte, 1u, h);
  }
  h = alias_fnv1a64(reinterpret_cast<const unsigned char*>(canonical_name.data()), canonical_name.size(), h);
  return std::string("s.") + hex64(h);
}

bool section_name_matches(SectionType type, const std::string& stored_name, const std::string& canonical_name) {
  if (stored_name == canonical_name) {
    return true;
  }
  return should_redact_section_name(type) && stored_name == protected_section_alias(type, canonical_name);
}

} // namespace venom::package
