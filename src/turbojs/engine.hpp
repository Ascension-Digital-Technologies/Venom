#pragma once

#include "turbojs/abi.hpp"
#include "turbojs/bridge.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct JSRuntime;
struct JSContext;

namespace venom::turbojs {

struct EngineLimits {
  std::size_t heap_limit_bytes = kDefaultHeapLimitBytes;
  std::size_t stack_limit_bytes = kDefaultStackLimitBytes;
  std::size_t script_buffer_limit_bytes = kDefaultScriptBufferLimitBytes;
  std::size_t max_host_calls = kDefaultHostCallLimit;
  std::size_t max_console_events = kDefaultConsoleEventLimit;
  std::size_t max_module_records = kDefaultModuleRecordLimit;
};

struct ConsoleEvent {
  std::string level;
  std::string message;
  std::string source;
};

struct ExceptionRecord {
  std::uint32_t code = 0;
  std::string kind;
  std::string message;
};

struct ExecutionCheckpoint {
  std::uint32_t id = 0;
  EngineLifecycleState state = EngineLifecycleState::Created;
  std::size_t accounted_script_bytes = 0;
  std::uint64_t host_call_count = 0;
  std::uint64_t console_event_count = 0;
  std::uint64_t module_record_count = 0;
};

struct SnapshotRecord {
  std::uint32_t id = 0;
  std::uint32_t stage_id = 0;
  ExecutionCheckpoint checkpoint;
  std::uint64_t snapshot_hash = 0;
  std::uint64_t journal_hash = 0;
  bool valid = false;
};

struct PolicyReceiptRecord {
  std::uint32_t id = 0;
  std::uint32_t capability_id = 0;
  std::string capability;
  std::string decision;
  std::uint64_t payload_hash = 0;
  std::uint64_t receipt_hash = 0;
};

struct HostIoDecisionRecord {
  std::uint32_t id = 0;
  std::uint32_t io_class = 0;
  std::uint32_t capability_id = 0;
  std::string capability;
  std::string decision;
  std::string reason;
  std::uint64_t payload_hash = 0;
  std::uint64_t decision_hash = 0;
};

struct ExecutionCommitRecord {
  std::uint32_t id = 0;
  std::uint32_t checkpoint_id = 0;
  std::uint64_t snapshot_hash = 0;
  std::uint64_t ledger_hash = 0;
  std::uint64_t result_hash = 0;
  std::uint64_t host_receipt_hash = 0;
  std::uint64_t commit_hash = 0;
  bool accepted = false;
};

using ConsoleCallback = std::function<void(const ConsoleEvent&)>;

class Engine {
public:
  explicit Engine(EngineLimits limits = {});
  ~Engine();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  void set_limits(EngineLimits limits);
  const EngineLimits& limits() const noexcept { return limits_; }
  EngineLifecycleState state() const noexcept { return state_; }
  std::size_t accounted_script_bytes() const noexcept { return accounted_script_bytes_; }
  std::uint64_t script_buffer_alloc_count() const noexcept { return script_buffer_alloc_count_; }
  std::uint64_t script_buffer_free_count() const noexcept { return script_buffer_free_count_; }
  const ExceptionRecord& last_exception() const noexcept { return last_exception_; }
  ExecutionCheckpoint create_checkpoint();
  bool restore_checkpoint(std::uint32_t checkpoint_id);
  std::string execution_journal_text() const;
  std::string resume_state_text() const;
  std::string determinism_audit_text() const;
  std::uint64_t capture_snapshot(std::uint32_t stage_id = 0);
  bool validate_snapshot(std::uint64_t expected_hash);
  std::string snapshot_records_text() const;
  std::string replay_validation_text() const;
  std::string determinism_ledger_text() const;
  std::string audit_seal_text() const;
  std::uint64_t commit_execution(std::uint32_t checkpoint_id = 0);
  bool rollback_to_checkpoint(std::uint32_t checkpoint_id);
  bool release_acceptance_check() const;
  std::string execution_commits_text() const;
  std::string rollback_policy_text() const;
  std::string host_call_receipts_text() const;
  std::string release_acceptance_text() const;
  std::string commit_audit_text() const;
  bool check_capability(std::uint32_t capability_id);
  bool request_host_io(std::uint32_t io_class, std::uint32_t capability_id, std::uint64_t payload_hash = 0);
  std::uint64_t seal_permissions();
  bool release_gate_check() const;
  std::string capability_policy_text() const;
  std::string host_io_policy_text() const;
  std::string permission_seal_text() const;
  std::string policy_receipts_text() const;
  std::string release_gate_text() const;
  std::string host_io_decisions_text() const;
  std::string host_io_deny_traces_text() const;
  std::string capability_ledger_text() const;
  std::string policy_seal_audit_text() const;
  std::string runtime_denylist_text() const;

  void set_console_callback(ConsoleCallback callback);

  std::string eval_string(const std::string& source, const std::string& filename = "<eval>");
  std::string eval_byte_buffer(const unsigned char* bytes, std::size_t size, const std::string& filename = "<buffer>");
  std::string eval_byte_buffer_checked(const unsigned char* bytes, std::size_t size, std::uint64_t expected_hash, const std::string& filename = "<buffer>");

  ParityProbe parity_probe() const;

private:
  void transition(EngineLifecycleState next);
  void trap(std::uint32_t code, std::string kind, std::string message);
  void clear_exception();
  void validate_script_bytes(const unsigned char* bytes, std::size_t size, std::uint64_t expected_hash);
  void account_script_bytes(std::size_t size);
  void release_script_bytes(std::size_t size) noexcept;
  void record_host_call(HostCallId id);
  void record_console_event(const std::string& filename);
  void record_policy_receipt(std::uint32_t capability_id, const std::string& capability, const std::string& decision, std::uint64_t payload_hash);
  void record_host_io_decision(std::uint32_t io_class, std::uint32_t capability_id, const std::string& capability, const std::string& decision, const std::string& reason, std::uint64_t payload_hash);

  EngineLimits limits_;
  EngineLifecycleState state_ = EngineLifecycleState::Created;
  std::size_t accounted_script_bytes_ = 0;
  std::uint64_t script_buffer_alloc_count_ = 0;
  std::uint64_t script_buffer_free_count_ = 0;
  std::uint64_t host_call_count_ = 0;
  std::uint64_t console_event_count_ = 0;
  std::uint64_t module_record_count_ = 0;
  ExceptionRecord last_exception_;
  std::vector<ExecutionCheckpoint> checkpoints_;
  std::vector<SnapshotRecord> snapshots_;
  std::vector<ExecutionCommitRecord> commits_;
  std::vector<PolicyReceiptRecord> policy_receipts_;
  std::vector<HostIoDecisionRecord> host_io_decisions_;
  std::vector<HostIoDecisionRecord> host_io_denials_;
  std::uint32_t next_checkpoint_id_ = 1;
  std::uint32_t next_snapshot_id_ = 1;
  std::uint32_t next_commit_id_ = 1;
  std::uint32_t next_policy_receipt_id_ = 1;
  std::uint32_t next_host_io_decision_id_ = 1;
  std::uint64_t permission_seal_hash_ = 0;
  ConsoleCallback console_callback_;
  JSRuntime* runtime_ = nullptr;
  JSContext* context_ = nullptr;
};

} // namespace venom::turbojs
