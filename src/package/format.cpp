#include "base/error.hpp"
#include "package/format.hpp"

#include <stdexcept>

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
    case SectionType::TurboJs: return "turbojs";
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
    case SectionType::TurboJsBridge: return "turbojs_bridge";
    case SectionType::ScriptIsolation: return "script_isolation";
    case SectionType::ScriptPolicy: return "script_policy";
    case SectionType::TurboJsChunks: return "turbojs_chunks";
    case SectionType::TurboJsEngine: return "turbojs_engine";
    case SectionType::ScriptEnginePolicy: return "script_engine_policy";
    case SectionType::TurboJsEngineModule: return "turbojs_engine_module";
    case SectionType::TurboJsContextLifecycle: return "turbojs_context_lifecycle";
    case SectionType::HostCapabilities: return "host_capabilities";
    case SectionType::TurboJsAdapterDiagnostics: return "turbojs_adapter_diagnostics";
    case SectionType::TurboJsWasmRuntime: return "turbojs_wasm_runtime";
    case SectionType::TurboJsSourceTransfer: return "turbojs_source_transfer";
    case SectionType::TurboJsConsoleBridge: return "turbojs_console_bridge";
    case SectionType::TurboJsExecutionRecords: return "turbojs_execution_records";
    case SectionType::TurboJsResultBridge: return "turbojs_result_bridge";
    case SectionType::TurboJsFallbackPolicy: return "turbojs_fallback_policy";
    case SectionType::TurboJsEngineBackend: return "turbojs_engine_backend";
    case SectionType::TurboJsNativeParity: return "turbojs_native_parity";
    case SectionType::TurboJsExecutionMode: return "turbojs_execution_mode";
    case SectionType::TurboJsRuntimeAbi: return "turbojs_runtime_abi";
    case SectionType::TurboJsHostImports: return "turbojs_host_imports";
    case SectionType::TurboJsHeapLimits: return "turbojs_heap_limits";
    case SectionType::TurboJsScriptBuffer: return "turbojs_script_buffer";
    case SectionType::TurboJsConsoleAbi: return "turbojs_console_abi";
    case SectionType::TurboJsParityProbe: return "turbojs_parity_probe";
    case SectionType::TurboJsReleaseFallback: return "turbojs_release_fallback";
    case SectionType::TurboJsBytecodeManifest: return "turbojs_bytecode_manifest";
    case SectionType::TurboJsModuleResolver: return "turbojs_module_resolver";
    case SectionType::TurboJsExceptionAbi: return "turbojs_exception_abi";
    case SectionType::TurboJsHostTrapPolicy: return "turbojs_host_trap_policy";
    case SectionType::TurboJsExecutionLifecycle: return "turbojs_execution_lifecycle";
    case SectionType::TurboJsScriptBufferPolicy: return "turbojs_script_buffer_policy";
    case SectionType::TurboJsContextLimitPolicy: return "turbojs_context_limit_policy";
    case SectionType::TurboJsHostCallDispatch: return "turbojs_host_call_dispatch";
    case SectionType::TurboJsParityContract: return "turbojs_parity_contract";
    case SectionType::TurboJsReleaseFailClosed: return "turbojs_release_failclosed";
    case SectionType::TurboJsModuleGraph: return "turbojs_module_graph";
    case SectionType::TurboJsModuleExecution: return "turbojs_module_execution";
    case SectionType::TurboJsModuleCache: return "turbojs_module_cache";
    case SectionType::TurboJsResolverAudit: return "turbojs_resolver_audit";
    case SectionType::TurboJsInteropFallback: return "turbojs_interop_fallback";
    case SectionType::TurboJsExecutionPipeline: return "turbojs_execution_pipeline";
    case SectionType::TurboJsScriptRecords: return "turbojs_script_records";
    case SectionType::TurboJsEvalResults: return "turbojs_eval_results";
    case SectionType::TurboJsConsoleCapture: return "turbojs_console_capture";
    case SectionType::TurboJsFailureReports: return "turbojs_failure_reports";
    case SectionType::TurboJsExecutionJournal: return "turbojs_execution_journal";
    case SectionType::TurboJsCheckpointPolicy: return "turbojs_checkpoint_policy";
    case SectionType::TurboJsReplayCursor: return "turbojs_replay_cursor";
    case SectionType::TurboJsResumeState: return "turbojs_resume_state";
    case SectionType::TurboJsDeterminismAudit: return "turbojs_determinism_audit";
    case SectionType::TurboJsSnapshotPolicy: return "turbojs_snapshot_policy";
    case SectionType::TurboJsSnapshotRecords: return "turbojs_snapshot_records";
    case SectionType::TurboJsReplayValidation: return "turbojs_replay_validation";
    case SectionType::TurboJsDeterminismLedger: return "turbojs_determinism_ledger";
    case SectionType::TurboJsAuditSeal: return "turbojs_audit_seal";
    case SectionType::TurboJsExecutionCommit: return "turbojs_execution_commit";
    case SectionType::TurboJsRollbackPolicy: return "turbojs_rollback_policy";
    case SectionType::TurboJsHostCallReceipts: return "turbojs_host_call_receipts";
    case SectionType::TurboJsReleaseAcceptance: return "turbojs_release_acceptance";
    case SectionType::TurboJsCommitAudit: return "turbojs_commit_audit";
    case SectionType::TurboJsCapabilityPolicy: return "turbojs_capability_policy";
    case SectionType::TurboJsHostIoPolicy: return "turbojs_host_io_policy";
    case SectionType::TurboJsPermissionSeal: return "turbojs_permission_seal";
    case SectionType::TurboJsPolicyReceipts: return "turbojs_policy_receipts";
    case SectionType::TurboJsReleaseGate: return "turbojs_release_gate";
    case SectionType::TurboJsHostIoDecision: return "turbojs_host_io_decision";
    case SectionType::TurboJsHostIoDenyTrace: return "turbojs_host_io_deny_trace";
    case SectionType::TurboJsCapabilityLedger: return "turbojs_capability_ledger";
    case SectionType::TurboJsPolicySealAudit: return "turbojs_policy_seal_audit";
    case SectionType::TurboJsRuntimeDenylist: return "turbojs_runtime_denylist";
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
    case SectionType::TurboJs:
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
    case SectionType::TurboJsBridge:
    case SectionType::ScriptIsolation:
    case SectionType::ScriptPolicy:
    case SectionType::TurboJsChunks:
    case SectionType::TurboJsEngine:
    case SectionType::ScriptEnginePolicy:
    case SectionType::TurboJsEngineModule:
    case SectionType::TurboJsContextLifecycle:
    case SectionType::HostCapabilities:
    case SectionType::TurboJsAdapterDiagnostics:
    case SectionType::TurboJsWasmRuntime:
    case SectionType::TurboJsSourceTransfer:
    case SectionType::TurboJsConsoleBridge:
    case SectionType::TurboJsExecutionRecords:
    case SectionType::TurboJsResultBridge:
    case SectionType::TurboJsFallbackPolicy:
    case SectionType::TurboJsEngineBackend:
    case SectionType::TurboJsNativeParity:
    case SectionType::TurboJsExecutionMode:
    case SectionType::TurboJsRuntimeAbi:
    case SectionType::TurboJsHostImports:
    case SectionType::TurboJsHeapLimits:
    case SectionType::TurboJsScriptBuffer:
    case SectionType::TurboJsConsoleAbi:
    case SectionType::TurboJsParityProbe:
    case SectionType::TurboJsReleaseFallback:
    case SectionType::TurboJsBytecodeManifest:
    case SectionType::TurboJsModuleResolver:
    case SectionType::TurboJsExceptionAbi:
    case SectionType::TurboJsHostTrapPolicy:
    case SectionType::TurboJsExecutionLifecycle:
    case SectionType::TurboJsScriptBufferPolicy:
    case SectionType::TurboJsContextLimitPolicy:
    case SectionType::TurboJsHostCallDispatch:
    case SectionType::TurboJsParityContract:
    case SectionType::TurboJsReleaseFailClosed:
    case SectionType::TurboJsModuleGraph:
    case SectionType::TurboJsModuleExecution:
    case SectionType::TurboJsModuleCache:
    case SectionType::TurboJsResolverAudit:
    case SectionType::TurboJsInteropFallback:
    case SectionType::TurboJsExecutionPipeline:
    case SectionType::TurboJsScriptRecords:
    case SectionType::TurboJsEvalResults:
    case SectionType::TurboJsConsoleCapture:
    case SectionType::TurboJsFailureReports:
    case SectionType::TurboJsExecutionJournal:
    case SectionType::TurboJsCheckpointPolicy:
    case SectionType::TurboJsReplayCursor:
    case SectionType::TurboJsResumeState:
    case SectionType::TurboJsDeterminismAudit:
    case SectionType::TurboJsSnapshotPolicy:
    case SectionType::TurboJsSnapshotRecords:
    case SectionType::TurboJsReplayValidation:
    case SectionType::TurboJsDeterminismLedger:
    case SectionType::TurboJsAuditSeal:
    case SectionType::TurboJsExecutionCommit:
    case SectionType::TurboJsRollbackPolicy:
    case SectionType::TurboJsHostCallReceipts:
    case SectionType::TurboJsReleaseAcceptance:
    case SectionType::TurboJsCommitAudit:
    case SectionType::TurboJsCapabilityPolicy:
    case SectionType::TurboJsHostIoPolicy:
    case SectionType::TurboJsPermissionSeal:
    case SectionType::TurboJsPolicyReceipts:
    case SectionType::TurboJsReleaseGate:
    case SectionType::TurboJsHostIoDecision:
    case SectionType::TurboJsHostIoDenyTrace:
    case SectionType::TurboJsCapabilityLedger:
    case SectionType::TurboJsPolicySealAudit:
    case SectionType::TurboJsRuntimeDenylist:
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

void validate_section_name(const std::string& name) {
  constexpr std::size_t kMaxSectionNameBytes = 1024u;
  if (name.empty()) {
    raise_error("VENOM-E4000", "package section name cannot be empty");
  }
  if (name.size() > kMaxSectionNameBytes) {
    raise_error("VENOM-E4000", "package section name exceeds maximum length");
  }
  for (const char raw_ch : name) {
    const auto ch = static_cast<unsigned char>(raw_ch);
    if (ch == 0u || ch == '\t' || ch == '\n' || ch == '\r' || ch < 0x20u || ch == 0x7fu) {
      raise_error("VENOM-E4000", "package section name contains a control character");
    }
  }
}

bool section_name_matches(SectionType type, const std::string& stored_name, const std::string& canonical_name) {
  if (stored_name == canonical_name) {
    return true;
  }
  return should_redact_section_name(type) && stored_name == protected_section_alias(type, canonical_name);
}

} // namespace venom::package
