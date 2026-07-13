#include "quickjs/abi.hpp"

#include "package/hash.hpp"

#include <sstream>

namespace venom::quickjs {

const std::vector<AbiEntry>& runtime_abi_entries() {
  static const std::vector<AbiEntry> entries = {
      {"memory", "export", "WebAssembly.Memory", "linear memory for ABI tables, script buffers, results, and console records"},
      {"venom_qjs_engine_abi", "export", "() -> u32", "runtime ABI major version"},
      {"venom_qjs_engine_version", "export", "() -> u32", "Venom package/runtime version"},
      {"venom_qjs_abi_table_ptr", "export", "() -> ptr", "pointer to newline-delimited ABI table"},
      {"venom_qjs_abi_table_size", "export", "() -> u32", "ABI table byte size"},
      {"venom_qjs_abi_table_hash", "export", "() -> u32", "FNV-1a32 hash of ABI table bytes"},
      {"venom_qjs_abi_entry_count", "export", "() -> u32", "number of ABI table entries"},
      {"venom_qjs_context_alloc", "export", "() -> ctx", "allocate a runtime context"},
      {"venom_qjs_context_free", "export", "(ctx) -> bool", "free a runtime context"},
      {"venom_qjs_context_set_limits", "export", "(ctx, heap_bytes, stack_bytes) -> bool", "set per-context accounting limits"},
      {"venom_qjs_context_heap_limit", "export", "(ctx) -> u32", "read context heap limit"},
      {"venom_qjs_context_heap_used", "export", "(ctx) -> u32", "read context accounted bytes"},
      {"venom_qjs_context_stack_limit", "export", "(ctx) -> u32", "read context stack limit"},
      {"venom_qjs_script_buffer_alloc", "export", "(ctx, bytes) -> ptr", "allocate/copy target for script source bytes"},
      {"venom_qjs_script_buffer_ptr", "export", "() -> ptr", "active script byte buffer pointer"},
      {"venom_qjs_script_buffer_size", "export", "() -> u32", "active script byte buffer size"},
      {"venom_qjs_script_buffer_capacity", "export", "() -> u32", "active script byte buffer capacity"},
      {"venom_qjs_script_buffer_free", "export", "(ctx, ptr) -> bool", "release active script buffer accounting"},
      {"venom_qjs_execute_source", "export", "(ctx, bytes) -> bool", "execute the active script source buffer"},
      {"venom_qjs_result_ptr", "export", "() -> ptr", "JSON result pointer"},
      {"venom_qjs_result_size", "export", "() -> u32", "JSON result byte size"},
      {"venom_qjs_execution_record_ptr", "export", "() -> ptr", "JSON execution record pointer"},
      {"venom_qjs_execution_record_size", "export", "() -> u32", "JSON execution record size"},
      {"venom_qjs_console_callback_abi", "export", "() -> u32", "console callback ABI version"},
      {"venom_qjs_console_event_ptr", "export", "() -> ptr", "latest console event JSON pointer"},
      {"venom_qjs_console_event_size", "export", "() -> u32", "latest console event JSON size"},
      {"venom_qjs_console_event_count", "export", "() -> u32", "captured console event count"},
      {"venom_qjs_console_clear", "export", "() -> u32", "clear captured console events"},
      {"venom_qjs_host_import_table_ptr", "export", "() -> ptr", "host import contract table pointer"},
      {"venom_qjs_host_import_table_size", "export", "() -> u32", "host import contract byte size"},
      {"venom_qjs_host_import_table_hash", "export", "() -> u32", "FNV-1a32 host import table hash"},
      {"venom_qjs_host_import_count", "export", "() -> u32", "number of host import contract entries"},
      {"venom_qjs_parity_probe", "export", "() -> u32", "deterministic native/WASM ABI parity probe value"},
      {"venom_qjs_fallback_required", "export", "() -> bool", "true when scaffold requires host-compatible execution fallback"},
      {"venom_qjs_backend_kind", "export", "() -> u32", "backend kind: scaffold/real/native"},
      {"venom_qjs_backend_ready", "export", "() -> bool", "backend load readiness"},
      {"venom_qjs_real_engine_candidate", "export", "() -> bool", "true only for real QuickJS execution backends"},
      {"venom_qjs_bytecode_manifest_ptr", "export", "() -> ptr", "QuickJS bytecode/cache manifest pointer"},
      {"venom_qjs_bytecode_manifest_size", "export", "() -> u32", "QuickJS bytecode/cache manifest byte size"},
      {"venom_qjs_bytecode_manifest_hash", "export", "() -> u32", "FNV-1a32 bytecode manifest hash"},
      {"venom_qjs_module_resolver_abi", "export", "() -> u32", "module resolver ABI version"},
      {"venom_qjs_module_resolve", "export", "(specifier_ptr, specifier_size, referrer_ptr, referrer_size) -> bool", "resolve package-relative module specifiers"},
      {"venom_qjs_module_record_ptr", "export", "() -> ptr", "latest module resolver record JSON pointer"},
      {"venom_qjs_module_record_size", "export", "() -> u32", "latest module resolver record JSON size"},
      {"venom_qjs_exception_abi", "export", "() -> u32", "exception record ABI version"},
      {"venom_qjs_exception_ptr", "export", "() -> ptr", "latest exception record JSON pointer"},
      {"venom_qjs_exception_size", "export", "() -> u32", "latest exception record JSON size"},
      {"venom_qjs_exception_code", "export", "() -> u32", "latest exception code"},
      {"venom_qjs_exception_clear", "export", "() -> u32", "clear latest exception record"},
      {"venom_qjs_host_trap_policy_ptr", "export", "() -> ptr", "host trap policy pointer"},
      {"venom_qjs_host_trap_policy_size", "export", "() -> u32", "host trap policy byte size"},
      {"venom_qjs_host_trap_policy_hash", "export", "() -> u32", "FNV-1a32 host trap policy hash"},
      {"venom_qjs_engine_state", "export", "() -> u32", "current engine lifecycle state"},
      {"venom_qjs_engine_trap_code", "export", "() -> u32", "latest lifecycle or host trap code"},
      {"venom_qjs_script_buffer_expected_hash", "export", "() -> u32", "expected script buffer hash set before execution"},
      {"venom_qjs_script_buffer_set_expected_hash", "export", "(ctx, hash) -> bool", "set expected source hash before executing"},
      {"venom_qjs_script_buffer_alloc_count", "export", "() -> u32", "script byte buffer allocation counter"},
      {"venom_qjs_script_buffer_free_count", "export", "() -> u32", "script byte buffer free counter"},
      {"venom_qjs_host_call_known", "export", "(id) -> bool", "return true for stable host-call dispatch IDs"},
      {"venom_qjs_host_call_dispatch", "export", "(id, payload_ptr, payload_size) -> bool", "dispatch or trap a stable host call record"},
      {"venom_qjs_host_call_count", "export", "() -> u32", "host call dispatch counter"},
      {"venom_qjs_exception_message_ptr", "export", "() -> ptr", "latest exception message pointer"},
      {"venom_qjs_exception_message_size", "export", "() -> u32", "latest exception message byte size"},
      {"venom_qjs_module_graph_ptr", "export", "() -> ptr", "module graph metadata pointer"},
      {"venom_qjs_module_graph_size", "export", "() -> u32", "module graph metadata byte size"},
      {"venom_qjs_module_graph_hash", "export", "() -> u32", "FNV-1a32 module graph hash"},
      {"venom_qjs_module_execute", "export", "(ctx, module_id, bytes) -> bool", "execute a package module record through the prototype module boundary"},
      {"venom_qjs_module_execution_count", "export", "() -> u32", "module execution counter"},
      {"venom_qjs_module_cache_state_ptr", "export", "() -> ptr", "module namespace cache state JSON pointer"},
      {"venom_qjs_module_cache_state_size", "export", "() -> u32", "module namespace cache state JSON size"},
      {"venom_qjs_resolver_audit_ptr", "export", "() -> ptr", "latest resolver audit record pointer"},
      {"venom_qjs_resolver_audit_size", "export", "() -> u32", "latest resolver audit record byte size"},
      {"venom_qjs_interop_fallback_required", "export", "() -> bool", "true when host ESM transform fallback is required"},
      {"venom_qjs_script_record_ptr", "export", "() -> ptr", "prepared script execution record pointer"},
      {"venom_qjs_script_record_size", "export", "() -> u32", "prepared script execution record byte size"},
      {"venom_qjs_script_record_hash", "export", "() -> u32", "FNV-1a32 script execution record hash"},
      {"venom_qjs_eval_result_ptr", "export", "() -> ptr", "latest evaluation result record pointer"},
      {"venom_qjs_eval_result_size", "export", "() -> u32", "latest evaluation result record byte size"},
      {"venom_qjs_eval_result_hash", "export", "() -> u32", "FNV-1a32 evaluation result record hash"},
      {"venom_qjs_console_capture_ptr", "export", "() -> ptr", "console capture drain record pointer"},
      {"venom_qjs_console_capture_size", "export", "() -> u32", "console capture drain record byte size"},
      {"venom_qjs_failure_report_ptr", "export", "() -> ptr", "strict failure report pointer"},
      {"venom_qjs_failure_report_size", "export", "() -> u32", "strict failure report byte size"},
      {"venom_qjs_execution_journal_ptr", "export", "() -> ptr", "deterministic execution journal pointer"},
      {"venom_qjs_execution_journal_size", "export", "() -> u32", "deterministic execution journal byte size"},
      {"venom_qjs_execution_journal_hash", "export", "() -> u32", "FNV-1a32 execution journal hash"},
      {"venom_qjs_checkpoint_policy_ptr", "export", "() -> ptr", "checkpoint policy pointer"},
      {"venom_qjs_checkpoint_policy_size", "export", "() -> u32", "checkpoint policy byte size"},
      {"venom_qjs_checkpoint_policy_hash", "export", "() -> u32", "FNV-1a32 checkpoint policy hash"},
      {"venom_qjs_checkpoint_create", "export", "(ctx, stage_id) -> checkpoint_id", "capture a deterministic execution checkpoint"},
      {"venom_qjs_checkpoint_restore", "export", "(ctx, checkpoint_id) -> bool", "restore checkpoint-visible counters for deterministic replay"},
      {"venom_qjs_replay_cursor_ptr", "export", "() -> ptr", "current replay cursor record pointer"},
      {"venom_qjs_replay_cursor_size", "export", "() -> u32", "current replay cursor record byte size"},
      {"venom_qjs_replay_cursor_hash", "export", "() -> u32", "FNV-1a32 replay cursor hash"},
      {"venom_qjs_replay_advance", "export", "(ctx, stage_id) -> bool", "advance deterministic replay cursor"},
      {"venom_qjs_resume_state_ptr", "export", "() -> ptr", "resume state record pointer"},
      {"venom_qjs_resume_state_size", "export", "() -> u32", "resume state record byte size"},
      {"venom_qjs_resume_state_hash", "export", "() -> u32", "FNV-1a32 resume state hash"},
      {"venom_qjs_determinism_audit_ptr", "export", "() -> ptr", "determinism audit record pointer"},
      {"venom_qjs_determinism_audit_size", "export", "() -> u32", "determinism audit record byte size"},
      {"venom_qjs_determinism_audit_hash", "export", "() -> u32", "FNV-1a32 determinism audit hash"},
      {"venom_qjs_snapshot_policy_ptr", "export", "() -> ptr", "snapshot validation policy pointer"},
      {"venom_qjs_snapshot_policy_size", "export", "() -> u32", "snapshot validation policy byte size"},
      {"venom_qjs_snapshot_policy_hash", "export", "() -> u32", "FNV-1a32 snapshot policy hash"},
      {"venom_qjs_snapshot_record_ptr", "export", "() -> ptr", "latest deterministic snapshot record pointer"},
      {"venom_qjs_snapshot_record_size", "export", "() -> u32", "latest deterministic snapshot record byte size"},
      {"venom_qjs_snapshot_record_hash", "export", "() -> u32", "latest deterministic snapshot record hash"},
      {"venom_qjs_snapshot_capture", "export", "(ctx, stage) -> snapshot_hash", "capture a deterministic validation snapshot"},
      {"venom_qjs_snapshot_validate", "export", "(ctx, expected_hash) -> bool", "validate latest deterministic snapshot hash"},
      {"venom_qjs_replay_validation_ptr", "export", "() -> ptr", "replay validation record pointer"},
      {"venom_qjs_replay_validation_size", "export", "() -> u32", "replay validation record byte size"},
      {"venom_qjs_replay_validation_hash", "export", "() -> u32", "replay validation record hash"},
      {"venom_qjs_determinism_ledger_ptr", "export", "() -> ptr", "determinism ledger pointer"},
      {"venom_qjs_determinism_ledger_size", "export", "() -> u32", "determinism ledger byte size"},
      {"venom_qjs_determinism_ledger_hash", "export", "() -> u32", "determinism ledger hash"},
      {"venom_qjs_audit_seal_ptr", "export", "() -> ptr", "audit seal pointer"},
      {"venom_qjs_audit_seal_size", "export", "() -> u32", "audit seal byte size"},
      {"venom_qjs_audit_seal_hash", "export", "() -> u32", "audit seal hash"},
      {"venom_qjs_execution_commit_ptr", "export", "() -> ptr", "latest execution commit record pointer"},
      {"venom_qjs_execution_commit_size", "export", "() -> u32", "latest execution commit record byte size"},
      {"venom_qjs_execution_commit_hash", "export", "() -> u32", "latest execution commit hash"},
      {"venom_qjs_execution_commit", "export", "(ctx, checkpoint_id) -> commit_hash", "seal execution state into a commit record"},
      {"venom_qjs_rollback_policy_ptr", "export", "() -> ptr", "rollback policy pointer"},
      {"venom_qjs_rollback_policy_size", "export", "() -> u32", "rollback policy byte size"},
      {"venom_qjs_rollback_policy_hash", "export", "() -> u32", "rollback policy hash"},
      {"venom_qjs_execution_rollback", "export", "(ctx, checkpoint_id) -> bool", "rollback to a committed checkpoint"},
      {"venom_qjs_host_receipts_ptr", "export", "() -> ptr", "host-call receipts pointer"},
      {"venom_qjs_host_receipts_size", "export", "() -> u32", "host-call receipts byte size"},
      {"venom_qjs_host_receipts_hash", "export", "() -> u32", "host-call receipts hash"},
      {"venom_qjs_release_acceptance_ptr", "export", "() -> ptr", "release acceptance record pointer"},
      {"venom_qjs_release_acceptance_size", "export", "() -> u32", "release acceptance byte size"},
      {"venom_qjs_release_acceptance_hash", "export", "() -> u32", "release acceptance hash"},
      {"venom_qjs_release_accept", "export", "(ctx) -> bool", "validate release acceptance gates"},
      {"venom_qjs_commit_audit_ptr", "export", "() -> ptr", "commit audit pointer"},
      {"venom_qjs_commit_audit_size", "export", "() -> u32", "commit audit byte size"},
      {"venom_qjs_commit_audit_hash", "export", "() -> u32", "commit audit hash"},
      {"venom_qjs_capability_policy_ptr", "export", "() -> ptr", "capability policy pointer"},
      {"venom_qjs_capability_policy_size", "export", "() -> u32", "capability policy byte size"},
      {"venom_qjs_capability_policy_hash", "export", "() -> u32", "capability policy hash"},
      {"venom_qjs_host_io_policy_ptr", "export", "() -> ptr", "host I/O policy pointer"},
      {"venom_qjs_host_io_policy_size", "export", "() -> u32", "host I/O policy byte size"},
      {"venom_qjs_host_io_policy_hash", "export", "() -> u32", "host I/O policy hash"},
      {"venom_qjs_permission_seal_ptr", "export", "() -> ptr", "permission seal pointer"},
      {"venom_qjs_permission_seal_size", "export", "() -> u32", "permission seal byte size"},
      {"venom_qjs_permission_seal_hash", "export", "() -> u32", "permission seal hash"},
      {"venom_qjs_policy_receipts_ptr", "export", "() -> ptr", "policy receipt records pointer"},
      {"venom_qjs_policy_receipts_size", "export", "() -> u32", "policy receipt records byte size"},
      {"venom_qjs_policy_receipts_hash", "export", "() -> u32", "policy receipt records hash"},
      {"venom_qjs_capability_check", "export", "(ctx, capability_id) -> bool", "check sealed runtime capability policy"},
      {"venom_qjs_permission_seal", "export", "(ctx) -> seal_hash", "seal current permission policy and receipts"},
      {"venom_qjs_release_gate_ptr", "export", "() -> ptr", "release gate record pointer"},
      {"venom_qjs_release_gate_size", "export", "() -> u32", "release gate record byte size"},
      {"venom_qjs_release_gate_hash", "export", "() -> u32", "release gate record hash"},
      {"venom_qjs_release_gate", "export", "(ctx) -> bool", "validate sealed release-gate policy"},
      {"venom_qjs_status_code", "export", "() -> u32", "last ABI status code"},
      {"venom_qjs_status_record_ptr", "export", "() -> ptr", "status record pointer"},
      {"venom_qjs_status_record_size", "export", "() -> u32", "status record byte size"},
      {"venom_qjs_status_record_hash", "export", "() -> u32", "status record hash"},
      {"venom_qjs_runtime_limits_ptr", "export", "() -> ptr", "runtime limits table pointer"},
      {"venom_qjs_runtime_limits_size", "export", "() -> u32", "runtime limits table byte size"},
      {"venom_qjs_runtime_limits_hash", "export", "() -> u32", "runtime limits table hash"},
      {"venom_qjs_context_report_ptr", "export", "(ctx) -> ptr", "context accounting report pointer"},
      {"venom_qjs_context_report_size", "export", "() -> u32", "context accounting report byte size"},
      {"venom_qjs_context_report_hash", "export", "() -> u32", "context accounting report hash"},
      {"venom_qjs_bytecode_validate", "export", "(ctx, bytes) -> status", "validate active VQJSBC03 record without executing"},
      {"venom_qjs_bytecode_record_hash32", "export", "() -> u32", "last bytecode record hash"},
      {"venom_qjs_bytecode_payload_size", "export", "() -> u32", "native QuickJS object payload byte size"},
      {"venom_qjs_bytecode_expected_source_hash32", "export", "() -> u32", "compile-time source hash metadata"},
      {"venom_qjs_backend_contract_ptr", "export", "() -> ptr", "backend contract record pointer"},
      {"venom_qjs_backend_contract_size", "export", "() -> u32", "backend contract record byte size"},
      {"venom_qjs_backend_contract_hash", "export", "() -> u32", "backend contract hash"},
      {"venom_qjs_release_status", "export", "() -> u32", "fail-closed release status"},
      {"venom_qjs_interpreter_ready", "export", "() -> bool", "QuickJS WASM interpreter dispatch is available"},
      {"venom_qjs_interpreter_contract_ptr", "export", "() -> ptr", "interpreter contract record pointer"},
      {"venom_qjs_interpreter_contract_size", "export", "() -> u32", "interpreter contract record byte size"},
      {"venom_qjs_interpreter_contract_hash", "export", "() -> u32", "interpreter contract record hash"},
      {"venom_qjs_interpreter_dispatch_count", "export", "() -> u32", "interpreter dispatch counter"},
      {"venom_qjs_interpreter_opcode_count", "export", "() -> u32", "last interpreter opcode estimate"},
      {"venom_qjs_global_slot_count", "export", "() -> u32", "tracked global writes"},
      {"venom_qjs_last_global_write_hash", "export", "() -> u32", "last tracked global write hash"},
      {"venom_qjs_upstream_bytecode_semantics_record_ptr", "export", "() -> ptr", "upstream bytecode semantics record pointer"},
      {"venom_qjs_upstream_bytecode_semantics_score", "export", "() -> u32", "upstream bytecode semantics score"},
      {"venom_qjs_upstream_intrinsic_record_ptr", "export", "() -> ptr", "upstream intrinsic semantics record pointer"},
      {"venom_qjs_upstream_intrinsic_record_size", "export", "() -> u32", "upstream intrinsic semantics record size"},
      {"venom_qjs_upstream_intrinsic_record_hash", "export", "() -> u32", "upstream intrinsic semantics record hash"},
      {"venom_qjs_upstream_intrinsic_semantics_score", "export", "() -> u32", "upstream intrinsic semantics score"},
      {"venom_qjs_upstream_property_descriptor_record_ptr", "export", "() -> ptr", "upstream property descriptor record pointer"},
      {"venom_qjs_upstream_property_descriptor_record_size", "export", "() -> u32", "upstream property descriptor record size"},
      {"venom_qjs_upstream_property_descriptor_record_hash", "export", "() -> u32", "upstream property descriptor record hash"},
      {"venom_qjs_upstream_prototype_chain_record_ptr", "export", "() -> ptr", "upstream prototype chain record pointer"},
      {"venom_qjs_upstream_prototype_chain_record_size", "export", "() -> u32", "upstream prototype chain record size"},
      {"venom_qjs_upstream_prototype_chain_record_hash", "export", "() -> u32", "upstream prototype chain record hash"},
      {"venom_qjs_upstream_call_frame_record_ptr", "export", "() -> ptr", "upstream call frame record pointer"},
      {"venom_qjs_upstream_call_frame_record_size", "export", "() -> u32", "upstream call frame record size"},
      {"venom_qjs_upstream_call_frame_record_hash", "export", "() -> u32", "upstream call frame record hash"},
  };
  return entries;
}

std::string runtime_abi_table_text() {
  std::ostringstream out;
  const auto& entries = runtime_abi_entries();
  out << "VENOM_QJS_RUNTIME_ABI_V12\n"
      << "abi=" << kRuntimeAbiVersion << "\n"
      << "package_version=" << kRuntimePackageVersion << "\n"
      << "entry_count=" << entries.size() << "\n"
      << "default_heap_limit=" << kDefaultHeapLimitBytes << "\n"
      << "default_stack_limit=" << kDefaultStackLimitBytes << "\n"
      << "script_buffer_limit=" << kDefaultScriptBufferLimitBytes << "\n"
      << "context_limit=" << kDefaultContextLimit << "\n"
      << "host_call_limit=" << kDefaultHostCallLimit << "\n"
      << "console_event_limit=" << kDefaultConsoleEventLimit << "\n"
      << "module_record_limit=" << kDefaultModuleRecordLimit << "\n";
  for (const auto& entry : entries) {
    out << "entry\t" << entry.name << "\t" << entry.kind << "\t" << entry.signature << "\t" << entry.purpose << "\n";
  }
  return out.str();
}

std::string host_import_table_text() {
  std::ostringstream out;
  out << "VENOM_QJS_HOST_IMPORT_TABLE_V10\n"
      << "version=10\n"
      << "required=false\n"
      << "mode=stable-host-call-id-table\n"
      << "entry_count=38\n"
      << "import\t1\tconsole.log\t(console_event_record) -> void\truntime\n"
      << "import\t2\tconsole.warn\t(console_event_record) -> void\truntime\n"
      << "import\t3\tconsole.error\t(console_event_record) -> void\truntime\n"
      << "import\t4\tmodule.resolve\t(specifier, referrer) -> module_record\truntime\n"
      << "import\t5\tmodule.load\t(module_record) -> source_record\tfuture-real-engine\n"
      << "import\t6\tfetch.blocked\t(request_record) -> denied_response\ttrap\n"
      << "import\t7\tmodule.instantiate\t(module_record) -> namespace_record\truntime\n"
      << "import\t8\tmodule.evaluate\t(module_record) -> result_record\truntime\n"
      << "import\t9\tmodule.cache.get\t(specifier, referrer) -> namespace_record\truntime\n"
      << "import\t10\tmodule.cache.put\t(specifier, namespace_record) -> void\truntime\n"
      << "import\t11\texecution.prepare\t(script_record) -> prepared_record\truntime\n"
      << "import\t12\texecution.evaluate\t(prepared_record) -> eval_result\truntime\n"
      << "import\t13\texecution.result\t(eval_result) -> result_record\truntime\n"
      << "import\t14\tconsole.capture.flush\t(console_capture) -> void\truntime\n"
      << "import\t15\texecution.checkpoint\t(checkpoint_record) -> checkpoint_id\truntime\n"
      << "import\t16\texecution.replay\t(replay_cursor) -> replay_status\truntime\n"
      << "import\t17\texecution.resume\t(resume_state) -> execution_state\truntime\n"
      << "import\t18\texecution.audit\t(determinism_audit) -> audit_status\truntime\n"
      << "import\t19\tsnapshot.capture\t(snapshot_record) -> snapshot_hash\truntime\n"
      << "import\t20\tsnapshot.validate\t(snapshot_hash) -> validation_status\truntime\n"
      << "import\t21\tdeterminism.ledger.append\t(ledger_entry) -> ledger_hash\truntime\n"
      << "import\t22\taudit.seal\t(audit_record) -> seal_hash\truntime\n"
      << "import\t23\texecution.commit\t(commit_record) -> commit_hash\truntime\n"
      << "import\t24\texecution.rollback\t(rollback_marker) -> checkpoint_id\truntime\n"
      << "import\t25\thost.receipt\t(host_call_receipt) -> receipt_hash\truntime\n"
      << "import\t26\trelease.accept\t(acceptance_record) -> acceptance_hash\truntime\n"
      << "import\t27\tcapability.check\t(capability_record) -> allow|deny\truntime\n"
      << "import\t28\thost.io.request\t(io_record) -> policy_decision\truntime\n"
      << "import\t29\tpermission.seal\t(permission_record) -> seal_hash\truntime\n"
      << "import\t30\trelease.gate\t(gate_record) -> release_decision\truntime\n"
      << "import\t31\thost.io.decision\t(io_decision_record) -> decision_hash\truntime\n"
      << "import\t32\thost.io.deny\t(deny_trace_record) -> deny_hash\ttrap\n"
      << "import\t33\tcapability.ledger.append\t(capability_ledger_entry) -> ledger_hash\truntime\n"
      << "import\t34\tpolicy.seal.audit\t(policy_seal_audit) -> audit_hash\truntime\n"
      << "import\t35\tbytecode.validate\t(bytecode_record) -> validation_status\truntime\n"
      << "import\t36\tcontext.report\t(context_accounting) -> report_hash\truntime\n"
      << "import\t37\tstatus.emit\t(status_record) -> status_hash\truntime\n"
      << "import\t38\trelease.status\t(release_status) -> status_hash\truntime\n"
      << "release_unknown_host_call=trap\n"
      << "debug_unknown_host_call=record-and-fallback\n";
  return out.str();
}

std::uint64_t abi_table_hash() {
  const auto text = runtime_abi_table_text();
  return venom::package::fnv1a64(std::vector<unsigned char>(text.begin(), text.end()));
}

std::uint64_t host_import_table_hash() {
  const auto text = host_import_table_text();
  return venom::package::fnv1a64(std::vector<unsigned char>(text.begin(), text.end()));
}

std::string parity_probe_text(const ParityProbe& probe) {
  std::ostringstream out;
  out << "VENOM_QJS_PARITY_PROBE_V7\n"
      << "abi=" << probe.abi << "\n"
      << "package_version=" << probe.package_version << "\n"
      << "native_eval=" << probe.native_eval << "\n"
      << "native_abi_hash=0x" << std::hex << probe.abi_hash << std::dec << "\n"
      << "host_import_hash=0x" << std::hex << probe.host_import_hash << std::dec << "\n"
      << "wasm_probe_export=venom_qjs_parity_probe\n"
      << "wasm_abi_table_export=venom_qjs_abi_table_ptr|venom_qjs_abi_table_size|venom_qjs_abi_table_hash\n"
      << "wasm_context_limits=venom_qjs_context_set_limits|venom_qjs_context_heap_limit|venom_qjs_context_heap_used|venom_qjs_context_stack_limit\n"
      << "wasm_script_buffer=venom_qjs_script_buffer_alloc|venom_qjs_script_buffer_ptr|venom_qjs_script_buffer_size|venom_qjs_script_buffer_capacity|venom_qjs_script_buffer_free\n"
      << "wasm_console_callback_abi=venom_qjs_console_callback_abi|venom_qjs_console_event_ptr|venom_qjs_console_event_size|venom_qjs_console_event_count|venom_qjs_console_clear\n"
      << "wasm_bytecode_manifest=venom_qjs_bytecode_manifest_ptr|venom_qjs_bytecode_manifest_size|venom_qjs_bytecode_manifest_hash\n"
      << "wasm_abi12_status=venom_qjs_status_code|venom_qjs_status_record_ptr|venom_qjs_status_record_size|venom_qjs_release_status\n"
      << "wasm_abi12_limits=venom_qjs_runtime_limits_ptr|venom_qjs_runtime_limits_size|venom_qjs_context_report_ptr|venom_qjs_context_report_size\n"
      << "wasm_abi12_bytecode=venom_qjs_bytecode_validate|venom_qjs_bytecode_record_hash32|venom_qjs_bytecode_payload_size|venom_qjs_backend_contract_ptr\n"
      << "wasm_interpreter_dispatch=venom_qjs_interpreter_ready|venom_qjs_interpreter_dispatch_count|venom_qjs_interpreter_opcode_count|venom_qjs_global_slot_count\n"
      << "wasm_module_resolver=venom_qjs_module_resolver_abi|venom_qjs_module_resolve|venom_qjs_module_record_ptr|venom_qjs_module_record_size\n"
      << "wasm_exception_abi=venom_qjs_exception_abi|venom_qjs_exception_ptr|venom_qjs_exception_size|venom_qjs_exception_code|venom_qjs_exception_message_ptr|venom_qjs_exception_message_size|venom_qjs_exception_clear\n"
      << "wasm_lifecycle=venom_qjs_engine_state|venom_qjs_engine_trap_code\n"
      << "wasm_host_dispatch=venom_qjs_host_call_known|venom_qjs_host_call_dispatch|venom_qjs_host_call_count\n"
      << "wasm_replay_checkpoint=venom_qjs_execution_journal_ptr|venom_qjs_checkpoint_create|venom_qjs_checkpoint_restore|venom_qjs_replay_advance|venom_qjs_resume_state_ptr|venom_qjs_determinism_audit_ptr\n"
      << "wasm_commit_acceptance=venom_qjs_execution_commit|venom_qjs_execution_rollback|venom_qjs_host_receipts_ptr|venom_qjs_release_accept|venom_qjs_commit_audit_ptr\n"
      << "wasm_permission_gate=venom_qjs_capability_policy_ptr|venom_qjs_host_io_policy_ptr|venom_qjs_permission_seal|venom_qjs_policy_receipts_ptr|venom_qjs_release_gate\n"
      << "wasm_host_io_enforcement=venom_qjs_host_io_decision_ptr|venom_qjs_host_io_deny_trace_ptr|venom_qjs_capability_ledger_ptr|venom_qjs_policy_seal_audit_ptr|venom_qjs_runtime_denylist_ptr\n"
      << "wasm_upstream_intrinsics=venom_qjs_upstream_intrinsic_record_ptr|venom_qjs_upstream_property_descriptor_record_ptr|venom_qjs_upstream_prototype_chain_record_ptr|venom_qjs_upstream_call_frame_record_ptr|venom_qjs_upstream_intrinsic_semantics_score\n";
  return out.str();
}

} // namespace venom::quickjs
