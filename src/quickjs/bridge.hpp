#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace venom::quickjs {

enum class EngineLifecycleState : std::uint32_t {
  Created = 1,
  Configured = 2,
  Loaded = 3,
  Executing = 4,
  Trapped = 5,
  Disposed = 6,
};

const char* lifecycle_state_name(EngineLifecycleState state) noexcept;
bool lifecycle_transition_allowed(EngineLifecycleState from, EngineLifecycleState to) noexcept;

struct HostCallEntry {
  std::uint32_t id;
  const char* name;
  const char* signature;
  const char* mode;
};

enum class HostCallId : std::uint32_t {
  ConsoleLog = 1,
  ConsoleWarn = 2,
  ConsoleError = 3,
  ModuleResolve = 4,
  ModuleLoad = 5,
  FetchBlocked = 6,
  ModuleInstantiate = 7,
  ModuleEvaluate = 8,
  ModuleCacheGet = 9,
  ModuleCachePut = 10,
  ExecutionPrepare = 11,
  ExecutionEvaluate = 12,
  ExecutionResult = 13,
  ConsoleCaptureFlush = 14,
  ExecutionCheckpoint = 15,
  ExecutionReplay = 16,
  ExecutionResume = 17,
  ExecutionAudit = 18,
  SnapshotCapture = 19,
  SnapshotValidate = 20,
  DeterminismLedgerAppend = 21,
  AuditSeal = 22,
  ExecutionCommit = 23,
  ExecutionRollback = 24,
  HostReceipt = 25,
  ReleaseAccept = 26,
  CapabilityCheck = 27,
  HostIoRequest = 28,
  PermissionSeal = 29,
  ReleaseGate = 30,
  HostIoDecision = 31,
  HostIoDeny = 32,
  CapabilityLedgerAppend = 33,
  PolicySealAudit = 34,
};

const std::vector<HostCallEntry>& host_call_entries();
bool is_known_host_call_id(std::uint32_t id) noexcept;
std::string host_call_dispatch_table_text(bool strict_release);
std::uint64_t host_call_dispatch_table_hash(bool strict_release);
std::string execution_lifecycle_text(bool strict_release);
std::string script_buffer_policy_text(std::uint32_t max_script_bytes, bool strict_release);
std::string context_limit_policy_text(std::uint32_t heap_limit,
                                      std::uint32_t stack_limit,
                                      std::uint32_t max_contexts,
                                      std::uint32_t max_script_bytes,
                                      std::uint32_t max_host_calls,
                                      std::uint32_t max_console_events,
                                      std::uint32_t max_module_records,
                                      bool strict_release);
std::string parity_contract_text(std::uint32_t runtime_abi,
                                 std::uint32_t package_version,
                                 std::uint64_t abi_hash,
                                 std::uint64_t host_import_hash,
                                 std::uint64_t host_dispatch_hash,
                                 bool strict_release);
std::string release_failclosed_policy_text(bool strict_release);
std::string module_execution_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t module_count, bool strict_release);
std::string module_cache_policy_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t module_count);
std::string resolver_audit_policy_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t module_count, bool strict_release);
std::string interop_fallback_policy_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string execution_pipeline_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release);
std::string script_records_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release);
std::string eval_results_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string console_capture_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string failure_reports_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string execution_journal_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release);
std::string checkpoint_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string replay_cursor_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string resume_state_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string determinism_audit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string snapshot_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string snapshot_records_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, std::size_t chunk_count, bool strict_release);
std::string replay_validation_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string determinism_ledger_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string audit_seal_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string execution_commit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string rollback_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string host_call_receipts_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string release_acceptance_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string commit_audit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string capability_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string host_io_policy_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string permission_seal_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string policy_receipts_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string release_gate_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string host_io_decision_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string host_io_deny_trace_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string capability_ledger_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string policy_seal_audit_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);
std::string runtime_denylist_contract_text(std::uint32_t runtime_abi, std::uint32_t package_version, bool strict_release);

// Backward-compatible no-op retained for older internal callers.
void install_host_bridge_placeholder();

} // namespace venom::quickjs
