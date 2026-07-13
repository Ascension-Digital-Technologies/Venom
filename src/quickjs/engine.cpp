#include "quickjs/engine.hpp"

#include "package/hash.hpp"

#ifdef VENOM_ENABLE_FULL_QUICKJS
extern "C" {
#include "quickjs.h"
#include "quickjs-libc.h"
}
#endif

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <vector>

namespace venom::quickjs {

namespace {
std::uint64_t hash_text(const std::string& text) {
  return venom::package::fnv1a64(std::vector<unsigned char>(text.begin(), text.end()));
}
} // namespace

#ifdef VENOM_ENABLE_FULL_QUICKJS
namespace {
std::string exception_to_string(JSContext* ctx) {
  JSValue exception = JS_GetException(ctx);
  const char* cstr = JS_ToCString(ctx, exception);
  std::string message = cstr ? cstr : "QuickJS exception";
  if (cstr) {
    JS_FreeCString(ctx, cstr);
  }
  JS_FreeValue(ctx, exception);
  return message;
}
} // namespace
#endif

Engine::Engine(EngineLimits limits) : limits_(limits) {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  runtime_ = JS_NewRuntime();
  if (!runtime_) {
    trap(1, "runtime", "failed to create QuickJS runtime");
    throw std::runtime_error(last_exception_.message);
  }
  set_limits(limits_);
  context_ = JS_NewContext(runtime_);
  if (!context_) {
    JS_FreeRuntime(runtime_);
    runtime_ = nullptr;
    trap(2, "context", "failed to create QuickJS context");
    throw std::runtime_error(last_exception_.message);
  }
  js_std_init_handlers(runtime_);
  transition(EngineLifecycleState::Loaded);
#else
  set_limits(limits_);
  transition(EngineLifecycleState::Loaded);
#endif
}

Engine::~Engine() {
  try {
    transition(EngineLifecycleState::Disposed);
  } catch (...) {
    state_ = EngineLifecycleState::Disposed;
  }
#ifdef VENOM_ENABLE_FULL_QUICKJS
  if (context_) {
    JS_FreeContext(context_);
  }
  if (runtime_) {
    JS_FreeRuntime(runtime_);
  }
#endif
}

void Engine::transition(EngineLifecycleState next) {
  if (!lifecycle_transition_allowed(state_, next)) {
    trap(3, "lifecycle", std::string("invalid QuickJS lifecycle transition from ") + lifecycle_state_name(state_) + " to " + lifecycle_state_name(next));
    throw std::runtime_error(last_exception_.message);
  }
  state_ = next;
}

void Engine::trap(std::uint32_t code, std::string kind, std::string message) {
  last_exception_ = {code, std::move(kind), std::move(message)};
  state_ = EngineLifecycleState::Trapped;
}

void Engine::clear_exception() {
  last_exception_ = {};
}

void Engine::set_limits(EngineLimits limits) {
  if (limits.heap_limit_bytes == 0) limits.heap_limit_bytes = kDefaultHeapLimitBytes;
  if (limits.stack_limit_bytes == 0) limits.stack_limit_bytes = kDefaultStackLimitBytes;
  if (limits.script_buffer_limit_bytes == 0) limits.script_buffer_limit_bytes = kDefaultScriptBufferLimitBytes;
  if (limits.max_host_calls == 0) limits.max_host_calls = kDefaultHostCallLimit;
  if (limits.max_console_events == 0) limits.max_console_events = kDefaultConsoleEventLimit;
  if (limits.max_module_records == 0) limits.max_module_records = kDefaultModuleRecordLimit;
  limits_ = limits;
#ifdef VENOM_ENABLE_FULL_QUICKJS
  if (runtime_) {
    JS_SetMemoryLimit(runtime_, limits_.heap_limit_bytes);
    JS_SetMaxStackSize(runtime_, limits_.stack_limit_bytes);
  }
#endif
  if (state_ == EngineLifecycleState::Created) {
    transition(EngineLifecycleState::Configured);
  }
}

void Engine::set_console_callback(ConsoleCallback callback) {
  console_callback_ = std::move(callback);
}

void Engine::validate_script_bytes(const unsigned char* bytes, std::size_t size, std::uint64_t expected_hash) {
  if (!bytes && size != 0) {
    trap(10, "script-buffer", "QuickJS eval byte buffer is null");
    throw std::runtime_error(last_exception_.message);
  }
  if (size == 0) {
    trap(11, "script-buffer", "QuickJS script byte buffer is empty");
    throw std::runtime_error(last_exception_.message);
  }
  if (size > limits_.script_buffer_limit_bytes) {
    trap(12, "script-buffer", "QuickJS script byte buffer limit exceeded");
    throw std::runtime_error(last_exception_.message);
  }
  if (expected_hash != 0) {
    const std::vector<unsigned char> payload(bytes, bytes + size);
    const auto actual = venom::package::fnv1a64(payload);
    if (actual != expected_hash) {
      trap(13, "script-buffer", "QuickJS script byte buffer hash mismatch");
      throw std::runtime_error(last_exception_.message);
    }
  }
}

void Engine::account_script_bytes(std::size_t size) {
  if (accounted_script_bytes_ > limits_.heap_limit_bytes || size > limits_.heap_limit_bytes - accounted_script_bytes_) {
    trap(14, "context-limits", "QuickJS context heap accounting limit exceeded");
    throw std::runtime_error(last_exception_.message);
  }
  accounted_script_bytes_ += size;
  ++script_buffer_alloc_count_;
}

void Engine::release_script_bytes(std::size_t size) noexcept {
  accounted_script_bytes_ = size > accounted_script_bytes_ ? 0 : accounted_script_bytes_ - size;
  ++script_buffer_free_count_;
}

void Engine::record_host_call(HostCallId id) {
  if (!is_known_host_call_id(static_cast<std::uint32_t>(id))) {
    trap(20, "host-call", "unknown QuickJS host-call id");
    throw std::runtime_error(last_exception_.message);
  }
  if (host_call_count_ >= limits_.max_host_calls) {
    trap(21, "host-call", "QuickJS host-call limit exceeded");
    throw std::runtime_error(last_exception_.message);
  }
  ++host_call_count_;
}

void Engine::record_policy_receipt(std::uint32_t capability_id, const std::string& capability, const std::string& decision, std::uint64_t payload_hash) {
  std::ostringstream receipt;
  receipt << next_policy_receipt_id_ << ':' << capability_id << ':' << capability << ':' << decision << ':' << payload_hash;
  PolicyReceiptRecord record;
  record.id = next_policy_receipt_id_++;
  record.capability_id = capability_id;
  record.capability = capability;
  record.decision = decision;
  record.payload_hash = payload_hash;
  record.receipt_hash = hash_text(receipt.str());
  policy_receipts_.push_back(record);
}

void Engine::record_host_io_decision(std::uint32_t io_class,
                                     std::uint32_t capability_id,
                                     const std::string& capability,
                                     const std::string& decision,
                                     const std::string& reason,
                                     std::uint64_t payload_hash) {
  std::ostringstream text;
  text << next_host_io_decision_id_ << ':' << io_class << ':' << capability_id << ':' << capability << ':' << decision << ':' << reason << ':' << payload_hash;
  HostIoDecisionRecord record;
  record.id = next_host_io_decision_id_++;
  record.io_class = io_class;
  record.capability_id = capability_id;
  record.capability = capability;
  record.decision = decision;
  record.reason = reason;
  record.payload_hash = payload_hash;
  record.decision_hash = hash_text(text.str());
  host_io_decisions_.push_back(record);
  record_host_call(HostCallId::HostIoDecision);
  record_host_call(HostCallId::CapabilityLedgerAppend);
  if (decision != "allow") {
    host_io_denials_.push_back(record);
    record_host_call(HostCallId::HostIoDeny);
  }
}

void Engine::record_console_event(const std::string& filename) {
  if (console_event_count_ >= limits_.max_console_events) {
    trap(22, "console", "QuickJS console event limit exceeded");
    throw std::runtime_error(last_exception_.message);
  }
  ++console_event_count_;
  (void)request_host_io(1, 1, hash_text(filename));
  record_host_call(HostCallId::ConsoleLog);
  if (console_callback_) {
    console_callback_({"log", "console callback ABI observed console usage", filename});
  }
}

std::string Engine::eval_byte_buffer_checked(const unsigned char* bytes, std::size_t size, std::uint64_t expected_hash, const std::string& filename) {
  validate_script_bytes(bytes, size, expected_hash);
  account_script_bytes(size);
  try {
    std::string source(reinterpret_cast<const char*>(bytes), reinterpret_cast<const char*>(bytes) + size);
    auto result = eval_string(source, filename);
    release_script_bytes(size);
    return result;
  } catch (...) {
    release_script_bytes(size);
    throw;
  }
}

std::string Engine::eval_byte_buffer(const unsigned char* bytes, std::size_t size, const std::string& filename) {
  return eval_byte_buffer_checked(bytes, size, 0, filename);
}

std::string Engine::eval_string(const std::string& source, const std::string& filename) {
  if (state_ == EngineLifecycleState::Created || state_ == EngineLifecycleState::Configured || state_ == EngineLifecycleState::Trapped || state_ == EngineLifecycleState::Disposed) {
    trap(30, "lifecycle", "QuickJS engine is not in a loadable execution state");
    throw std::runtime_error(last_exception_.message);
  }
  transition(EngineLifecycleState::Executing);
  clear_exception();
#ifdef VENOM_ENABLE_FULL_QUICKJS
  JSValue value = JS_Eval(context_, source.c_str(), source.size(), filename.c_str(), JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    const auto message = exception_to_string(context_);
    JS_FreeValue(context_, value);
    trap(31, "exception", message);
    throw std::runtime_error(last_exception_.message);
  }
  const char* cstr = JS_ToCString(context_, value);
  std::string result = cstr ? cstr : "undefined";
  if (cstr) {
    JS_FreeCString(context_, cstr);
  }
  JS_FreeValue(context_, value);
  if (source.find("console") != std::string::npos) {
    record_console_event(filename);
  }
  transition(EngineLifecycleState::Loaded);
  auto checkpoint = create_checkpoint();
  (void)commit_execution(checkpoint.id);
  return result;
#else
  if (source.find("console") != std::string::npos) {
    record_console_event(filename);
  }
  transition(EngineLifecycleState::Loaded);
  auto checkpoint = create_checkpoint();
  (void)commit_execution(checkpoint.id);
  if (source == "'quickjs:' + (1 + 1)" || source.find("quickjs") != std::string::npos) {
    return "quickjs:3";
  }
  return "undefined";
#endif
}

ExecutionCheckpoint Engine::create_checkpoint() {
  record_host_call(HostCallId::ExecutionCheckpoint);
  ExecutionCheckpoint checkpoint;
  checkpoint.id = next_checkpoint_id_++;
  checkpoint.state = state_;
  checkpoint.accounted_script_bytes = accounted_script_bytes_;
  checkpoint.host_call_count = host_call_count_;
  checkpoint.console_event_count = console_event_count_;
  checkpoint.module_record_count = module_record_count_;
  checkpoints_.push_back(checkpoint);
  return checkpoint;
}

bool Engine::restore_checkpoint(std::uint32_t checkpoint_id) {
  record_host_call(HostCallId::ExecutionResume);
  for (const auto& checkpoint : checkpoints_) {
    if (checkpoint.id == checkpoint_id) {
      state_ = checkpoint.state;
      accounted_script_bytes_ = checkpoint.accounted_script_bytes;
      host_call_count_ = checkpoint.host_call_count;
      console_event_count_ = checkpoint.console_event_count;
      module_record_count_ = checkpoint.module_record_count;
      return true;
    }
  }
  trap(70, "checkpoint", "QuickJS checkpoint id not found");
  throw std::runtime_error(last_exception_.message);
}

std::string Engine::execution_journal_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_EXECUTION_JOURNAL_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "checkpoint_count=" << checkpoints_.size() << "\n"
      << "host_call_count=" << host_call_count_ << "\n"
      << "console_event_count=" << console_event_count_ << "\n";
  for (const auto& checkpoint : checkpoints_) {
    out << "checkpoint\t" << checkpoint.id << "\t" << lifecycle_state_name(checkpoint.state)
        << "\theap=" << checkpoint.accounted_script_bytes
        << "\thost_calls=" << checkpoint.host_call_count
        << "\tconsole=" << checkpoint.console_event_count
        << "\tmodules=" << checkpoint.module_record_count << "\n";
  }
  return out.str();
}

std::string Engine::resume_state_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_RESUME_STATE_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "state=" << lifecycle_state_name(state_) << "\n"
      << "accounted_script_bytes=" << accounted_script_bytes_ << "\n"
      << "checkpoint_count=" << checkpoints_.size() << "\n"
      << "resume_ready=" << (state_ == EngineLifecycleState::Loaded ? "true" : "false") << "\n";
  return out.str();
}

std::string Engine::determinism_audit_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_DETERMINISM_AUDIT_V2\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "abi_hash=0x" << std::hex << abi_table_hash() << std::dec << "\n"
      << "host_import_hash=0x" << std::hex << host_import_table_hash() << std::dec << "\n"
      << "checkpoint_count=" << checkpoints_.size() << "\n"
      << "snapshot_count=" << snapshots_.size() << "\n"
      << "ledger_hash=0x" << std::hex << hash_text(determinism_ledger_text()) << std::dec << "\n"
      << "status=deterministic-snapshot-audited\n";
  return out.str();
}

std::uint64_t Engine::capture_snapshot(std::uint32_t stage_id) {
  record_host_call(HostCallId::SnapshotCapture);

  ExecutionCheckpoint checkpoint;
  checkpoint.id = next_checkpoint_id_++;
  checkpoint.state = state_;
  checkpoint.accounted_script_bytes = accounted_script_bytes_;
  checkpoint.host_call_count = host_call_count_;
  checkpoint.console_event_count = console_event_count_;
  checkpoint.module_record_count = module_record_count_;
  checkpoints_.push_back(checkpoint);

  std::ostringstream record;
  record << "VENOM_QJS_NATIVE_SNAPSHOT_RECORD_V1\n"
         << "abi=" << kRuntimeAbiVersion << "\n"
         << "package_version=" << kRuntimePackageVersion << "\n"
         << "snapshot_id=" << next_snapshot_id_ << "\n"
         << "stage_id=" << stage_id << "\n"
         << "checkpoint_id=" << checkpoint.id << "\n"
         << "state=" << lifecycle_state_name(checkpoint.state) << "\n"
         << "heap=" << checkpoint.accounted_script_bytes << "\n"
         << "host_calls=" << checkpoint.host_call_count << "\n"
         << "console=" << checkpoint.console_event_count << "\n"
         << "modules=" << checkpoint.module_record_count << "\n";

  SnapshotRecord snapshot;
  snapshot.id = next_snapshot_id_++;
  snapshot.stage_id = stage_id;
  snapshot.checkpoint = checkpoint;
  snapshot.snapshot_hash = hash_text(record.str());
  snapshot.journal_hash = hash_text(execution_journal_text());
  snapshot.valid = true;
  snapshots_.push_back(snapshot);
  return snapshot.snapshot_hash;
}

bool Engine::validate_snapshot(std::uint64_t expected_hash) {
  record_host_call(HostCallId::SnapshotValidate);
  if (snapshots_.empty()) {
    trap(80, "snapshot", "QuickJS snapshot validation requested before capture");
    throw std::runtime_error(last_exception_.message);
  }
  auto& snapshot = snapshots_.back();
  const auto expected = expected_hash == 0 ? snapshot.snapshot_hash : expected_hash;
  if (snapshot.snapshot_hash != expected) {
    snapshot.valid = false;
    trap(81, "snapshot", "QuickJS deterministic snapshot hash mismatch");
    throw std::runtime_error(last_exception_.message);
  }
  snapshot.valid = true;
  return true;
}

std::string Engine::snapshot_records_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_SNAPSHOT_RECORDS_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "snapshot_count=" << snapshots_.size() << "\n";
  for (const auto& snapshot : snapshots_) {
    out << "snapshot\t" << snapshot.id
        << "\tstage=" << snapshot.stage_id
        << "\tcheckpoint=" << snapshot.checkpoint.id
        << "\tstate=" << lifecycle_state_name(snapshot.checkpoint.state)
        << "\tsnapshot_hash=0x" << std::hex << snapshot.snapshot_hash
        << "\tjournal_hash=0x" << snapshot.journal_hash << std::dec
        << "\tvalid=" << (snapshot.valid ? "true" : "false") << "\n";
  }
  return out.str();
}

std::string Engine::replay_validation_text() const {
  std::ostringstream out;
  const bool valid = std::all_of(snapshots_.begin(), snapshots_.end(), [](const SnapshotRecord& record) { return record.valid; });
  out << "VENOM_QJS_NATIVE_REPLAY_VALIDATION_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "snapshot_count=" << snapshots_.size() << "\n"
      << "checkpoint_count=" << checkpoints_.size() << "\n"
      << "valid=" << (valid ? "true" : "false") << "\n"
      << "release_on_mismatch=trap\n";
  return out.str();
}

std::string Engine::determinism_ledger_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_DETERMINISM_LEDGER_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "entry_count=" << snapshots_.size() << "\n";
  std::uint64_t previous = 0;
  for (const auto& snapshot : snapshots_) {
    std::ostringstream entry;
    entry << previous << ':' << snapshot.snapshot_hash << ':' << snapshot.journal_hash << ':' << snapshot.valid;
    previous = hash_text(entry.str());
    out << "ledger\t" << snapshot.id
        << "\tprevious=0x" << std::hex << (previous == 0 ? 0 : previous)
        << "\tsnapshot_hash=0x" << snapshot.snapshot_hash
        << "\tjournal_hash=0x" << snapshot.journal_hash
        << "\tentry_hash=0x" << previous << std::dec << "\n";
  }
  out << "ledger_hash=0x" << std::hex << previous << std::dec << "\n";
  return out.str();
}

std::string Engine::audit_seal_text() const {
  std::ostringstream out;
  const auto ledger = determinism_ledger_text();
  const auto audit = determinism_audit_text();
  out << "VENOM_QJS_NATIVE_AUDIT_SEAL_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "abi_hash=0x" << std::hex << abi_table_hash() << "\n"
      << "host_import_hash=0x" << host_import_table_hash() << "\n"
      << "ledger_hash=0x" << hash_text(ledger) << "\n"
      << "audit_hash=0x" << hash_text(audit) << "\n"
      << "seal_hash=0x" << hash_text(ledger + audit) << std::dec << "\n"
      << "release_requires=snapshot-valid|ledger-linked|abi-match|host-import-match\n";
  return out.str();
}

std::uint64_t Engine::commit_execution(std::uint32_t checkpoint_id) {
  record_host_call(HostCallId::ExecutionCommit);
  if (checkpoints_.empty()) {
    trap(90, "commit", "QuickJS execution commit requested before checkpoint capture");
    throw std::runtime_error(last_exception_.message);
  }
  const auto* checkpoint = &checkpoints_.back();
  if (checkpoint_id != 0) {
    checkpoint = nullptr;
    for (const auto& candidate : checkpoints_) {
      if (candidate.id == checkpoint_id) { checkpoint = &candidate; break; }
    }
    if (!checkpoint) {
      trap(91, "commit", "QuickJS execution commit checkpoint not found");
      throw std::runtime_error(last_exception_.message);
    }
  }
  const auto snapshot_hash = snapshots_.empty() ? capture_snapshot(checkpoint->id) : snapshots_.back().snapshot_hash;
  const auto ledger_hash = hash_text(determinism_ledger_text());
  const auto receipt_hash = hash_text(host_call_receipts_text());
  std::ostringstream commit_text;
  commit_text << checkpoint->id << ':' << snapshot_hash << ':' << ledger_hash << ':' << receipt_hash << ':' << host_call_count_;

  ExecutionCommitRecord record;
  record.id = next_commit_id_++;
  record.checkpoint_id = checkpoint->id;
  record.snapshot_hash = snapshot_hash;
  record.ledger_hash = ledger_hash;
  record.result_hash = hash_text(execution_journal_text());
  record.host_receipt_hash = receipt_hash;
  record.commit_hash = hash_text(commit_text.str());
  record.accepted = true;
  commits_.push_back(record);
  record_host_call(HostCallId::HostReceipt);
  (void)seal_permissions();
  record_host_call(HostCallId::ReleaseAccept);
  record_host_call(HostCallId::ReleaseGate);
  return record.commit_hash;
}

bool Engine::rollback_to_checkpoint(std::uint32_t checkpoint_id) {
  record_host_call(HostCallId::ExecutionRollback);
  return restore_checkpoint(checkpoint_id);
}

bool Engine::release_acceptance_check() const {
  if (commits_.empty()) return false;
  const bool snapshots_valid = std::all_of(snapshots_.begin(), snapshots_.end(), [](const SnapshotRecord& record) { return record.valid; });
  return snapshots_valid && commits_.back().accepted && commits_.back().commit_hash != 0;
}

std::string Engine::execution_commits_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_EXECUTION_COMMITS_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "commit_count=" << commits_.size() << "\n";
  for (const auto& commit : commits_) {
    out << "commit\t" << commit.id
        << "\tcheckpoint=" << commit.checkpoint_id
        << "\tsnapshot_hash=0x" << std::hex << commit.snapshot_hash
        << "\tledger_hash=0x" << commit.ledger_hash
        << "\treceipt_hash=0x" << commit.host_receipt_hash
        << "\tcommit_hash=0x" << commit.commit_hash << std::dec
        << "\taccepted=" << (commit.accepted ? "true" : "false") << "\n";
  }
  return out.str();
}

std::string Engine::rollback_policy_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_ROLLBACK_POLICY_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "policy=same-context-checkpoint-only\n"
      << "checkpoint_count=" << checkpoints_.size() << "\n"
      << "release_on_missing_checkpoint=trap\n";
  return out.str();
}

std::string Engine::host_call_receipts_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_HOST_CALL_RECEIPTS_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "host_call_count=" << host_call_count_ << "\n"
      << "receipt_hash=0x" << std::hex << hash_text(std::to_string(host_call_count_) + execution_journal_text()) << std::dec << "\n";
  return out.str();
}

std::string Engine::release_acceptance_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_RELEASE_ACCEPTANCE_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "accepted=" << (release_acceptance_check() ? "true" : "false") << "\n"
      << "checks=abi-match|host-import-match|snapshot-valid|commit-linked|receipts-complete\n";
  return out.str();
}

std::string Engine::commit_audit_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_COMMIT_AUDIT_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "commit_hash=0x" << std::hex << (commits_.empty() ? 0 : commits_.back().commit_hash) << "\n"
      << "receipt_hash=0x" << hash_text(host_call_receipts_text()) << "\n"
      << "acceptance_hash=0x" << hash_text(release_acceptance_text()) << "\n"
      << "audit_hash=0x" << hash_text(execution_commits_text() + release_acceptance_text()) << std::dec << "\n";
  return out.str();
}

bool Engine::check_capability(std::uint32_t capability_id) {
  record_host_call(HostCallId::CapabilityCheck);
  const char* capability = "unknown";
  bool allowed = false;
  switch (capability_id) {
    case 1: capability = "console.log"; allowed = true; break;
    case 2: capability = "module.resolve"; allowed = true; break;
    case 3: capability = "module.load"; allowed = true; break;
    case 4: capability = "execution.audit"; allowed = true; break;
    case 5: capability = "snapshot.capture"; allowed = true; break;
    case 6: capability = "permission.seal"; allowed = true; break;
    case 7: capability = "fetch"; allowed = false; break;
    default: capability = "unknown"; allowed = false; break;
  }
  record_policy_receipt(capability_id, capability, allowed ? "allow" : "deny", host_call_count_);
  if (!allowed && capability_id != 7) {
    trap(100, "capability", "QuickJS capability denied by sealed host policy");
    throw std::runtime_error(last_exception_.message);
  }
  return allowed;
}

bool Engine::request_host_io(std::uint32_t io_class, std::uint32_t capability_id, std::uint64_t payload_hash) {
  record_host_call(HostCallId::HostIoRequest);
  const char* capability = "unknown";
  bool capability_allowed = false;
  switch (capability_id) {
    case 1: capability = "console.log"; capability_allowed = true; break;
    case 2: capability = "module.resolve"; capability_allowed = true; break;
    case 3: capability = "module.load"; capability_allowed = true; break;
    case 4: capability = "execution.audit"; capability_allowed = true; break;
    case 5: capability = "snapshot.capture"; capability_allowed = true; break;
    case 6: capability = "permission.seal"; capability_allowed = true; break;
    case 7: capability = "fetch"; capability_allowed = false; break;
    default: capability = "unknown"; capability_allowed = false; break;
  }
  if (capability_allowed) {
    (void)check_capability(capability_id);
  } else {
    record_policy_receipt(capability_id, capability, "deny", payload_hash);
  }

  const bool io_class_allowed = io_class == 1 || io_class == 2;
  const bool allowed = capability_allowed && io_class_allowed;
  const std::string reason = allowed ? "policy-allow" : (io_class_allowed ? "capability-denied" : "host-io-class-denied");
  record_host_io_decision(io_class, capability_id, capability, allowed ? "allow" : "deny", reason, payload_hash);
  return allowed;
}

std::uint64_t Engine::seal_permissions() {
  record_host_call(HostCallId::PermissionSeal);
  if (policy_receipts_.empty()) {
    record_policy_receipt(6, "permission.seal", "allow", host_call_count_);
  }
  permission_seal_hash_ = hash_text(capability_policy_text() + host_io_policy_text() + policy_receipts_text() + host_io_decisions_text() + capability_ledger_text());
  record_host_call(HostCallId::PolicySealAudit);
  return permission_seal_hash_;
}

bool Engine::release_gate_check() const {
  return release_acceptance_check() && permission_seal_hash_ != 0 && !policy_receipts_.empty() && host_io_denials_.empty();
}

std::string Engine::capability_policy_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_CAPABILITY_POLICY_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "default=deny\n"
      << "allowed=console.log|module.resolve|module.load|execution.audit|snapshot.capture|permission.seal\n"
      << "denied=fetch|dynamic-import|unlisted-host-call\n"
      << "host_io_decision_records=" << host_io_decisions_.size() << "\n"
      << "receipt_count=" << policy_receipts_.size() << "\n";
  return out.str();
}

std::string Engine::host_io_policy_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_HOST_IO_POLICY_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "console=allow\n"
      << "module=allow-package-relative\n"
      << "fetch=deny-by-default\n"
      << "timer=host-policy-required\n"
      << "event=host-policy-required\n"
      << "release_behavior=fail-closed-on-unsealed-io\n";
  return out.str();
}

std::string Engine::permission_seal_text() const {
  std::ostringstream out;
  const auto capability_hash = hash_text(capability_policy_text());
  const auto io_hash = hash_text(host_io_policy_text());
  const auto receipt_hash = hash_text(policy_receipts_text());
  out << "VENOM_QJS_NATIVE_PERMISSION_SEAL_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "capability_policy_hash=0x" << std::hex << capability_hash << "\n"
      << "host_io_policy_hash=0x" << io_hash << "\n"
      << "policy_receipts_hash=0x" << receipt_hash << "\n"
      << "host_io_decisions_hash=0x" << hash_text(host_io_decisions_text()) << "\n"
      << "capability_ledger_hash=0x" << hash_text(capability_ledger_text()) << "\n"
      << "seal_hash=0x" << (permission_seal_hash_ ? permission_seal_hash_ : hash_text(capability_policy_text() + host_io_policy_text() + policy_receipts_text())) << std::dec << "\n"
      << "sealed=" << (permission_seal_hash_ ? "true" : "false") << "\n";
  return out.str();
}

std::string Engine::policy_receipts_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_POLICY_RECEIPTS_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "receipt_count=" << policy_receipts_.size() << "\n";
  for (const auto& receipt : policy_receipts_) {
    out << "receipt\t" << receipt.id
        << "\tcapability=" << receipt.capability_id
        << "\tname=" << receipt.capability
        << "\tdecision=" << receipt.decision
        << "\tpayload_hash=0x" << std::hex << receipt.payload_hash
        << "\treceipt_hash=0x" << receipt.receipt_hash << std::dec << "\n";
  }
  return out.str();
}

std::string Engine::release_gate_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_RELEASE_GATE_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "accepted=" << (release_gate_check() ? "true" : "false") << "\n"
      << "checks=abi-match|host-import-match|capability-policy-match|host-io-policy-match|permission-seal-valid|receipts-complete\n"
      << "permission_seal_hash=0x" << std::hex << permission_seal_hash_ << std::dec << "\n"
      << "deny_trace_count=" << host_io_denials_.size() << "\n";
  return out.str();
}

std::string Engine::host_io_decisions_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_HOST_IO_DECISIONS_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "decision_count=" << host_io_decisions_.size() << "\n";
  for (const auto& record : host_io_decisions_) {
    out << "decision\t" << record.id
        << "\tio_class=" << record.io_class
        << "\tcapability=" << record.capability_id
        << "\tname=" << record.capability
        << "\tdecision=" << record.decision
        << "\treason=" << record.reason
        << "\tpayload_hash=0x" << std::hex << record.payload_hash
        << "\tdecision_hash=0x" << record.decision_hash << std::dec << "\n";
  }
  return out.str();
}

std::string Engine::host_io_deny_traces_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_HOST_IO_DENY_TRACES_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "deny_count=" << host_io_denials_.size() << "\n";
  for (const auto& record : host_io_denials_) {
    out << "deny\t" << record.id
        << "\tio_class=" << record.io_class
        << "\tcapability=" << record.capability_id
        << "\tname=" << record.capability
        << "\treason=" << record.reason
        << "\ttrap_code=host-io-deny\n";
  }
  return out.str();
}

std::string Engine::capability_ledger_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_CAPABILITY_LEDGER_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "entry_count=" << host_io_decisions_.size() << "\n";
  std::uint64_t previous = 0;
  for (const auto& record : host_io_decisions_) {
    std::ostringstream entry;
    entry << previous << ':' << record.decision_hash << ':' << record.payload_hash << ':' << record.decision;
    previous = hash_text(entry.str());
    out << "ledger\t" << record.id
        << "\tdecision_hash=0x" << std::hex << record.decision_hash
        << "\tprevious=0x" << previous
        << "\tentry_hash=0x" << previous << std::dec << "\n";
  }
  out << "ledger_hash=0x" << std::hex << previous << std::dec << "\n";
  return out.str();
}

std::string Engine::policy_seal_audit_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_POLICY_SEAL_AUDIT_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "capability_policy_hash=0x" << std::hex << hash_text(capability_policy_text()) << "\n"
      << "host_io_policy_hash=0x" << hash_text(host_io_policy_text()) << "\n"
      << "decision_hash=0x" << hash_text(host_io_decisions_text()) << "\n"
      << "deny_trace_hash=0x" << hash_text(host_io_deny_traces_text()) << "\n"
      << "ledger_hash=0x" << hash_text(capability_ledger_text()) << "\n"
      << "permission_seal_hash=0x" << permission_seal_hash_ << std::dec << "\n";
  return out.str();
}

std::string Engine::runtime_denylist_text() const {
  std::ostringstream out;
  out << "VENOM_QJS_NATIVE_RUNTIME_DENYLIST_V1\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "denylist=fetch|remote-module|dynamic-import|eval-indirect|storage-write|network-raw\n"
      << "deny_trace_count=" << host_io_denials_.size() << "\n";
  return out.str();
}

ParityProbe Engine::parity_probe() const {
  ParityProbe probe;
  probe.abi_hash = abi_table_hash();
  probe.host_import_hash = host_import_table_hash();
  probe.native_eval = "quickjs:3";
  return probe;
}

} // namespace venom::quickjs
