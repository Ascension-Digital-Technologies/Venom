#include "quickjs/bridge.hpp"

#include "package/hash.hpp"

#include <sstream>
#include <vector>

namespace venom::quickjs {

const char* lifecycle_state_name(EngineLifecycleState state) noexcept {
  switch (state) {
    case EngineLifecycleState::Created: return "created";
    case EngineLifecycleState::Configured: return "configured";
    case EngineLifecycleState::Loaded: return "loaded";
    case EngineLifecycleState::Executing: return "executing";
    case EngineLifecycleState::Trapped: return "trapped";
    case EngineLifecycleState::Disposed: return "disposed";
  }
  return "unknown";
}

bool lifecycle_transition_allowed(EngineLifecycleState from, EngineLifecycleState to) noexcept {
  if (from == to) return true;
  switch (from) {
    case EngineLifecycleState::Created:
      return to == EngineLifecycleState::Configured || to == EngineLifecycleState::Disposed;
    case EngineLifecycleState::Configured:
      return to == EngineLifecycleState::Loaded || to == EngineLifecycleState::Trapped || to == EngineLifecycleState::Disposed;
    case EngineLifecycleState::Loaded:
      return to == EngineLifecycleState::Executing || to == EngineLifecycleState::Trapped || to == EngineLifecycleState::Disposed;
    case EngineLifecycleState::Executing:
      return to == EngineLifecycleState::Loaded || to == EngineLifecycleState::Trapped || to == EngineLifecycleState::Disposed;
    case EngineLifecycleState::Trapped:
      return to == EngineLifecycleState::Disposed;
    case EngineLifecycleState::Disposed:
      return false;
  }
  return false;
}

const std::vector<HostCallEntry>& host_call_entries() {
  static const std::vector<HostCallEntry> entries = {
      {static_cast<std::uint32_t>(HostCallId::ConsoleLog), "console.log", "(console_event_record) -> void", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ConsoleWarn), "console.warn", "(console_event_record) -> void", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ConsoleError), "console.error", "(console_event_record) -> void", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ModuleResolve), "module.resolve", "(specifier, referrer) -> module_record", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ModuleLoad), "module.load", "(module_record) -> source_record", "future-real-engine"},
      {static_cast<std::uint32_t>(HostCallId::FetchBlocked), "fetch.blocked", "(request_record) -> denied_response", "trap"},
      {static_cast<std::uint32_t>(HostCallId::ModuleInstantiate), "module.instantiate", "(module_record) -> namespace_record", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ModuleEvaluate), "module.evaluate", "(module_record) -> result_record", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ModuleCacheGet), "module.cache.get", "(specifier, referrer) -> namespace_record", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ModuleCachePut), "module.cache.put", "(specifier, namespace_record) -> void", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionPrepare), "execution.prepare", "(script_record) -> prepared_record", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionEvaluate), "execution.evaluate", "(prepared_record) -> eval_result", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionResult), "execution.result", "(eval_result) -> result_record", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ConsoleCaptureFlush), "console.capture.flush", "(console_capture) -> void", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionCheckpoint), "execution.checkpoint", "(checkpoint_record) -> checkpoint_id", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionReplay), "execution.replay", "(replay_cursor) -> replay_status", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionResume), "execution.resume", "(resume_state) -> execution_state", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionAudit), "execution.audit", "(determinism_audit) -> audit_status", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::SnapshotCapture), "snapshot.capture", "(snapshot_record) -> snapshot_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::SnapshotValidate), "snapshot.validate", "(snapshot_hash) -> validation_status", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::DeterminismLedgerAppend), "determinism.ledger.append", "(ledger_entry) -> ledger_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::AuditSeal), "audit.seal", "(audit_record) -> seal_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionCommit), "execution.commit", "(commit_record) -> commit_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ExecutionRollback), "execution.rollback", "(rollback_marker) -> checkpoint_id", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::HostReceipt), "host.receipt", "(host_call_receipt) -> receipt_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ReleaseAccept), "release.accept", "(acceptance_record) -> acceptance_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::CapabilityCheck), "capability.check", "(capability_record) -> allow|deny", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::HostIoRequest), "host.io.request", "(io_record) -> policy_decision", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::PermissionSeal), "permission.seal", "(permission_record) -> seal_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::ReleaseGate), "release.gate", "(gate_record) -> release_decision", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::HostIoDecision), "host.io.decision", "(io_decision_record) -> decision_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::HostIoDeny), "host.io.deny", "(deny_trace_record) -> deny_hash", "trap"},
      {static_cast<std::uint32_t>(HostCallId::CapabilityLedgerAppend), "capability.ledger.append", "(capability_ledger_entry) -> ledger_hash", "runtime"},
      {static_cast<std::uint32_t>(HostCallId::PolicySealAudit), "policy.seal.audit", "(policy_seal_audit) -> audit_hash", "runtime"},
  };
  return entries;
}

bool is_known_host_call_id(std::uint32_t id) noexcept {
  for (const auto& entry : host_call_entries()) {
    if (entry.id == id) return true;
  }
  return false;
}

std::string host_call_dispatch_table_text(bool strict_release) {
  std::ostringstream out;
  const auto& entries = host_call_entries();
  out << "VENOM_QJS_HOST_CALL_DISPATCH_V3\n"
      << "version=1\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "unknown_host_call=" << (strict_release ? "trap" : "record-and-fallback") << "\n"
      << "entry_count=" << entries.size() << "\n";
  for (const auto& entry : entries) {
    out << "host_call\t" << entry.id << "\t" << entry.name << "\t" << entry.signature << "\t" << entry.mode << "\n";
  }
  return out.str();
}

std::uint64_t host_call_dispatch_table_hash(bool strict_release) {
  const auto text = host_call_dispatch_table_text(strict_release);
  return venom::package::fnv1a64(std::vector<unsigned char>(text.begin(), text.end()));
}

std::string execution_lifecycle_text(bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_EXECUTION_LIFECYCLE_V1\n"
      << "version=1\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "states=created|configured|loaded|executing|trapped|disposed\n"
      << "initial=created\n"
      << "terminal=disposed\n"
      << "trap_state=trapped\n"
      << "transition\tcreated\tconfigured\n"
      << "transition\tconfigured\tloaded\n"
      << "transition\tloaded\texecuting\n"
      << "transition\texecuting\tloaded\n"
      << "transition\tcreated|configured|loaded|executing\ttrapped\n"
      << "transition\tcreated|configured|loaded|executing|trapped\tdisposed\n"
      << "deny_execute_when=created|configured|trapped|disposed\n"
      << "release_fail_closed_on_invalid_transition=true\n";
  return out.str();
}

std::string script_buffer_policy_text(std::uint32_t max_script_bytes, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_SCRIPT_BUFFER_POLICY_V1\n"
      << "version=1\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "reject_zero_size=true\n"
      << "reject_oversized=true\n"
      << "validate_hash_before_execute=true\n"
      << "track_alloc_counter=true\n"
      << "track_free_counter=true\n"
      << "max_script_bytes=" << max_script_bytes << "\n"
      << "allocation_state=none|allocated|executing|freed\n";
  return out.str();
}

std::string context_limit_policy_text(std::uint32_t heap_limit,
                                      std::uint32_t stack_limit,
                                      std::uint32_t max_contexts,
                                      std::uint32_t max_script_bytes,
                                      std::uint32_t max_host_calls,
                                      std::uint32_t max_console_events,
                                      std::uint32_t max_module_records,
                                      bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_CONTEXT_LIMIT_POLICY_V1\n"
      << "version=1\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "max_heap_bytes=" << heap_limit << "\n"
      << "max_stack_bytes=" << stack_limit << "\n"
      << "max_script_bytes=" << max_script_bytes << "\n"
      << "max_contexts=" << max_contexts << "\n"
      << "max_host_calls=" << max_host_calls << "\n"
      << "max_console_events=" << max_console_events << "\n"
      << "max_module_records=" << max_module_records << "\n"
      << "enforce_before_execution=true\n";
  return out.str();
}

std::string parity_contract_text(std::uint32_t runtime_abi,
                                 std::uint32_t package_version,
                                 std::uint64_t abi_hash,
                                 std::uint64_t host_import_hash,
                                 std::uint64_t host_dispatch_hash,
                                 bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_PARITY_CONTRACT_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "compare=abi-version|exported-section-hashes|limit-table|trap-policy|host-import-table|host-dispatch-table\n"
      << "abi_hash=0x" << std::hex << abi_hash << std::dec << "\n"
      << "host_import_hash=0x" << std::hex << host_import_hash << std::dec << "\n"
      << "host_dispatch_hash=0x" << std::hex << host_dispatch_hash << std::dec << "\n"
      << "release_on_mismatch=fail-closed\n";
  return out.str();
}

std::string release_failclosed_policy_text(bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_RELEASE_FAILCLOSED_V1\n"
      << "version=1\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "debug_policy=warn-and-record\n"
      << "release_policy=fail-closed\n"
      << "fail_on=abi-mismatch|missing-policy-section|malformed-host-imports|heap-accounting-section-mismatch|wasm-native-parity-failure|unknown-host-call|malformed-exception-abi\n";
  return out.str();
}


std::string module_execution_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t module_count, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_MODULE_EXECUTION_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "mode=route-scoped-esm-prototype\n"
      << "module_count=" << module_count << "\n"
      << "entry_strategy=ordered-script-chunks\n"
      << "static_imports=simple-package-relative\n"
      << "dynamic_import=trap-until-real-engine\n"
      << "host_fallback=esm-transform-prototype\n"
      << "release_requires=module-graph|resolver-audit|cache-policy|interop-fallback-policy\n";
  return out.str();
}

std::string module_cache_policy_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t module_count) {
  std::ostringstream out;
  out << "VENOM_QJS_MODULE_CACHE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "module_count=" << module_count << "\n"
      << "cache_key=normalized-specifier|route|source-hash\n"
      << "namespace_model=frozen-export-record\n"
      << "instantiate_once=true\n"
      << "evaluate_once=true\n"
      << "eviction=package-lifetime\n";
  return out.str();
}

std::string resolver_audit_policy_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t module_count, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_RESOLVER_AUDIT_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "module_count=" << module_count << "\n"
      << "record=specifier|referrer|normalized|status|host-call-id\n"
      << "unknown_specifier=" << (strict_release ? "trap" : "record-and-empty-namespace") << "\n"
      << "dynamic_import=trap\n";
  return out.str();
}

std::string interop_fallback_policy_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_INTEROP_FALLBACK_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "fallback_kind=host-esm-transform-prototype\n"
      << "allowed_syntax=import-default|import-named|import-namespace|import-side-effect|export-const|export-let|export-var|export-function|export-default|export-list\n"
      << "denied_syntax=top-level-await|dynamic-import|import-assertions\n"
      << "release_behavior=" << (strict_release ? "fail-closed-on-unsupported-module-syntax" : "record-and-fallback") << "\n";
  return out.str();
}


std::string execution_pipeline_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_EXECUTION_PIPELINE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "chunk_count=" << chunk_count << "\n"
      << "stages=prepare|transfer|evaluate|capture-console|result|failure-report\n"
      << "prepare_host_call=execution.prepare\n"
      << "evaluate_host_call=execution.evaluate\n"
      << "result_host_call=execution.result\n"
      << "console_flush_host_call=console.capture.flush\n"
      << "release_requires=script-records|eval-results|console-capture|failure-reports\n";
  return out.str();
}

std::string script_records_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_SCRIPT_RECORDS_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "chunk_count=" << chunk_count << "\n"
      << "record_fields=route|source|order|flags|bytes|fnv1a32|module|prepared\n"
      << "reject_unprepared_eval=true\n";
  return out.str();
}

std::string eval_results_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_EVAL_RESULTS_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "result_fields=ok|context|source_bytes|source_hash|lines|console_count|exception_code|fallback_required\n"
      << "result_hash=fnv1a32-json-record\n";
  return out.str();
}

std::string console_capture_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_CONSOLE_CAPTURE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "levels=debug|log|info|warn|error\n"
      << "capture_schema=id|level|route|source|order|message|args\n"
      << "flush_host_call=console.capture.flush\n";
  return out.str();
}

std::string failure_reports_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_FAILURE_REPORTS_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "schema=kind|code|message|context|source_hash|trap_code|stage\n"
      << "release_behavior=fail-closed-with-report\n";
  return out.str();
}

std::string execution_journal_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_EXECUTION_JOURNAL_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "chunk_count=" << chunk_count << "\n"
      << "schema=sequence|context|stage|source_hash|result_hash|console_count|host_call_count|checkpoint_id\n"
      << "stable_order=route-order-then-source-order\n"
      << "hash=fnv1a32-journal-records\n"
      << "release_requires=checkpoint-policy|replay-cursor|resume-state|determinism-audit\n";
  return out.str();
}

std::string checkpoint_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_CHECKPOINT_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "checkpoint_stages=prepare|evaluate|console-capture|result|failure\n"
      << "capture=context|state|heap-used|source-hash|host-call-count|console-count|module-count\n"
      << "restore_policy=" << (strict_release ? "fail-closed-on-missing-checkpoint" : "record-and-restart") << "\n"
      << "host_call=execution.checkpoint\n";
  return out.str();
}

std::string replay_cursor_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_REPLAY_CURSOR_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "cursor_fields=sequence|stage|checkpoint_id|journal_hash|next_action\n"
      << "advance_host_call=execution.replay\n"
      << "on_mismatch=" << (strict_release ? "trap" : "record-and-continue") << "\n";
  return out.str();
}

std::string resume_state_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_RESUME_STATE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "state_fields=context|checkpoint_id|engine_state|source_hash|heap_used|resume_ready\n"
      << "resume_host_call=execution.resume\n"
      << "resume_requires=checkpoint-policy|journal-entry\n";
  return out.str();
}

std::string determinism_audit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_DETERMINISM_AUDIT_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "checks=abi|host-imports|stage-order|source-hash|result-hash|console-count|module-cache-size\n"
      << "audit_host_call=execution.audit\n"
      << "release_on_mismatch=fail-closed\n";
  return out.str();
}


std::string snapshot_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_SNAPSHOT_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "capture=state|heap-used|source-hash|host-call-count|console-count|module-count|checkpoint-id\n"
      << "validate=hash|stage|context|checkpoint-sequence\n"
      << "release_on_mismatch=fail-closed\n"
      << "host_call_capture=snapshot.capture\n"
      << "host_call_validate=snapshot.validate\n";
  return out.str();
}

std::string snapshot_records_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_SNAPSHOT_RECORDS_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "chunk_count=" << chunk_count << "\n"
      << "record_fields=sequence|context|stage|checkpoint_id|source_hash|heap_used|host_call_count|console_count|module_count|snapshot_hash\n"
      << "hash=fnv1a32-snapshot-record\n";
  return out.str();
}

std::string replay_validation_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_REPLAY_VALIDATION_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "checks=snapshot-hash|journal-hash|host-call-order|checkpoint-id|stage-order\n"
      << "on_mismatch=" << (strict_release ? "trap" : "record-and-continue") << "\n";
  return out.str();
}

std::string determinism_ledger_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_DETERMINISM_LEDGER_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "ledger_fields=sequence|snapshot_hash|journal_hash|result_hash|audit_hash|previous_hash\n"
      << "append_host_call=determinism.ledger.append\n"
      << "chain_hash=fnv1a32-linked-records\n";
  return out.str();
}

std::string audit_seal_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_AUDIT_SEAL_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "seal_fields=abi_hash|host_import_hash|snapshot_hash|ledger_hash|trap_code\n"
      << "seal_host_call=audit.seal\n"
      << "release_requires=audit-seal-present-and-matched\n";
  return out.str();
}

std::string execution_commit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_EXECUTION_COMMIT_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "commit_fields=context|checkpoint_id|snapshot_hash|ledger_hash|result_hash|host_receipt_hash|commit_hash\n"
      << "commit_host_call=execution.commit\n"
      << "release_requires=commit-record-present-and-linked\n";
  return out.str();
}

std::string rollback_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_ROLLBACK_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "rollback_fields=context|from_commit|checkpoint_id|reason|restored_state\n"
      << "rollback_host_call=execution.rollback\n"
      << "release_behavior=" << (strict_release ? "fail-closed-on-unmatched-rollback" : "record-and-replay-from-checkpoint") << "\n";
  return out.str();
}

std::string host_call_receipts_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_HOST_CALL_RECEIPTS_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "receipt_fields=sequence|host_call_id|payload_hash|accepted|receipt_hash\n"
      << "receipt_host_call=host.receipt\n"
      << "release_requires=all-host-calls-receipted\n";
  return out.str();
}

std::string release_acceptance_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_RELEASE_ACCEPTANCE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "acceptance_checks=abi-match|host-import-match|snapshot-valid|audit-seal-valid|commit-linked|receipts-complete\n"
      << "accept_host_call=release.accept\n"
      << "release_on_failure=trap\n";
  return out.str();
}

std::string commit_audit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_COMMIT_AUDIT_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "audit_fields=commit_hash|receipt_hash|acceptance_hash|rollback_hash|trap_code\n"
      << "release_requires=commit-audit-present\n";
  return out.str();
}

std::string capability_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_CAPABILITY_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "capabilities=console.log|module.resolve|module.load|module.cache|execution.audit|snapshot.capture|permission.seal\n"
      << "default=deny\n"
      << "host_call=capability.check\n"
      << "release_requires=permission-seal|policy-receipts|release-gate\n";
  return out.str();
}

std::string host_io_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_HOST_IO_POLICY_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "io_classes=console|module|fetch|timer|event|storage\n"
      << "fetch=deny-by-default\n"
      << "timer=host-policy-required\n"
      << "event=host-policy-required\n"
      << "host_call=host.io.request\n"
      << "release_behavior=fail-closed-on-unsealed-io\n";
  return out.str();
}

std::string permission_seal_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_PERMISSION_SEAL_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "seal_fields=abi_hash|host_import_hash|capability_policy_hash|host_io_policy_hash|receipt_hash\n"
      << "seal_host_call=permission.seal\n"
      << "release_requires=seal-present-and-matched\n";
  return out.str();
}

std::string policy_receipts_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_POLICY_RECEIPTS_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "receipt_fields=sequence|capability|io_class|decision|payload_hash|receipt_hash\n"
      << "receipt_host_call=host.receipt\n"
      << "release_requires=all-policy-decisions-receipted\n";
  return out.str();
}

std::string release_gate_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_RELEASE_GATE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "gate_checks=abi-match|host-import-match|capability-policy-match|io-policy-match|permission-seal-valid|receipts-complete\n"
      << "gate_host_call=release.gate\n"
      << "release_on_failure=trap\n";
  return out.str();
}

std::string host_io_decision_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_HOST_IO_DECISION_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "decision_fields=sequence|context|io_class|capability|decision|reason|payload_hash|policy_hash|receipt_hash\n"
      << "decision_host_call=host.io.decision\n"
      << "release_requires=all-host-io-decisions-linked-to-policy-receipts\n";
  return out.str();
}

std::string host_io_deny_trace_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_HOST_IO_DENY_TRACE_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "trace_fields=sequence|context|io_class|capability|reason|trap_code|source_hash|payload_hash\n"
      << "deny_host_call=host.io.deny\n"
      << "release_behavior=" << (strict_release ? "fail-closed-with-deny-trace" : "record-deny-trace-and-fallback") << "\n";
  return out.str();
}

std::string capability_ledger_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_CAPABILITY_LEDGER_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "ledger_fields=sequence|capability|decision_hash|receipt_hash|previous_hash|entry_hash\n"
      << "append_host_call=capability.ledger.append\n"
      << "chain_hash=fnv1a32-linked-capability-decisions\n";
  return out.str();
}

std::string policy_seal_audit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_POLICY_SEAL_AUDIT_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "audit_fields=capability_policy_hash|host_io_policy_hash|decision_hash|deny_trace_hash|ledger_hash|permission_seal_hash\n"
      << "audit_host_call=policy.seal.audit\n"
      << "release_requires=policy-seal-audit-present-and-matched\n";
  return out.str();
}

std::string runtime_denylist_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release) {
  std::ostringstream out;
  out << "VENOM_QJS_RUNTIME_DENYLIST_V1\n"
      << "version=1\n"
      << "runtime_abi=" << runtime_abi << "\n"
      << "package_version=" << package_version << "\n"
      << "strict_release=" << (strict_release ? "true" : "false") << "\n"
      << "denylist=fetch|remote-module|dynamic-import|eval-indirect|storage-write|network-raw\n"
      << "deny_result=host.io.deny-trace\n"
      << "release_behavior=fail-closed-on-denylist-hit\n";
  return out.str();
}


} // namespace venom::quickjs
