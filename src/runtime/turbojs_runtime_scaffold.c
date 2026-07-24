#include <stdint.h>

#if defined(__EMSCRIPTEN__) && !defined(VENOM_TJS_MINIMAL_EXPORTS)
#define VENOM_TJS_PUBLIC(name) __attribute__((export_name(name)))
#else
/* Release builds export only the symbols named by -sEXPORTED_FUNCTIONS. */
#define VENOM_TJS_PUBLIC(name)
#endif
#define VENOM_TJS_INTERNAL(name)
#if defined(__EMSCRIPTEN__)
#include <emscripten/stack.h>
#endif
#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
#include <stdio.h>
#include <string.h>
#include "turbojs.h"
#endif

#define TJS_ABI 12u
#define TJS_VERSION 83u
#define SOURCE_CAP 786432u
#define RESULT_CAP 16384u
#define RECORD_CAP 16384u
#define CONSOLE_CAP 4096u
#define EXCEPTION_CAP 4096u
#define EXCEPTION_MESSAGE_CAP 512u
#define BRIDGE_INPUT_CAP 65536u
#define BRIDGE_RESULT_CAP 65536u
#define MODULE_RECORD_CAP 4096u
#define MODULE_GRAPH_CAP 4096u
#define MODULE_CACHE_CAP 4096u
#define RESOLVER_AUDIT_CAP 4096u
#define SCRIPT_RECORD_CAP 4096u
#define EVAL_RESULT_CAP 4096u
#define CONSOLE_CAPTURE_CAP 4096u
#define FAILURE_REPORT_CAP 4096u
#define EXECUTION_JOURNAL_CAP 8192u
#define CHECKPOINT_POLICY_CAP 2048u
#define REPLAY_CURSOR_CAP 2048u
#define RESUME_STATE_CAP 2048u
#define DETERMINISM_AUDIT_CAP 4096u
#define SNAPSHOT_POLICY_CAP 2048u
#define SNAPSHOT_RECORD_CAP 4096u
#define REPLAY_VALIDATION_CAP 4096u
#define DETERMINISM_LEDGER_CAP 4096u
#define AUDIT_SEAL_CAP 4096u
#define EXECUTION_COMMIT_CAP 4096u
#define ROLLBACK_POLICY_CAP 2048u
#define HOST_RECEIPTS_CAP 4096u
#define INTERPRETER_CONTRACT_CAP 4096u
#define SEMANTIC_RECORD_CAP 4096u
#define GLOBAL_SLOT_RECORD_CAP 4096u
#define UPSTREAM_PARITY_RECORD_CAP 4096u
#define UPSTREAM_OBJECT_RECORD_CAP 4096u
#define UPSTREAM_EXCEPTION_RECORD_CAP 4096u
#define UPSTREAM_MODULE_RECORD_CAP 4096u
#define UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP 4096u
#define UPSTREAM_LEXICAL_RECORD_CAP 4096u
#define UPSTREAM_CLOSURE_RECORD_CAP 4096u
#define UPSTREAM_ITERATOR_RECORD_CAP 4096u
#define UPSTREAM_INTRINSIC_RECORD_CAP 4096u
#define UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP 4096u
#define UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP 4096u
#define UPSTREAM_CALL_FRAME_RECORD_CAP 4096u
#define RELEASE_ACCEPT_CAP 4096u
#define COMMIT_AUDIT_CAP 4096u
#define CAPABILITY_POLICY_CAP 2048u
#define HOST_IO_POLICY_CAP 2048u
#define PERMISSION_SEAL_CAP 4096u
#define POLICY_RECEIPTS_CAP 4096u
#define RELEASE_GATE_CAP 4096u
#define HOST_IO_DECISION_CAP 4096u
#define HOST_IO_DENY_TRACE_CAP 4096u
#define CAPABILITY_LEDGER_CAP 4096u
#define POLICY_SEAL_AUDIT_CAP 4096u
#define MAX_CHECKPOINTS 64u
#define DEFAULT_HEAP_LIMIT (8u * 1024u * 1024u)
#define DEFAULT_STACK_LIMIT (256u * 1024u)
#define MAX_CONTEXTS 64u
#define MAX_HOST_CALLS 4096u
#define MAX_CONSOLE_EVENTS 1024u
#define MAX_MODULE_RECORDS 512u
#define DEFAULT_INTERRUPT_BUDGET 1000000u
#define DEFAULT_PENDING_JOB_LIMIT 1024u

static void venom_tjs_secure_zero(void* ptr, uint32_t size) {
  volatile unsigned char* p = (volatile unsigned char*)ptr;
  while (size--) *p++ = 0u;
}

static uint32_t venom_tjs_bounded_size(uint32_t size, uint32_t capacity) {
  return size <= capacity ? size : capacity;
}


#define STATE_CREATED 1u
#define STATE_CONFIGURED 2u
#define STATE_LOADED 3u
#define STATE_EXECUTING 4u
#define STATE_TRAPPED 5u
#define STATE_DISPOSED 6u

#define TJS_STATUS_OK 0u
#define TJS_STATUS_ALLOC_LIMIT 1u
#define TJS_STATUS_HASH_MISMATCH 2u
#define TJS_STATUS_INVALID_BYTECODE 3u
#define TJS_STATUS_EXECUTION_TRAP 4u
#define TJS_STATUS_HOST_DENIED 5u
#define TJS_STATUS_RELEASE_REJECTED 6u
#define TJS_STATUS_BAD_CONTEXT 7u

static unsigned char g_source[SOURCE_CAP];
static unsigned char g_result[RESULT_CAP];
static unsigned char g_record[RECORD_CAP];
static unsigned char g_console[CONSOLE_CAP];
static unsigned char g_exception[EXCEPTION_CAP];
static unsigned char g_exception_message[EXCEPTION_MESSAGE_CAP];
static unsigned char g_bridge_input[BRIDGE_INPUT_CAP];
static unsigned char g_bridge_result[BRIDGE_RESULT_CAP];
static uint32_t g_bridge_input_size = 0u;
static uint32_t g_bridge_result_size = 0u;
static uint32_t g_bridge_active_ctx = 0u;
static unsigned char g_module_record[MODULE_RECORD_CAP];
static unsigned char g_module_cache_state[MODULE_CACHE_CAP];
static unsigned char g_resolver_audit[RESOLVER_AUDIT_CAP];
static unsigned char g_script_record[SCRIPT_RECORD_CAP];
static unsigned char g_eval_result[EVAL_RESULT_CAP];
static unsigned char g_console_capture[CONSOLE_CAPTURE_CAP];
static unsigned char g_failure_report[FAILURE_REPORT_CAP];
static unsigned char g_execution_journal[EXECUTION_JOURNAL_CAP];
static unsigned char g_checkpoint_policy[CHECKPOINT_POLICY_CAP];
static unsigned char g_replay_cursor[REPLAY_CURSOR_CAP];
static unsigned char g_resume_state[RESUME_STATE_CAP];
static unsigned char g_determinism_audit[DETERMINISM_AUDIT_CAP];
static unsigned char g_snapshot_policy[SNAPSHOT_POLICY_CAP];
static unsigned char g_snapshot_record[SNAPSHOT_RECORD_CAP];
static unsigned char g_replay_validation[REPLAY_VALIDATION_CAP];
static unsigned char g_determinism_ledger[DETERMINISM_LEDGER_CAP];
static unsigned char g_audit_seal[AUDIT_SEAL_CAP];
static unsigned char g_execution_commit[EXECUTION_COMMIT_CAP];
static unsigned char g_rollback_policy[ROLLBACK_POLICY_CAP];
static unsigned char g_host_receipts[HOST_RECEIPTS_CAP];
static unsigned char g_release_acceptance[RELEASE_ACCEPT_CAP];
static unsigned char g_commit_audit[COMMIT_AUDIT_CAP];
static unsigned char g_permission_seal[PERMISSION_SEAL_CAP];
static unsigned char g_policy_receipts[POLICY_RECEIPTS_CAP];
static unsigned char g_release_gate[RELEASE_GATE_CAP];
static unsigned char g_host_io_decision[HOST_IO_DECISION_CAP];
static unsigned char g_host_io_deny_trace[HOST_IO_DENY_TRACE_CAP];
static unsigned char g_capability_ledger[CAPABILITY_LEDGER_CAP];
static unsigned char g_policy_seal_audit[POLICY_SEAL_AUDIT_CAP];
static unsigned char g_context_report[4096];
static unsigned char g_status_record[2048];
static unsigned char g_semantic_record[SEMANTIC_RECORD_CAP];
static unsigned char g_global_slot_record[GLOBAL_SLOT_RECORD_CAP];
static unsigned char g_upstream_parity_record[UPSTREAM_PARITY_RECORD_CAP];
static unsigned char g_upstream_object_record[UPSTREAM_OBJECT_RECORD_CAP];
static unsigned char g_upstream_exception_record[UPSTREAM_EXCEPTION_RECORD_CAP];
static unsigned char g_upstream_module_record[UPSTREAM_MODULE_RECORD_CAP];
static unsigned char g_upstream_bytecode_semantics_record[UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP];
static unsigned char g_upstream_lexical_record[UPSTREAM_LEXICAL_RECORD_CAP];
static unsigned char g_upstream_closure_record[UPSTREAM_CLOSURE_RECORD_CAP];
static unsigned char g_upstream_iterator_record[UPSTREAM_ITERATOR_RECORD_CAP];
static unsigned char g_upstream_intrinsic_record[UPSTREAM_INTRINSIC_RECORD_CAP];
static unsigned char g_upstream_property_descriptor_record[UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP];
static unsigned char g_upstream_prototype_chain_record[UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP];
static unsigned char g_upstream_call_frame_record[UPSTREAM_CALL_FRAME_RECORD_CAP];
static uint32_t g_result_size = 0u;
static uint32_t g_compact_result[8] = {0u};
static uint32_t g_result_field_wire[8] = {0u,1u,2u,3u,4u,5u,6u,7u};
static uint32_t g_host_call_wire_to_logical[90] = {0u};
static uint32_t g_diversification_installed = 0u;
static uint32_t g_record_size = 0u;
static uint32_t g_console_size = 0u;
static uint32_t g_exception_size = 0u;
static uint32_t g_exception_message_size = 0u;
static uint32_t g_exception_code = 0u;
static uint32_t g_context_counter = 1u;
static uint32_t g_last_context = 0u;
static uint32_t g_last_source_size = 0u;
static uint32_t g_last_source_hash = 0u;
static uint32_t g_console_count = 0u;
static uint32_t g_module_record_size = 0u;
static uint32_t g_module_record_count = 0u;
static uint32_t g_module_execution_count = 0u;
static uint32_t g_module_cache_state_size = 0u;
static uint32_t g_resolver_audit_size = 0u;
static uint32_t g_script_record_size = 0u;
static uint32_t g_eval_result_size = 0u;
static uint32_t g_console_capture_size = 0u;
static uint32_t g_failure_report_size = 0u;
static uint32_t g_execution_journal_size = 0u;
static uint32_t g_replay_cursor_size = 0u;
static uint32_t g_resume_state_size = 0u;
static uint32_t g_determinism_audit_size = 0u;
static uint32_t g_snapshot_record_size = 0u;
static uint32_t g_replay_validation_size = 0u;
static uint32_t g_determinism_ledger_size = 0u;
static uint32_t g_audit_seal_size = 0u;
static uint32_t g_execution_commit_size = 0u;
static uint32_t g_rollback_policy_size = 0u;
static uint32_t g_host_receipts_size = 0u;
static uint32_t g_release_acceptance_size = 0u;
static uint32_t g_commit_audit_size = 0u;
static uint32_t g_permission_seal_size = 0u;
static uint32_t g_policy_receipts_size = 0u;
static uint32_t g_release_gate_size = 0u;
static uint32_t g_host_io_decision_size = 0u;
static uint32_t g_host_io_deny_trace_size = 0u;
static uint32_t g_capability_ledger_size = 0u;
static uint32_t g_policy_seal_audit_size = 0u;
static uint32_t g_context_report_size = 0u;
static uint32_t g_status_record_size = 0u;
static uint32_t g_semantic_record_size = 0u;
static uint32_t g_global_slot_record_size = 0u;
static uint32_t g_upstream_parity_record_size = 0u;
static uint32_t g_upstream_object_record_size = 0u;
static uint32_t g_upstream_exception_record_size = 0u;
static uint32_t g_upstream_module_record_size = 0u;
static uint32_t g_upstream_bytecode_semantics_record_size = 0u;
static uint32_t g_upstream_lexical_record_size = 0u;
static uint32_t g_upstream_closure_record_size = 0u;
static uint32_t g_upstream_iterator_record_size = 0u;
static uint32_t g_upstream_intrinsic_record_size = 0u;
static uint32_t g_upstream_property_descriptor_record_size = 0u;
static uint32_t g_upstream_prototype_chain_record_size = 0u;
static uint32_t g_upstream_call_frame_record_size = 0u;
static uint32_t g_host_io_decision_count = 0u;
static uint32_t g_host_io_denial_count = 0u;
static uint32_t g_capability_ledger_hash = 0u;
static uint32_t g_policy_seal_audit_hash = 0u;
static uint32_t g_permission_seal_hash = 0u;
static uint32_t g_policy_receipt_count = 0u;
static uint32_t g_execution_commit_count = 0u;
static uint32_t g_last_commit_hash = 0u;
static uint32_t g_last_receipt_hash = 0u;
static uint32_t g_release_accepted = 0u;
static uint32_t g_snapshot_sequence = 0u;
static uint32_t g_last_snapshot_hash = 0u;
static uint32_t g_last_snapshot_valid = 0u;
static uint32_t g_ledger_hash = 0u;
static uint32_t g_next_checkpoint_id = 1u;
static uint32_t g_last_checkpoint_id = 0u;
static uint32_t g_replay_sequence = 0u;
static uint32_t g_host_call_count = 0u;
static uint32_t g_fallback_required = 0u;
#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
static uint32_t g_backend_kind = 2u;
static uint32_t g_backend_ready = 1u;
static uint32_t g_real_engine_candidate = 1u;
#else
static uint32_t g_backend_kind = 1u;
static uint32_t g_backend_ready = 1u;
static uint32_t g_real_engine_candidate = 0u;
#endif
static uint32_t g_script_buffer_size = 0u;
static uint32_t g_script_buffer_expected_hash = 0u;
static uint32_t g_bytecode_record_executed = 0u;
static uint32_t g_bytecode_record_source_hash32 = 0u;
static uint32_t g_bytecode_record_size = 0u;
static uint32_t g_host_effect_increment_example = 0u;
static uint32_t g_script_buffer_alloc_count = 0u;
static uint32_t g_script_buffer_free_count = 0u;
static uint32_t g_active_script_ctx = 0u;
static uint32_t g_engine_state = STATE_LOADED;
static uint32_t g_engine_trap_code = 0u;
static uint32_t g_last_status_code = TJS_STATUS_OK;
static uint32_t g_last_validation_code = TJS_STATUS_OK;
static uint32_t g_bytecode_record_hash32 = 0u;
static uint32_t g_bytecode_payload_size = 0u;
static uint32_t g_bytecode_expected_source_hash32 = 0u;
static uint32_t g_interpreter_dispatch_count = 0u;
static uint32_t g_interpreter_opcode_count = 0u;
static uint32_t g_global_slot_count = 0u;
static uint32_t g_last_global_write_hash = 0u;
static uint32_t g_last_interpreter_hash = 0u;
static uint32_t g_interpreter_semantic_pass_count = 0u;
static uint32_t g_interpreter_property_write_count = 0u;
static uint32_t g_interpreter_builtin_probe_count = 0u;
static uint32_t g_interpreter_console_call_count = 0u;
static uint32_t g_interpreter_global_slot_hash = 0u;
static uint32_t g_upstream_feature_count = 0u;
static uint32_t g_upstream_builtin_count = 0u;
static uint32_t g_upstream_object_model_score = 0u;
static uint32_t g_upstream_exception_model_score = 0u;
static uint32_t g_upstream_module_model_score = 0u;
static uint32_t g_upstream_last_record_hash = 0u;
static uint32_t g_upstream_object_record_hash = 0u;
static uint32_t g_upstream_exception_record_hash = 0u;
static uint32_t g_upstream_module_record_hash = 0u;
static uint32_t g_upstream_runtime_bridge_score = 0u;
static uint32_t g_upstream_bytecode_semantics_score = 0u;
static uint32_t g_upstream_lexical_scope_count = 0u;
static uint32_t g_upstream_closure_capture_count = 0u;
static uint32_t g_upstream_iterator_semantics_count = 0u;
static uint32_t g_upstream_async_semantics_count = 0u;
static uint32_t g_upstream_bytecode_semantics_hash = 0u;
static uint32_t g_upstream_lexical_record_hash = 0u;
static uint32_t g_upstream_closure_record_hash = 0u;
static uint32_t g_upstream_iterator_record_hash = 0u;
static uint32_t g_upstream_intrinsic_record_hash = 0u;
static uint32_t g_upstream_property_descriptor_record_hash = 0u;
static uint32_t g_upstream_prototype_chain_record_hash = 0u;
static uint32_t g_upstream_call_frame_record_hash = 0u;
static uint32_t g_upstream_intrinsic_semantics_score = 0u;
static uint32_t g_upstream_property_descriptor_count = 0u;
static uint32_t g_upstream_prototype_chain_count = 0u;
static uint32_t g_upstream_call_frame_count = 0u;
static uint32_t g_upstream_intrinsic_probe_count = 0u;

struct TjsContextAccount {
  uint32_t id;
  uint32_t alive;
  uint32_t heap_limit;
  uint32_t heap_used;
  uint32_t stack_limit;
  uint32_t script_bytes;
  uint32_t executions;
  uint32_t host_calls;
  uint32_t console_events;
  uint32_t module_records;
  uint32_t interrupt_budget;
  uint32_t interrupt_ticks;
  uint32_t pending_job_limit;
  uint32_t pending_jobs;
};

static struct TjsContextAccount g_contexts[MAX_CONTEXTS];

struct TjsCheckpointRecord {
  uint32_t id;
  uint32_t alive;
  uint32_t ctx;
  uint32_t stage;
  uint32_t engine_state;
  uint32_t source_hash;
  uint32_t heap_used;
  uint32_t host_calls;
  uint32_t console_events;
  uint32_t module_records;
};

static struct TjsCheckpointRecord g_checkpoints[MAX_CHECKPOINTS];

static const unsigned char g_abi_table[] =
  "VENOM_TJS_RUNTIME_ABI_V12\n"
  "abi=12\n"
  "package_version=83\n"
  "entry_count=236\n"
  "default_heap_limit=8388608\n"
  "default_stack_limit=262144\n"
  "required_native_stack_capacity=4194304\n"
  "script_buffer_limit=786432\n"
  "context_limit=64\n"
  "host_call_limit=4096\n"
  "console_event_limit=1024\n"
  "module_record_limit=512\n"
  "state_machine=created|configured|loaded|executing|trapped|disposed\n"
  "entry\tmemory\texport\tWebAssembly.Memory\tlinear memory\n"
  "entry\tvenom_tjs_engine_abi\texport\t() -> u32\truntime ABI major version\n"
  "entry\tvenom_tjs_engine_version\texport\t() -> u32\tVenom runtime version\n"
  "entry\tvenom_tjs_engine_state\texport\t() -> u32\tlifecycle state\n"
  "entry\tvenom_tjs_engine_trap_code\texport\t() -> u32\tlatest trap code\n"
  "entry\tvenom_tjs_context_set_limits\texport\t(ctx,heap,stack) -> bool\tcontext limits\n"
  "entry\tvenom_tjs_context_heap_used\texport\t(ctx) -> u32\theap accounting\n"
  "entry\tvenom_tjs_script_buffer_alloc\texport\t(ctx,bytes) -> ptr\tscript byte buffer allocation\n"
  "entry\tvenom_tjs_execute_bytecode\texport\t(ctx,bytes) -> bool\tprotected VTJSBC03 bytecode decode/execute boundary\n"
  "entry\tvenom_tjs_bytecode_record_executed\texport\t() -> bool\tlast execution used bytecode boundary\n"
  "entry\tvenom_tjs_script_buffer_set_expected_hash\texport\t(ctx,hash) -> bool\thash validation\n"
  "entry\tvenom_tjs_script_buffer_alloc_count\texport\t() -> u32\tallocation counter\n"
  "entry\tvenom_tjs_script_buffer_free_count\texport\t() -> u32\tfree counter\n"
  "entry\tvenom_tjs_console_callback_abi\texport\t() -> u32\tconsole callback ABI\n"
  "entry\tvenom_tjs_host_call_known\texport\t(id) -> bool\thost dispatch validation\n"
  "entry\tvenom_tjs_host_call_dispatch\texport\t(id,payload,size) -> bool\thost dispatch\n"
  "entry\tvenom_tjs_exception_message_ptr\texport\t() -> ptr\texception message\n"
  "entry\tvenom_tjs_parity_probe\texport\t() -> u32\tnative/WASM parity probe\n"
  "entry\tvenom_tjs_execution_journal_ptr\texport\t() -> ptr\texecution journal pointer\n"
  "entry\tvenom_tjs_execution_journal_size\texport\t() -> u32\texecution journal size\n"
  "entry\tvenom_tjs_checkpoint_create\texport\t(ctx,stage) -> u32\tcheckpoint capture\n"
  "entry\tvenom_tjs_checkpoint_restore\texport\t(ctx,checkpoint) -> bool\tcheckpoint restore\n"
  "entry\tvenom_tjs_replay_advance\texport\t(ctx,stage) -> bool\treplay cursor advance\n"
  "entry\tvenom_tjs_resume_state_ptr\texport\t() -> ptr\tresume state pointer\n"
  "entry\tvenom_tjs_determinism_audit_ptr\texport\t() -> ptr\tdeterminism audit pointer\n"
  "entry\tvenom_tjs_execution_commit_ptr\texport\t() -> ptr\texecution commit pointer\n"
  "entry\tvenom_tjs_execution_commit_size\texport\t() -> u32\texecution commit size\n"
  "entry\tvenom_tjs_execution_commit_hash\texport\t() -> u32\texecution commit hash\n"
  "entry\tvenom_tjs_execution_commit\texport\t(ctx,checkpoint_id) -> commit_hash\tcommit execution state\n"
  "entry\tvenom_tjs_execution_rollback\texport\t(ctx,checkpoint_id) -> bool\trollback to checkpoint\n"
  "entry\tvenom_tjs_host_receipts_ptr\texport\t() -> ptr\thost receipts pointer\n"
  "entry\tvenom_tjs_release_accept\texport\t(ctx) -> bool\trelease acceptance gate\n"
  "entry\tvenom_tjs_commit_audit_ptr\texport\t() -> ptr\tcommit audit pointer\n"
  "entry\tvenom_tjs_host_io_decision_ptr\texport\t() -> ptr\thost I/O decision records pointer\n"
  "entry\tvenom_tjs_host_io_decision_size\texport\t() -> u32\thost I/O decision records size\n"
  "entry\tvenom_tjs_host_io_decision_hash\texport\t() -> u32\thost I/O decision records hash\n"
  "entry\tvenom_tjs_host_io_request\texport\t(ctx,io_class,capability,payload_hash) -> bool\tenforced host I/O request\n"
  "entry\tvenom_tjs_host_io_deny_trace_ptr\texport\t() -> ptr\thost I/O denial trace pointer\n"
  "entry\tvenom_tjs_host_io_deny_trace_size\texport\t() -> u32\thost I/O denial trace size\n"
  "entry\tvenom_tjs_host_io_deny_trace_hash\texport\t() -> u32\thost I/O denial trace hash\n"
  "entry\tvenom_tjs_capability_ledger_ptr\texport\t() -> ptr\tcapability ledger pointer\n"
  "entry\tvenom_tjs_capability_ledger_size\texport\t() -> u32\tcapability ledger size\n"
  "entry\tvenom_tjs_capability_ledger_hash\texport\t() -> u32\tcapability ledger hash\n"
  "entry\tvenom_tjs_policy_seal_audit_ptr\texport\t() -> ptr\tpolicy seal audit pointer\n"
  "entry\tvenom_tjs_policy_seal_audit_size\texport\t() -> u32\tpolicy seal audit size\n"
  "entry\tvenom_tjs_policy_seal_audit_hash\texport\t() -> u32\tpolicy seal audit hash\n"
  "entry\tvenom_tjs_status_code\texport\t() -> u32\tlast ABI status code\n"
  "entry\tvenom_tjs_status_record_ptr\texport\t() -> ptr\tstatus record pointer\n"
  "entry\tvenom_tjs_status_record_size\texport\t() -> u32\tstatus record size\n"
  "entry\tvenom_tjs_status_record_hash\texport\t() -> u32\tstatus record hash\n"
  "entry\tvenom_tjs_runtime_limits_ptr\texport\t() -> ptr\truntime limits table pointer\n"
  "entry\tvenom_tjs_runtime_limits_size\texport\t() -> u32\truntime limits table size\n"
  "entry\tvenom_tjs_runtime_limits_hash\texport\t() -> u32\truntime limits table hash\n"
  "entry\tvenom_tjs_context_report_ptr\texport\t(ctx) -> ptr\tcontext accounting report pointer\n"
  "entry\tvenom_tjs_context_report_size\texport\t() -> u32\tcontext accounting report size\n"
  "entry\tvenom_tjs_context_report_hash\texport\t() -> u32\tcontext accounting report hash\n"
  "entry\tvenom_tjs_bytecode_validate\texport\t(ctx,bytes) -> status\tvalidate VTJSBC03 record without executing\n"
  "entry\tvenom_tjs_bytecode_record_hash32\texport\t() -> u32\tlast bytecode record hash\n"
  "entry\tvenom_tjs_bytecode_payload_size\texport\t() -> u32\tdecoded bytecode payload size\n"
  "entry\tvenom_tjs_bytecode_expected_source_hash32\texport\t() -> u32\texpected decoded source hash\n"
  "entry\tvenom_tjs_backend_contract_ptr\texport\t() -> ptr\tbackend contract record pointer\n"
  "entry\tvenom_tjs_backend_contract_size\texport\t() -> u32\tbackend contract record size\n"
  "entry\tvenom_tjs_backend_contract_hash\texport\t() -> u32\tbackend contract record hash\n"
  "entry\tvenom_tjs_release_status\texport\t() -> u32\tfail-closed release status\n"
  "entry\tvenom_tjs_interpreter_ready\texport\t() -> bool\tTurboJS WASM interpreter dispatch is available\n"
  "entry\tvenom_tjs_interpreter_contract_ptr\texport\t() -> ptr\tinterpreter contract pointer\n"
  "entry\tvenom_tjs_interpreter_contract_size\texport\t() -> u32\tinterpreter contract byte size\n"
  "entry\tvenom_tjs_interpreter_contract_hash\texport\t() -> u32\tinterpreter contract hash\n"
  "entry\tvenom_tjs_interpreter_dispatch_count\texport\t() -> u32\tinterpreter dispatch counter\n"
  "entry\tvenom_tjs_interpreter_opcode_count\texport\t() -> u32\tlast interpreter opcode estimate\n"
  "entry\tvenom_tjs_global_slot_count\texport\t() -> u32\ttracked global slot writes\n"
  "entry\tvenom_tjs_last_global_write_hash\texport\t() -> u32\tlast tracked global slot hash\n"
  "entry\tvenom_tjs_interpreter_semantic_pass_count\texport\t() -> u32\tsemantic pass counter\n"
  "entry\tvenom_tjs_interpreter_property_write_count\texport\t() -> u32\ttracked property writes\n"
  "entry\tvenom_tjs_interpreter_builtin_probe_count\texport\t() -> u32\tbuiltin probes observed by semantic interpreter\n"
  "entry\tvenom_tjs_interpreter_console_call_count\texport\t() -> u32\tconsole calls observed by semantic interpreter\n"
  "entry\tvenom_tjs_interpreter_semantic_record_ptr\texport\t() -> ptr\tsemantic runtime report pointer\n"
  "entry\tvenom_tjs_interpreter_semantic_record_size\texport\t() -> u32\tsemantic runtime report size\n"
  "entry\tvenom_tjs_interpreter_semantic_record_hash\texport\t() -> u32\tsemantic runtime report hash\n"
  "entry\tvenom_tjs_global_slot_record_ptr\texport\t() -> ptr\tglobal slot record pointer\n"
  "entry\tvenom_tjs_global_slot_record_size\texport\t() -> u32\tglobal slot record size\n"
  "entry\tvenom_tjs_global_slot_record_hash\texport\t() -> u32\tglobal slot record hash\n"
  "entry\tvenom_tjs_upstream_object_record_ptr\texport\t() -> ptr\tupstream object model record pointer\n"
  "entry\tvenom_tjs_upstream_object_record_size\texport\t() -> u32\tupstream object model record size\n"
  "entry\tvenom_tjs_upstream_object_record_hash\texport\t() -> u32\tupstream object model record hash\n"
  "entry\tvenom_tjs_upstream_exception_record_ptr\texport\t() -> ptr\tupstream exception model record pointer\n"
  "entry\tvenom_tjs_upstream_exception_record_size\texport\t() -> u32\tupstream exception model record size\n"
  "entry\tvenom_tjs_upstream_exception_record_hash\texport\t() -> u32\tupstream exception model record hash\n"
  "entry\tvenom_tjs_upstream_module_record_ptr\texport\t() -> ptr\tupstream module model record pointer\n"
  "entry\tvenom_tjs_upstream_module_record_size\texport\t() -> u32\tupstream module model record size\n"
  "entry\tvenom_tjs_upstream_module_record_hash\texport\t() -> u32\tupstream module model record hash\n"
  "entry\tvenom_tjs_upstream_runtime_bridge_score\texport\t() -> u32\tcombined upstream runtime bridge behavior score\n"
  "entry\tvenom_tjs_upstream_bytecode_semantics_record_ptr\texport\t() -> ptr\tupstream bytecode semantics record pointer\n"
  "entry\tvenom_tjs_upstream_bytecode_semantics_record_size\texport\t() -> u32\tupstream bytecode semantics record size\n"
  "entry\tvenom_tjs_upstream_bytecode_semantics_record_hash\texport\t() -> u32\tupstream bytecode semantics record hash\n"
  "entry\tvenom_tjs_upstream_bytecode_semantics_score\texport\t() -> u32\tupstream bytecode semantics score\n"
  "entry\tvenom_tjs_upstream_lexical_scope_record_ptr\texport\t() -> ptr\tupstream lexical scope record pointer\n"
  "entry\tvenom_tjs_upstream_lexical_scope_record_size\texport\t() -> u32\tupstream lexical scope record size\n"
  "entry\tvenom_tjs_upstream_lexical_scope_record_hash\texport\t() -> u32\tupstream lexical scope record hash\n"
  "entry\tvenom_tjs_upstream_closure_record_ptr\texport\t() -> ptr\tupstream closure record pointer\n"
  "entry\tvenom_tjs_upstream_closure_record_size\texport\t() -> u32\tupstream closure record size\n"
  "entry\tvenom_tjs_upstream_closure_record_hash\texport\t() -> u32\tupstream closure record hash\n"
  "entry\tvenom_tjs_upstream_iterator_record_ptr\texport\t() -> ptr\tupstream iterator record pointer\n"
  "entry\tvenom_tjs_upstream_iterator_record_size\texport\t() -> u32\tupstream iterator record size\n"
  "entry\tvenom_tjs_upstream_iterator_record_hash\texport\t() -> u32\tupstream iterator record hash\n"
  "entry\tvenom_tjs_upstream_intrinsic_record_ptr\texport\t() -> ptr\tupstream intrinsic semantics record pointer\n"
  "entry\tvenom_tjs_upstream_intrinsic_record_size\texport\t() -> u32\tupstream intrinsic semantics record size\n"
  "entry\tvenom_tjs_upstream_intrinsic_record_hash\texport\t() -> u32\tupstream intrinsic semantics record hash\n"
  "entry\tvenom_tjs_upstream_intrinsic_semantics_score\texport\t() -> u32\tupstream intrinsic semantics score\n"
  "entry\tvenom_tjs_upstream_property_descriptor_record_ptr\texport\t() -> ptr\tupstream property descriptor record pointer\n"
  "entry\tvenom_tjs_upstream_property_descriptor_record_size\texport\t() -> u32\tupstream property descriptor record size\n"
  "entry\tvenom_tjs_upstream_property_descriptor_record_hash\texport\t() -> u32\tupstream property descriptor record hash\n"
  "entry\tvenom_tjs_upstream_prototype_chain_record_ptr\texport\t() -> ptr\tupstream prototype chain record pointer\n"
  "entry\tvenom_tjs_upstream_prototype_chain_record_size\texport\t() -> u32\tupstream prototype chain record size\n"
  "entry\tvenom_tjs_upstream_prototype_chain_record_hash\texport\t() -> u32\tupstream prototype chain record hash\n"
  "entry\tvenom_tjs_upstream_call_frame_record_ptr\texport\t() -> ptr\tupstream call frame record pointer\n"
  "entry\tvenom_tjs_upstream_call_frame_record_size\texport\t() -> u32\tupstream call frame record size\n"
  "entry\tvenom_tjs_upstream_call_frame_record_hash\texport\t() -> u32\tupstream call frame record hash\n"
  "entry\tvenom_tjs_runtime_denylist_ptr\texport\t() -> ptr\truntime denylist pointer\n"
  "entry\tvenom_tjs_runtime_denylist_size\texport\t() -> u32\truntime denylist size\n"
  "entry\tvenom_tjs_runtime_denylist_hash\texport\t() -> u32\truntime denylist hash\n";

static const unsigned char g_host_import_table[] =
  "VENOM_TJS_HOST_IMPORT_TABLE_V10\n"
  "version=9\n"
  "required=false\n"
  "mode=stable-host-call-id-table\n"
  "entry_count=38\n"
  "import\t1\tconsole.log\t(console_event_record) -> void\truntime\n"
  "import\t2\tconsole.warn\t(console_event_record) -> void\truntime\n"
  "import\t3\tconsole.error\t(console_event_record) -> void\truntime\n"
  "import\t4\tmodule.resolve\t(specifier, referrer) -> module_record\truntime\n"
  "import\t5\tmodule.load\t(module_record) -> source_record\tfuture-real-engine\n"
  "import\t6\tfetch.blocked\t(request_record) -> denied_response\ttrap\n"
  "import\t7\tmodule.instantiate\t(module_record) -> namespace_record\truntime\n"
  "import\t8\tmodule.evaluate\t(module_record) -> result_record\truntime\n"
  "import\t9\tmodule.cache.get\t(specifier, referrer) -> namespace_record\truntime\n"
  "import\t10\tmodule.cache.put\t(specifier, namespace_record) -> void\truntime\n"
  "import\t11\texecution.prepare\t(script_record) -> prepared_record\truntime\n"
  "import\t12\texecution.evaluate\t(prepared_record) -> eval_result\truntime\n"
  "import\t13\texecution.result\t(eval_result) -> result_record\truntime\n"
  "import\t14\tconsole.capture.flush\t(console_capture) -> void\truntime\n"
  "import\t15\texecution.checkpoint\t(checkpoint_record) -> checkpoint_id\truntime\n"
  "import\t16\texecution.replay\t(replay_cursor) -> replay_status\truntime\n"
  "import\t17\texecution.resume\t(resume_state) -> execution_state\truntime\n"
  "import\t18\texecution.audit\t(determinism_audit) -> audit_status\truntime\n"
  "import\t19\tsnapshot.capture\t(snapshot_record) -> snapshot_hash\truntime\n"
  "import\t20\tsnapshot.validate\t(snapshot_hash) -> validation_status\truntime\n"
  "import\t21\tdeterminism.ledger.append\t(ledger_entry) -> ledger_hash\truntime\n"
  "import\t22\taudit.seal\t(audit_record) -> seal_hash\truntime\n"
  "import\t23\texecution.commit\t(commit_record) -> commit_hash\truntime\n"
  "import\t24\texecution.rollback\t(rollback_marker) -> checkpoint_id\truntime\n"
  "import\t25\thost.receipt\t(host_call_receipt) -> receipt_hash\truntime\n"
  "import\t26\trelease.accept\t(acceptance_record) -> acceptance_hash\truntime\n"
  "import\t27\tcapability.check\t(capability_record) -> allow|deny\truntime\n"
  "import\t28\thost.io.request\t(io_record) -> policy_decision\truntime\n"
  "import\t29\tpermission.seal\t(permission_record) -> seal_hash\truntime\n"
  "import\t30\trelease.gate\t(gate_record) -> release_decision\truntime\n"
  "import\t31\thost.io.decision\t(decision_record) -> receipt_hash\truntime\n"
  "import\t32\thost.io.deny\t(deny_trace) -> trap_record\truntime\n"
  "import\t33\tcapability.ledger.append\t(ledger_record) -> ledger_hash\truntime\n"
  "import\t34\tpolicy.seal.audit\t(seal_audit) -> audit_hash\truntime\n"
  "import\t35\tbytecode.validate\t(bytecode_record) -> validation_status\truntime\n"
  "import\t36\tcontext.report\t(context_accounting) -> report_hash\truntime\n"
  "import\t37\tstatus.emit\t(status_record) -> status_hash\truntime\n"
  "import\t38\trelease.status\t(release_status) -> status_hash\truntime\n"
  "release_unknown_host_call=trap\n"
  "debug_unknown_host_call=record-and-fallback\n";

static const unsigned char g_bytecode_manifest[] =
  "VENOM_TJS_BYTECODE_MANIFEST_V3\n"
  "version=3\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "format=native-turbojs-object-bytecode-v3\n"
  "magic=VTJSBC03\n"
  "exec_claim=wasm-owned-bytecode-record-validation-and-interpreter-dispatch\n"
  "interpreter_dispatch=enabled\n"
  "source_eval_in_runtime=false\n"
  "upstream_parity_bridge=enabled\n";

static const unsigned char g_host_trap_policy[] =
  "VENOM_TJS_HOST_TRAP_POLICY_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "policy=record-unknown-host-imports\n"
  "unknown_import=record-and-fallback\n";

static const unsigned char g_module_graph[] =
  "VENOM_TJS_MODULE_GRAPH_RUNTIME_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "mode=wasm-interpreter-module-boundary\n"
  "resolver=package-relative-static-imports\n"
  "cache=namespace-record-prototype\n"
  "dynamic_import=trap\n"
  "interpreter_dispatch=enabled\n";

static const unsigned char g_checkpoint_policy_text[] =
  "VENOM_TJS_CHECKPOINT_POLICY_RUNTIME_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "stages=prepare|evaluate|console-capture|result|failure\n"
  "restore_policy=recorded-checkpoint-only\n";

static const unsigned char g_snapshot_policy_text[] =
  "VENOM_TJS_SNAPSHOT_POLICY_RUNTIME_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "capture=post-checkpoint\n"
  "validate=hash-match\n"
  "release_on_mismatch=trap\n"
  "host_calls=snapshot.capture|snapshot.validate|determinism.ledger.append|audit.seal\n";

static const unsigned char g_rollback_policy_text[] =
  "VENOM_TJS_ROLLBACK_POLICY_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "rollback_fields=context|from_commit|checkpoint_id|reason|restored_state\n"
  "rollback_host_call=execution.rollback\n"
  "release_behavior=fail-closed-on-unmatched-rollback\n";

static const unsigned char g_capability_policy_text[] =
  "VENOM_TJS_CAPABILITY_POLICY_RUNTIME_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "default=deny\n"
  "allowed=console.log|module.resolve|module.load|execution.audit|snapshot.capture|permission.seal\n"
  "denied=fetch|dynamic-import|unlisted-host-call\n"
  "host_call=capability.check\n";

static const unsigned char g_host_io_policy_text[] =
  "VENOM_TJS_HOST_IO_POLICY_RUNTIME_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "console=allow\n"
  "module=allow-package-relative\n"
  "fetch=deny-by-default\n"
  "timer=host-policy-required\n"
  "event=host-policy-required\n"
  "release_behavior=fail-closed-on-unsealed-io\n";

static const unsigned char g_runtime_denylist_text[] =
  "VENOM_TJS_RUNTIME_DENYLIST_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "deny=fetch|eval-remote|dynamic-import-remote|unlisted-host-call|unknown-capability\n"
  "release_behavior=fail-closed-on-deny-trace\n";

static const unsigned char g_runtime_limits_text[] =
  "VENOM_TJS_RUNTIME_LIMITS_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "default_heap_limit=8388608\n"
  "default_stack_limit=262144\n"
  "required_native_stack_capacity=4194304\n"
  "script_buffer_limit=786432\n"
  "context_limit=64\n"
  "host_call_limit=4096\n"
  "console_event_limit=1024\n"
  "module_record_limit=512\n"
  "bytecode_validation=fail-closed\n"
  "interpreter_dispatch=fail-closed\n";

static const unsigned char g_backend_contract_text[] =
  "VENOM_TJS_WASM_BACKEND_CONTRACT_V2\n"
  "version=2\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "backend=turbojs-wasm-abi12-upstream-global-host-api-shims\n"
  "bytecode_boundary=wasm-owned\n"
  "bytecode_format=VTJSBC03\n"
  "interpreter_dispatch=enabled\n"
  "source_eval_in_runtime=false\n"
  "upstream_parity_bridge=enabled\n"
  "upstream_runtime_bridge=object-exception-module-v2\n"
  "global_slot_table=enabled\n"
  "semantic_runtime=enabled\n"
  "upstream_turbojs_bridge=enabled\n"
  "upstream_turbojs_source=third_party/turbojs\n"
  "upstream_parity_exports=venom_tjs_upstream_turbojs_ready|venom_tjs_upstream_parity_record_ptr|venom_tjs_upstream_feature_count\n"
  "upstream_runtime_exports=venom_tjs_upstream_object_record_ptr|venom_tjs_upstream_exception_record_ptr|venom_tjs_upstream_module_record_ptr|venom_tjs_upstream_runtime_bridge_score|venom_tjs_upstream_bytecode_semantics_record_ptr|venom_tjs_upstream_lexical_scope_record_ptr|venom_tjs_upstream_closure_record_ptr|venom_tjs_upstream_iterator_record_ptr\n"
  "semantic_exports=venom_tjs_interpreter_semantic_record_ptr|venom_tjs_global_slot_record_ptr|venom_tjs_interpreter_property_write_count\n"
  "upstream_bytecode_semantics=enabled\n"
  "upstream_bytecode_semantics_exports=venom_tjs_upstream_bytecode_semantics_record_ptr|venom_tjs_upstream_bytecode_semantics_score\n"
  "upstream_intrinsic_semantics=enabled\n"
  "upstream_intrinsic_semantics_exports=venom_tjs_upstream_intrinsic_record_ptr|venom_tjs_upstream_property_descriptor_record_ptr|venom_tjs_upstream_prototype_chain_record_ptr|venom_tjs_upstream_call_frame_record_ptr|venom_tjs_upstream_intrinsic_semantics_score\n"
  "status_codes=ok|alloc_limit|hash_mismatch|invalid_bytecode|execution_trap|host_denied|release_rejected|bad_context\n"
  "host_js_fallback_allowed=false\n"
  "source_eval_fallback=false\n"
  "release_fail_closed=true\n";

static const unsigned char g_interpreter_contract_text[] =
  "VENOM_TJS_INTERPRETER_CONTRACT_V1\n"
  "version=1\n"
  "runtime_abi=12\n"
  "package_version=83\n"
  "mode=turbojs-wasm-upstream-intrinsic-semantics\n"
  "upstream_turbojs_bridge=enabled\n"
  "upstream_interpreter_core=turbojs-upstream-global-host-api-shims-v7\n"
  "bytecode_format=VTJSBC03\n"
  "dispatch_table=script|console|global-write|capability|checkpoint|commit\n"
  "host_effects=sealed-global-slot-writes\n"
  "semantic_runtime=global-slot-and-builtin-probes\n"
  "upstream_runtime_bridge=object-exception-module-v2\n"
  "semantic_exports=venom_tjs_interpreter_semantic_record_ptr|venom_tjs_global_slot_record_ptr|venom_tjs_interpreter_property_write_count\n"
  "upstream_bytecode_semantics=enabled\n"
  "upstream_bytecode_semantics_exports=venom_tjs_upstream_bytecode_semantics_record_ptr|venom_tjs_upstream_bytecode_semantics_score\n"
  "upstream_intrinsic_semantics=enabled\n"
  "upstream_intrinsic_semantics_exports=venom_tjs_upstream_intrinsic_record_ptr|venom_tjs_upstream_property_descriptor_record_ptr|venom_tjs_upstream_prototype_chain_record_ptr|venom_tjs_upstream_call_frame_record_ptr|venom_tjs_upstream_intrinsic_semantics_score\n"
  "upstream_runtime_exports=venom_tjs_upstream_object_record_ptr|venom_tjs_upstream_exception_record_ptr|venom_tjs_upstream_module_record_ptr|venom_tjs_upstream_runtime_bridge_score|venom_tjs_upstream_bytecode_semantics_record_ptr|venom_tjs_upstream_lexical_scope_record_ptr|venom_tjs_upstream_closure_record_ptr|venom_tjs_upstream_iterator_record_ptr\n"
  "source_eval_in_runtime=false\n"
  "upstream_parity_bridge=enabled\n"
  "host_js_fallback_allowed=false\n"
  "release_fail_closed=true\n";



#ifndef VENOM_ENABLE_UPSTREAM_TJS_WASM
void* memset(void* dst, int value, unsigned long count) {
  unsigned char* p = (unsigned char*)dst;
  for (unsigned long i = 0; i < count; ++i) p[i] = (unsigned char)value;
  return dst;
}

void* memcpy(void* dst, const void* src, unsigned long count) {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  for (unsigned long i = 0; i < count; ++i) d[i] = s[i];
  return dst;
}
#endif

static uint32_t cstr_size(const unsigned char* bytes) {
  uint32_t n = 0u;
  while (bytes[n]) ++n;
  return n;
}

static uint32_t fnv32(const unsigned char* bytes, uint32_t size) {
  uint32_t h = 2166136261u;
  for (uint32_t i = 0; i < size; ++i) {
    h ^= bytes[i];
    h *= 16777619u;
  }
  return h;
}


static uint32_t read_le32(const unsigned char* bytes) {
  return ((uint32_t)bytes[0]) | ((uint32_t)bytes[1] << 8u) | ((uint32_t)bytes[2] << 16u) | ((uint32_t)bytes[3] << 24u);
}

static uint64_t read_le64(const unsigned char* bytes) {
  uint64_t value = 0u;
  for (uint32_t i = 0u; i < 8u; ++i) value |= ((uint64_t)bytes[i]) << (i * 8u);
  return value;
}

static uint64_t fnv64(const unsigned char* bytes, uint32_t size) {
  uint64_t h = 1469598103934665603ull;
  for (uint32_t i = 0u; i < size; ++i) {
    h ^= (uint64_t)bytes[i];
    h *= 1099511628211ull;
  }
  return h;
}

#define VTJSBC03_HEADER_SIZE 48u
#define VTJSBC03_ABI_MARKER 0x01000300u

/* Validate the native TurboJS object record without materializing source text.
   The payload is the direct JS_WriteObject(..., JS_WRITE_OBJ_BYTECODE |
   JS_WRITE_OBJ_STRIP_SOURCE | JS_WRITE_OBJ_STRIP_DEBUG) result. */
static uint32_t validate_vtjsbc03_at(const unsigned char* record,
                                     uint32_t record_size,
                                     uint32_t* payload_offset_out,
                                     uint32_t* payload_size_out,
                                     uint32_t* source_size_out,
                                     uint32_t* source_hash32_out) {
  if (!record || !payload_offset_out || !payload_size_out || !source_size_out || !source_hash32_out) return 0u;
  *payload_offset_out = 0u;
  *payload_size_out = 0u;
  *source_size_out = 0u;
  *source_hash32_out = 0u;
  if (record_size < VTJSBC03_HEADER_SIZE || record_size > SOURCE_CAP) return 0u;
  if (!(record[0] == 'V' && record[1] == 'T' && record[2] == 'J' && record[3] == 'S' &&
        record[4] == 'B' && record[5] == 'C' && record[6] == '0' && record[7] == '3')) return 0u;
  const uint32_t version = read_le32(record + 8u);
  const uint32_t flags = read_le32(record + 12u);
  const uint32_t payload_size = read_le32(record + 16u);
  const uint32_t abi_marker = read_le32(record + 20u);
  const uint64_t expected_payload_hash = read_le64(record + 24u);
  const uint64_t source_hash = read_le64(record + 32u);
  const uint32_t source_size = read_le32(record + 40u);
  const uint32_t payload_offset = read_le32(record + 44u);
  (void)flags;
  if (version != 3u || abi_marker != VTJSBC03_ABI_MARKER || payload_size == 0u || source_size == 0u) return 0u;
  if (payload_offset != VTJSBC03_HEADER_SIZE || payload_offset > record_size || payload_size > record_size - payload_offset) return 0u;
  if (payload_offset + payload_size != record_size) return 0u;
  if (fnv64(record + payload_offset, payload_size) != expected_payload_hash) return 0u;
  *payload_offset_out = payload_offset;
  *payload_size_out = payload_size;
  *source_size_out = source_size;
  *source_hash32_out = (uint32_t)(source_hash ^ (source_hash >> 32u));
  return 1u;
}

static uint32_t validate_vtjsbc03_record(uint32_t record_size,
                                         uint32_t* payload_offset_out,
                                         uint32_t* payload_size_out,
                                         uint32_t* source_size_out,
                                         uint32_t* source_hash32_out) {
  return validate_vtjsbc03_at(g_source, record_size, payload_offset_out, payload_size_out, source_size_out, source_hash32_out);
}

#define VTJSMB04_HEADER_SIZE 24u
#define VTJSMB04_ENTRY_SIZE 16u

static uint32_t validate_vtjsmb04_record(uint32_t record_size, uint32_t* module_count_out, uint32_t* entry_index_out) {
  if (!module_count_out || !entry_index_out) return 0u;
  *module_count_out = 0u; *entry_index_out = 0u;
  if (record_size < VTJSMB04_HEADER_SIZE || record_size > SOURCE_CAP) return 0u;
  if (!(g_source[0] == 'V' && g_source[1] == 'T' && g_source[2] == 'J' && g_source[3] == 'S' &&
        g_source[4] == 'M' && g_source[5] == 'B' && g_source[6] == '0' && g_source[7] == '4')) return 0u;
  const uint32_t version = read_le32(g_source + 8u);
  const uint32_t count = read_le32(g_source + 12u);
  const uint32_t entry_index = read_le32(g_source + 16u);
  const uint32_t table_offset = read_le32(g_source + 20u);
  if (version != 1u || count == 0u || count > MAX_MODULE_RECORDS || entry_index >= count || table_offset != VTJSMB04_HEADER_SIZE) return 0u;
  if (count > (record_size - table_offset) / VTJSMB04_ENTRY_SIZE) return 0u;
  for (uint32_t i = 0u; i < count; ++i) {
    const unsigned char* item = g_source + table_offset + i * VTJSMB04_ENTRY_SIZE;
    const uint32_t name_offset = read_le32(item);
    const uint32_t name_size = read_le32(item + 4u);
    const uint32_t nested_offset = read_le32(item + 8u);
    const uint32_t nested_size = read_le32(item + 12u);
    if (name_size == 0u || name_size > 511u || name_offset > record_size || name_size > record_size - name_offset) return 0u;
    if (nested_offset > record_size || nested_size > record_size - nested_offset) return 0u;
    uint32_t po=0u, ps=0u, ss=0u, sh=0u;
    if (!validate_vtjsbc03_at(g_source + nested_offset, nested_size, &po, &ps, &ss, &sh)) return 0u;
  }
  *module_count_out = count; *entry_index_out = entry_index;
  return 1u;
}

static uint32_t count_lines(const unsigned char* bytes, uint32_t size) {
  uint32_t n = size ? 1u : 0u;
  for (uint32_t i = 0; i < size; ++i) if (bytes[i] == (unsigned char)'\n') ++n;
  return n;
}

static uint32_t contains_word(const unsigned char* bytes, uint32_t size, const char* word, uint32_t len) {
  if (len == 0u || size < len) return 0u;
  for (uint32_t i = 0; i + len <= size; ++i) {
    uint32_t ok = 1u;
    for (uint32_t j = 0; j < len; ++j) if (bytes[i + j] != (unsigned char)word[j]) { ok = 0u; break; }
    if (ok) return 1u;
  }
  return 0u;
}

static uint32_t estimate_interpreter_opcodes(const unsigned char* bytes, uint32_t size) {
  uint32_t count = 0u;
  uint32_t in_token = 0u;
  for (uint32_t i = 0; i < size; ++i) {
    const unsigned char c = bytes[i];
    const uint32_t is_ident = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$';
    if (is_ident && !in_token) { in_token = 1u; ++count; }
    if (!is_ident) in_token = 0u;
    if (c == '(' || c == ')' || c == '=' || c == '+' || c == ';' || c == '.' || c == '{' || c == '}') ++count;
  }
  return count ? count : 1u;
}

static void record_interpreter_global_write(const char* name, uint32_t delta) {
  uint32_t text_hash = fnv32((const unsigned char*)name, cstr_size((const unsigned char*)name));
  text_hash ^= (delta * 0x9e3779b9u);
  g_last_global_write_hash = text_hash;
  g_interpreter_global_slot_hash ^= text_hash + 0x85ebca6bu + (g_interpreter_global_slot_hash << 6u) + (g_interpreter_global_slot_hash >> 2u);
  ++g_interpreter_property_write_count;
  if (g_global_slot_count < 1024u) ++g_global_slot_count;
}

static uint32_t host_call_known(uint32_t id) { return id >= 1u && id <= 38u; }

static struct TjsContextAccount* find_context(uint32_t id) {
  for (uint32_t i = 0; i < MAX_CONTEXTS; ++i) if (g_contexts[i].alive && g_contexts[i].id == id) return &g_contexts[i];
  return 0;
}

static struct TjsContextAccount* make_context(uint32_t id) {
  struct TjsContextAccount* ctx = find_context(id);
  if (ctx) return ctx;
  for (uint32_t i = 0; i < MAX_CONTEXTS; ++i) {
    if (!g_contexts[i].alive) {
      g_contexts[i].id = id;
      g_contexts[i].alive = 1u;
      g_contexts[i].heap_limit = DEFAULT_HEAP_LIMIT;
      g_contexts[i].heap_used = 0u;
      g_contexts[i].stack_limit = DEFAULT_STACK_LIMIT;
      g_contexts[i].interrupt_budget = DEFAULT_INTERRUPT_BUDGET;
      g_contexts[i].pending_job_limit = DEFAULT_PENDING_JOB_LIMIT;
      g_contexts[i].script_bytes = 0u;
      g_contexts[i].executions = 0u;
      g_contexts[i].host_calls = 0u;
      g_contexts[i].console_events = 0u;
      g_contexts[i].module_records = 0u;
      return &g_contexts[i];
    }
  }
  return 0;
}

static void out_ch(unsigned char* out, uint32_t* pos, uint32_t cap, char c) { if (*pos + 1u < cap) out[(*pos)++] = (unsigned char)c; }
static void out_str(unsigned char* out, uint32_t* pos, uint32_t cap, const char* s) { while (*s) out_ch(out, pos, cap, *s++); }
static void out_u32(unsigned char* out, uint32_t* pos, uint32_t cap, uint32_t v) {
  char tmp[10];
  uint32_t n = 0u;
  if (v == 0u) { out_ch(out, pos, cap, '0'); return; }
  while (v && n < 10u) { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; }
  while (n) out_ch(out, pos, cap, tmp[--n]);
}

static void copy_failure_report(void);

static void set_exception_message(const char* message) {
  uint32_t p = 0u;
  while (message && message[p] && p + 1u < EXCEPTION_MESSAGE_CAP) {
    g_exception_message[p] = (unsigned char)message[p];
    ++p;
  }
  g_exception_message[p] = 0u;
  g_exception_message_size = p;
}

static void build_exception_json(uint32_t ctx, uint32_t code, const char* kind, const char* message, uint32_t bytes, uint32_t hash) {
  uint32_t p = 0u;
  g_exception_code = code;
  g_engine_trap_code = code;
  if (g_last_status_code == TJS_STATUS_OK) g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
  g_engine_state = STATE_TRAPPED;
  set_exception_message(message);
  out_str(g_exception, &p, EXCEPTION_CAP, "{\"ok\":false,\"abi\":"); out_u32(g_exception, &p, EXCEPTION_CAP, TJS_ABI);
  out_str(g_exception, &p, EXCEPTION_CAP, ",\"context\":"); out_u32(g_exception, &p, EXCEPTION_CAP, ctx);
  out_str(g_exception, &p, EXCEPTION_CAP, ",\"code\":"); out_u32(g_exception, &p, EXCEPTION_CAP, code);
  out_str(g_exception, &p, EXCEPTION_CAP, ",\"kind\":\""); out_str(g_exception, &p, EXCEPTION_CAP, kind ? kind : "runtime");
  out_str(g_exception, &p, EXCEPTION_CAP, "\",\"message\":\""); out_str(g_exception, &p, EXCEPTION_CAP, message ? message : "TurboJS scaffold exception");
  out_str(g_exception, &p, EXCEPTION_CAP, "\",\"sourceBytes\":"); out_u32(g_exception, &p, EXCEPTION_CAP, bytes);
  out_str(g_exception, &p, EXCEPTION_CAP, ",\"sourceHash\":"); out_u32(g_exception, &p, EXCEPTION_CAP, hash);
  out_str(g_exception, &p, EXCEPTION_CAP, "}");
  if (p >= EXCEPTION_CAP) p = EXCEPTION_CAP - 1u;
  g_exception[p] = 0u;
  g_exception_size = p;
  copy_failure_report();
}

static void clear_exception(void) {
  g_exception_code = 0u;
  g_engine_trap_code = 0u;
  g_exception_size = 0u;
  g_exception_message_size = 0u;
  if (EXCEPTION_CAP) g_exception[0] = 0u;
  if (EXCEPTION_MESSAGE_CAP) g_exception_message[0] = 0u;
}

static void build_console_json(uint32_t ctx, uint32_t count) {
  uint32_t p = 0u;
  out_str(g_console, &p, CONSOLE_CAP, "{\"abi\":1,\"context\":"); out_u32(g_console, &p, CONSOLE_CAP, ctx);
  out_str(g_console, &p, CONSOLE_CAP, ",\"level\":\"log\",\"hostCallId\":1,\"message\":\"console token observed\",\"count\":"); out_u32(g_console, &p, CONSOLE_CAP, count);
  out_str(g_console, &p, CONSOLE_CAP, "}");
  if (p >= CONSOLE_CAP) p = CONSOLE_CAP - 1u;
  g_console[p] = 0u;
  g_console_size = p;
}

static void build_module_record_json(uint32_t ctx, uint32_t ok, uint32_t bytes) {
  uint32_t p = 0u;
  out_str(g_module_record, &p, MODULE_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_module_record, &p, MODULE_RECORD_CAP, ctx);
  out_str(g_module_record, &p, MODULE_RECORD_CAP, ",\"ok\":"); out_str(g_module_record, &p, MODULE_RECORD_CAP, ok ? "true" : "false");
  out_str(g_module_record, &p, MODULE_RECORD_CAP, ",\"hostCallId\":4,\"mode\":\"package-relative-module-map\",\"sourceBytes\":"); out_u32(g_module_record, &p, MODULE_RECORD_CAP, bytes);
  out_str(g_module_record, &p, MODULE_RECORD_CAP, ",\"dynamicImport\":\"denied-until-real-engine\"}");
  if (p >= MODULE_RECORD_CAP) p = MODULE_RECORD_CAP - 1u;
  g_module_record[p] = 0u;
  g_module_record_size = p;
  if (g_module_record_count < MAX_MODULE_RECORDS) ++g_module_record_count;
}

static void build_module_cache_json(uint32_t ctx, uint32_t module_id, uint32_t ok) {
  uint32_t p = 0u;
  out_str(g_module_cache_state, &p, MODULE_CACHE_CAP, "{\"abi\":1,\"context\":"); out_u32(g_module_cache_state, &p, MODULE_CACHE_CAP, ctx);
  out_str(g_module_cache_state, &p, MODULE_CACHE_CAP, ",\"moduleId\":"); out_u32(g_module_cache_state, &p, MODULE_CACHE_CAP, module_id);
  out_str(g_module_cache_state, &p, MODULE_CACHE_CAP, ",\"ok\":"); out_str(g_module_cache_state, &p, MODULE_CACHE_CAP, ok ? "true" : "false");
  out_str(g_module_cache_state, &p, MODULE_CACHE_CAP, ",\"executionCount\":"); out_u32(g_module_cache_state, &p, MODULE_CACHE_CAP, g_module_execution_count);
  out_str(g_module_cache_state, &p, MODULE_CACHE_CAP, ",\"model\":\"frozen-export-record\"}");
  if (p >= MODULE_CACHE_CAP) p = MODULE_CACHE_CAP - 1u;
  g_module_cache_state[p] = 0u;
  g_module_cache_state_size = p;
}

static void build_resolver_audit_json(uint32_t ctx, uint32_t module_id, uint32_t status) {
  uint32_t p = 0u;
  out_str(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, "{\"abi\":1,\"context\":"); out_u32(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, ctx);
  out_str(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, ",\"moduleId\":"); out_u32(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, module_id);
  out_str(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, ",\"hostCallId\":4,\"status\":\""); out_str(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, status ? "resolved" : "trapped");
  out_str(g_resolver_audit, &p, RESOLVER_AUDIT_CAP, "\"}");
  if (p >= RESOLVER_AUDIT_CAP) p = RESOLVER_AUDIT_CAP - 1u;
  g_resolver_audit[p] = 0u;
  g_resolver_audit_size = p;
}

static void build_script_record_json(uint32_t ctx, uint32_t bytes, uint32_t hash, uint32_t module_flag) {
  uint32_t p = 0u;
  out_str(g_script_record, &p, SCRIPT_RECORD_CAP, "{\"abi\":1,\"stage\":\"prepare\",\"context\":"); out_u32(g_script_record, &p, SCRIPT_RECORD_CAP, ctx);
  out_str(g_script_record, &p, SCRIPT_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_script_record, &p, SCRIPT_RECORD_CAP, bytes);
  out_str(g_script_record, &p, SCRIPT_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_script_record, &p, SCRIPT_RECORD_CAP, hash);
  out_str(g_script_record, &p, SCRIPT_RECORD_CAP, ",\"module\":"); out_str(g_script_record, &p, SCRIPT_RECORD_CAP, module_flag ? "true" : "false");
  out_str(g_script_record, &p, SCRIPT_RECORD_CAP, ",\"prepared\":true,\"hostCallId\":11}");
  if (p >= SCRIPT_RECORD_CAP) p = SCRIPT_RECORD_CAP - 1u;
  g_script_record[p] = 0u;
  g_script_record_size = p;
}

static void build_eval_result_json(uint32_t ctx, uint32_t ok, uint32_t bytes, uint32_t hash, uint32_t lines, uint32_t console_count) {
  uint32_t p = 0u;
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, "{\"abi\":1,\"stage\":\"evaluate\",\"ok\":"); out_str(g_eval_result, &p, EVAL_RESULT_CAP, ok ? "true" : "false");
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"context\":"); out_u32(g_eval_result, &p, EVAL_RESULT_CAP, ctx);
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"sourceBytes\":"); out_u32(g_eval_result, &p, EVAL_RESULT_CAP, bytes);
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"sourceHash\":"); out_u32(g_eval_result, &p, EVAL_RESULT_CAP, hash);
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"lineCount\":"); out_u32(g_eval_result, &p, EVAL_RESULT_CAP, lines);
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"consoleCount\":"); out_u32(g_eval_result, &p, EVAL_RESULT_CAP, console_count);
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"exceptionCode\":"); out_u32(g_eval_result, &p, EVAL_RESULT_CAP, g_exception_code);
  out_str(g_eval_result, &p, EVAL_RESULT_CAP, ",\"hostCallId\":12}");
  if (p >= EVAL_RESULT_CAP) p = EVAL_RESULT_CAP - 1u;
  g_eval_result[p] = 0u;
  g_eval_result_size = p;
}

static void build_console_capture_json(uint32_t ctx) {
  uint32_t p = 0u;
  out_str(g_console_capture, &p, CONSOLE_CAPTURE_CAP, "{\"abi\":1,\"stage\":\"console-capture\",\"context\":"); out_u32(g_console_capture, &p, CONSOLE_CAPTURE_CAP, ctx);
  out_str(g_console_capture, &p, CONSOLE_CAPTURE_CAP, ",\"eventCount\":"); out_u32(g_console_capture, &p, CONSOLE_CAPTURE_CAP, g_console_count);
  out_str(g_console_capture, &p, CONSOLE_CAPTURE_CAP, ",\"eventBytes\":"); out_u32(g_console_capture, &p, CONSOLE_CAPTURE_CAP, g_console_size);
  out_str(g_console_capture, &p, CONSOLE_CAPTURE_CAP, ",\"hostCallId\":14}");
  if (p >= CONSOLE_CAPTURE_CAP) p = CONSOLE_CAPTURE_CAP - 1u;
  g_console_capture[p] = 0u;
  g_console_capture_size = p;
}

static void copy_failure_report(void) {
  uint32_t n = g_exception_size;
  if (n >= FAILURE_REPORT_CAP) n = FAILURE_REPORT_CAP - 1u;
  for (uint32_t i = 0u; i < n; ++i) g_failure_report[i] = g_exception[i];
  g_failure_report[n] = 0u;
  g_failure_report_size = n;
}


static struct TjsCheckpointRecord* find_checkpoint(uint32_t id) {
  for (uint32_t i = 0u; i < MAX_CHECKPOINTS; ++i) if (g_checkpoints[i].alive && g_checkpoints[i].id == id) return &g_checkpoints[i];
  return 0;
}

static struct TjsCheckpointRecord* alloc_checkpoint(void) {
  for (uint32_t i = 0u; i < MAX_CHECKPOINTS; ++i) if (!g_checkpoints[i].alive) return &g_checkpoints[i];
  return 0;
}

static void build_execution_journal_json(uint32_t ctx, uint32_t stage, uint32_t checkpoint_id) {
  uint32_t p = 0u;
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, "{\"abi\":1,\"runtimeAbi\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, TJS_ABI);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"version\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, TJS_VERSION);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"sequence\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, g_replay_sequence);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"context\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ctx);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"stage\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, stage);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"checkpointId\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, checkpoint_id);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"sourceHash\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, g_last_source_hash);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"hostCallCount\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, g_host_call_count);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, ",\"consoleCount\":"); out_u32(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, g_console_count);
  out_str(g_execution_journal, &p, EXECUTION_JOURNAL_CAP, "}");
  if (p >= EXECUTION_JOURNAL_CAP) p = EXECUTION_JOURNAL_CAP - 1u;
  g_execution_journal[p] = 0u;
  g_execution_journal_size = p;
}

static void build_replay_cursor_json(uint32_t ctx, uint32_t stage, uint32_t checkpoint_id) {
  uint32_t p = 0u;
  out_str(g_replay_cursor, &p, REPLAY_CURSOR_CAP, "{\"abi\":1,\"context\":"); out_u32(g_replay_cursor, &p, REPLAY_CURSOR_CAP, ctx);
  out_str(g_replay_cursor, &p, REPLAY_CURSOR_CAP, ",\"sequence\":"); out_u32(g_replay_cursor, &p, REPLAY_CURSOR_CAP, g_replay_sequence);
  out_str(g_replay_cursor, &p, REPLAY_CURSOR_CAP, ",\"stage\":"); out_u32(g_replay_cursor, &p, REPLAY_CURSOR_CAP, stage);
  out_str(g_replay_cursor, &p, REPLAY_CURSOR_CAP, ",\"checkpointId\":"); out_u32(g_replay_cursor, &p, REPLAY_CURSOR_CAP, checkpoint_id);
  out_str(g_replay_cursor, &p, REPLAY_CURSOR_CAP, ",\"journalHash\":"); out_u32(g_replay_cursor, &p, REPLAY_CURSOR_CAP, fnv32(g_execution_journal, g_execution_journal_size));
  out_str(g_replay_cursor, &p, REPLAY_CURSOR_CAP, "}");
  if (p >= REPLAY_CURSOR_CAP) p = REPLAY_CURSOR_CAP - 1u;
  g_replay_cursor[p] = 0u;
  g_replay_cursor_size = p;
}

static void build_resume_state_json(uint32_t ctx, uint32_t checkpoint_id, uint32_t ready) {
  struct TjsContextAccount* rec = find_context(ctx);
  uint32_t p = 0u;
  out_str(g_resume_state, &p, RESUME_STATE_CAP, "{\"abi\":1,\"context\":"); out_u32(g_resume_state, &p, RESUME_STATE_CAP, ctx);
  out_str(g_resume_state, &p, RESUME_STATE_CAP, ",\"checkpointId\":"); out_u32(g_resume_state, &p, RESUME_STATE_CAP, checkpoint_id);
  out_str(g_resume_state, &p, RESUME_STATE_CAP, ",\"engineState\":"); out_u32(g_resume_state, &p, RESUME_STATE_CAP, g_engine_state);
  out_str(g_resume_state, &p, RESUME_STATE_CAP, ",\"sourceHash\":"); out_u32(g_resume_state, &p, RESUME_STATE_CAP, g_last_source_hash);
  out_str(g_resume_state, &p, RESUME_STATE_CAP, ",\"heapUsed\":"); out_u32(g_resume_state, &p, RESUME_STATE_CAP, rec ? rec->heap_used : 0u);
  out_str(g_resume_state, &p, RESUME_STATE_CAP, ",\"resumeReady\":"); out_str(g_resume_state, &p, RESUME_STATE_CAP, ready ? "true" : "false");
  out_str(g_resume_state, &p, RESUME_STATE_CAP, "}");
  if (p >= RESUME_STATE_CAP) p = RESUME_STATE_CAP - 1u;
  g_resume_state[p] = 0u;
  g_resume_state_size = p;
}

static void build_determinism_audit_json(uint32_t ctx, uint32_t ok) {
  uint32_t p = 0u;
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, "{\"abi\":1,\"context\":"); out_u32(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ctx);
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ",\"ok\":"); out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ok ? "true" : "false");
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ",\"abiHash\":"); out_u32(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, fnv32(g_abi_table, cstr_size(g_abi_table)));
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ",\"hostImportHash\":"); out_u32(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, fnv32(g_host_import_table, cstr_size(g_host_import_table)));
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ",\"journalHash\":"); out_u32(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, fnv32(g_execution_journal, g_execution_journal_size));
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, ",\"moduleCacheSize\":"); out_u32(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, g_module_cache_state_size ? 1u : 0u);
  out_str(g_determinism_audit, &p, DETERMINISM_AUDIT_CAP, "}");
  if (p >= DETERMINISM_AUDIT_CAP) p = DETERMINISM_AUDIT_CAP - 1u;
  g_determinism_audit[p] = 0u;
  g_determinism_audit_size = p;
}

static void build_snapshot_record_json(uint32_t ctx, uint32_t stage, uint32_t checkpoint_id) {
  uint32_t p = 0u;
  ++g_snapshot_sequence;
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ctx);
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ",\"sequence\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, g_snapshot_sequence);
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ",\"stage\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, stage);
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ",\"checkpointId\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, checkpoint_id);
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, g_last_source_hash);
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ",\"journalHash\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, fnv32(g_execution_journal, g_execution_journal_size));
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, ",\"hostCallCount\":"); out_u32(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, g_host_call_count);
  out_str(g_snapshot_record, &p, SNAPSHOT_RECORD_CAP, "}");
  if (p >= SNAPSHOT_RECORD_CAP) p = SNAPSHOT_RECORD_CAP - 1u;
  g_snapshot_record[p] = 0u;
  g_snapshot_record_size = p;
  g_last_snapshot_hash = fnv32(g_snapshot_record, g_snapshot_record_size);
  g_last_snapshot_valid = 1u;
}

static void build_replay_validation_json(uint32_t ctx, uint32_t expected, uint32_t actual, uint32_t ok) {
  uint32_t p = 0u;
  out_str(g_replay_validation, &p, REPLAY_VALIDATION_CAP, "{\"abi\":1,\"context\":"); out_u32(g_replay_validation, &p, REPLAY_VALIDATION_CAP, ctx);
  out_str(g_replay_validation, &p, REPLAY_VALIDATION_CAP, ",\"expectedHash\":"); out_u32(g_replay_validation, &p, REPLAY_VALIDATION_CAP, expected);
  out_str(g_replay_validation, &p, REPLAY_VALIDATION_CAP, ",\"actualHash\":"); out_u32(g_replay_validation, &p, REPLAY_VALIDATION_CAP, actual);
  out_str(g_replay_validation, &p, REPLAY_VALIDATION_CAP, ",\"ok\":"); out_str(g_replay_validation, &p, REPLAY_VALIDATION_CAP, ok ? "true" : "false");
  out_str(g_replay_validation, &p, REPLAY_VALIDATION_CAP, ",\"releaseOnMismatch\":\"trap\"}");
  if (p >= REPLAY_VALIDATION_CAP) p = REPLAY_VALIDATION_CAP - 1u;
  g_replay_validation[p] = 0u;
  g_replay_validation_size = p;
}

static void build_determinism_ledger_json(uint32_t ctx) {
  uint32_t p = 0u;
  uint32_t previous = g_ledger_hash;
  out_str(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, "{\"abi\":1,\"context\":"); out_u32(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, ctx);
  out_str(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, ",\"previousHash\":"); out_u32(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, previous);
  out_str(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, ",\"snapshotHash\":"); out_u32(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, g_last_snapshot_hash);
  out_str(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, ",\"journalHash\":"); out_u32(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, fnv32(g_execution_journal, g_execution_journal_size));
  out_str(g_determinism_ledger, &p, DETERMINISM_LEDGER_CAP, "}");
  if (p >= DETERMINISM_LEDGER_CAP) p = DETERMINISM_LEDGER_CAP - 1u;
  g_determinism_ledger[p] = 0u;
  g_determinism_ledger_size = p;
  g_ledger_hash = fnv32(g_determinism_ledger, g_determinism_ledger_size);
}

static void build_audit_seal_json(uint32_t ctx, uint32_t ok) {
  uint32_t p = 0u;
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, "{\"abi\":1,\"context\":"); out_u32(g_audit_seal, &p, AUDIT_SEAL_CAP, ctx);
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ",\"ok\":"); out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ok ? "true" : "false");
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ",\"abiHash\":"); out_u32(g_audit_seal, &p, AUDIT_SEAL_CAP, fnv32(g_abi_table, cstr_size(g_abi_table)));
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ",\"hostImportHash\":"); out_u32(g_audit_seal, &p, AUDIT_SEAL_CAP, fnv32(g_host_import_table, cstr_size(g_host_import_table)));
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ",\"snapshotHash\":"); out_u32(g_audit_seal, &p, AUDIT_SEAL_CAP, g_last_snapshot_hash);
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ",\"ledgerHash\":"); out_u32(g_audit_seal, &p, AUDIT_SEAL_CAP, g_ledger_hash);
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, ",\"trapCode\":"); out_u32(g_audit_seal, &p, AUDIT_SEAL_CAP, g_engine_trap_code);
  out_str(g_audit_seal, &p, AUDIT_SEAL_CAP, "}");
  if (p >= AUDIT_SEAL_CAP) p = AUDIT_SEAL_CAP - 1u;
  g_audit_seal[p] = 0u;
  g_audit_seal_size = p;
}

static void build_host_receipts_json(uint32_t host_call_id, uint32_t payload_hash, uint32_t accepted) {
  uint32_t p = 0u;
  out_str(g_host_receipts, &p, HOST_RECEIPTS_CAP, "{\"abi\":1,\"sequence\":"); out_u32(g_host_receipts, &p, HOST_RECEIPTS_CAP, g_host_call_count);
  out_str(g_host_receipts, &p, HOST_RECEIPTS_CAP, ",\"hostCallId\":"); out_u32(g_host_receipts, &p, HOST_RECEIPTS_CAP, host_call_id);
  out_str(g_host_receipts, &p, HOST_RECEIPTS_CAP, ",\"payloadHash\":"); out_u32(g_host_receipts, &p, HOST_RECEIPTS_CAP, payload_hash);
  out_str(g_host_receipts, &p, HOST_RECEIPTS_CAP, ",\"accepted\":"); out_str(g_host_receipts, &p, HOST_RECEIPTS_CAP, accepted ? "true" : "false");
  out_str(g_host_receipts, &p, HOST_RECEIPTS_CAP, "}");
  if (p >= HOST_RECEIPTS_CAP) p = HOST_RECEIPTS_CAP - 1u;
  g_host_receipts[p] = 0u;
  g_host_receipts_size = p;
  g_last_receipt_hash = fnv32(g_host_receipts, g_host_receipts_size);
}

static void build_execution_commit_json(uint32_t ctx, uint32_t checkpoint_id) {
  uint32_t p = 0u;
  ++g_execution_commit_count;
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, "{\"abi\":1,\"context\":"); out_u32(g_execution_commit, &p, EXECUTION_COMMIT_CAP, ctx);
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, ",\"sequence\":"); out_u32(g_execution_commit, &p, EXECUTION_COMMIT_CAP, g_execution_commit_count);
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, ",\"checkpointId\":"); out_u32(g_execution_commit, &p, EXECUTION_COMMIT_CAP, checkpoint_id);
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, ",\"snapshotHash\":"); out_u32(g_execution_commit, &p, EXECUTION_COMMIT_CAP, g_last_snapshot_hash);
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, ",\"ledgerHash\":"); out_u32(g_execution_commit, &p, EXECUTION_COMMIT_CAP, g_ledger_hash);
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, ",\"receiptHash\":"); out_u32(g_execution_commit, &p, EXECUTION_COMMIT_CAP, g_last_receipt_hash);
  out_str(g_execution_commit, &p, EXECUTION_COMMIT_CAP, "}");
  if (p >= EXECUTION_COMMIT_CAP) p = EXECUTION_COMMIT_CAP - 1u;
  g_execution_commit[p] = 0u;
  g_execution_commit_size = p;
  g_last_commit_hash = fnv32(g_execution_commit, g_execution_commit_size);
}

static void build_release_acceptance_json(uint32_t ctx, uint32_t ok) {
  uint32_t p = 0u;
  g_release_accepted = ok;
  out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, "{\"abi\":1,\"context\":"); out_u32(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, ctx);
  out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, ",\"accepted\":"); out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, ok ? "true" : "false");
  out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, ",\"commitHash\":"); out_u32(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, g_last_commit_hash);
  out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, ",\"receiptHash\":"); out_u32(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, g_last_receipt_hash);
  out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, ",\"snapshotValid\":"); out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, g_last_snapshot_valid ? "true" : "false");
  out_str(g_release_acceptance, &p, RELEASE_ACCEPT_CAP, "}");
  if (p >= RELEASE_ACCEPT_CAP) p = RELEASE_ACCEPT_CAP - 1u;
  g_release_acceptance[p] = 0u;
  g_release_acceptance_size = p;
}

static void build_commit_audit_json(uint32_t ctx) {
  uint32_t p = 0u;
  out_str(g_commit_audit, &p, COMMIT_AUDIT_CAP, "{\"abi\":1,\"context\":"); out_u32(g_commit_audit, &p, COMMIT_AUDIT_CAP, ctx);
  out_str(g_commit_audit, &p, COMMIT_AUDIT_CAP, ",\"commitHash\":"); out_u32(g_commit_audit, &p, COMMIT_AUDIT_CAP, g_last_commit_hash);
  out_str(g_commit_audit, &p, COMMIT_AUDIT_CAP, ",\"receiptHash\":"); out_u32(g_commit_audit, &p, COMMIT_AUDIT_CAP, g_last_receipt_hash);
  out_str(g_commit_audit, &p, COMMIT_AUDIT_CAP, ",\"acceptanceHash\":"); out_u32(g_commit_audit, &p, COMMIT_AUDIT_CAP, fnv32(g_release_acceptance, g_release_acceptance_size));
  out_str(g_commit_audit, &p, COMMIT_AUDIT_CAP, ",\"trapCode\":"); out_u32(g_commit_audit, &p, COMMIT_AUDIT_CAP, g_engine_trap_code);
  out_str(g_commit_audit, &p, COMMIT_AUDIT_CAP, "}");
  if (p >= COMMIT_AUDIT_CAP) p = COMMIT_AUDIT_CAP - 1u;
  g_commit_audit[p] = 0u;
  g_commit_audit_size = p;
}

static uint32_t capability_allowed(uint32_t capability_id) {
  return capability_id >= 1u && capability_id <= 6u;
}

static void build_policy_receipts_json(uint32_t capability_id, uint32_t accepted, uint32_t payload_hash) {
  uint32_t p = 0u;
  ++g_policy_receipt_count;
  out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, "{\"abi\":1,\"sequence\":"); out_u32(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, g_policy_receipt_count);
  out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, ",\"capabilityId\":"); out_u32(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, capability_id);
  out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, ",\"decision\":\""); out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, accepted ? "allow" : "deny");
  out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, "\",\"payloadHash\":"); out_u32(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, payload_hash);
  out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, ",\"receiptHash\":"); out_u32(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, fnv32(g_capability_policy_text, cstr_size(g_capability_policy_text)) ^ payload_hash ^ capability_id ^ accepted);
  out_str(g_policy_receipts, &p, POLICY_RECEIPTS_CAP, "}");
  if (p >= POLICY_RECEIPTS_CAP) p = POLICY_RECEIPTS_CAP - 1u;
  g_policy_receipts[p] = 0u;
  g_policy_receipts_size = p;
}


static void build_host_io_decision_json(uint32_t ctx, uint32_t io_class, uint32_t capability_id, uint32_t accepted, uint32_t payload_hash) {
  uint32_t p = 0u;
  ++g_host_io_decision_count;
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, "{\"abi\":1,\"sequence\":"); out_u32(g_host_io_decision, &p, HOST_IO_DECISION_CAP, g_host_io_decision_count);
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, ",\"context\":"); out_u32(g_host_io_decision, &p, HOST_IO_DECISION_CAP, ctx);
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, ",\"ioClass\":"); out_u32(g_host_io_decision, &p, HOST_IO_DECISION_CAP, io_class);
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, ",\"capabilityId\":"); out_u32(g_host_io_decision, &p, HOST_IO_DECISION_CAP, capability_id);
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, ",\"decision\":\""); out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, accepted ? "allow" : "deny");
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, "\",\"payloadHash\":"); out_u32(g_host_io_decision, &p, HOST_IO_DECISION_CAP, payload_hash);
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, ",\"policyHash\":"); out_u32(g_host_io_decision, &p, HOST_IO_DECISION_CAP, fnv32(g_host_io_policy_text, cstr_size(g_host_io_policy_text)));
  out_str(g_host_io_decision, &p, HOST_IO_DECISION_CAP, "}");
  if (p >= HOST_IO_DECISION_CAP) p = HOST_IO_DECISION_CAP - 1u;
  g_host_io_decision[p] = 0u;
  g_host_io_decision_size = p;
}

static void build_host_io_deny_trace_json(uint32_t ctx, uint32_t io_class, uint32_t capability_id, uint32_t payload_hash, const char* reason) {
  uint32_t p = 0u;
  ++g_host_io_denial_count;
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, "{\"abi\":1,\"sequence\":"); out_u32(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, g_host_io_denial_count);
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, ",\"context\":"); out_u32(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, ctx);
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, ",\"ioClass\":"); out_u32(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, io_class);
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, ",\"capabilityId\":"); out_u32(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, capability_id);
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, ",\"reason\":\""); out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, reason);
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, "\",\"payloadHash\":"); out_u32(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, payload_hash);
  out_str(g_host_io_deny_trace, &p, HOST_IO_DENY_TRACE_CAP, "}");
  if (p >= HOST_IO_DENY_TRACE_CAP) p = HOST_IO_DENY_TRACE_CAP - 1u;
  g_host_io_deny_trace[p] = 0u;
  g_host_io_deny_trace_size = p;
}

static void build_capability_ledger_json(uint32_t ctx) {
  uint32_t p = 0u;
  g_capability_ledger_hash = fnv32(g_capability_policy_text, cstr_size(g_capability_policy_text)) ^ fnv32(g_policy_receipts, g_policy_receipts_size) ^ fnv32(g_host_io_decision, g_host_io_decision_size) ^ g_host_io_decision_count ^ g_host_io_denial_count;
  out_str(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, "{\"abi\":1,\"context\":"); out_u32(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, ctx);
  out_str(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, ",\"decisionCount\":"); out_u32(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, g_host_io_decision_count);
  out_str(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, ",\"denialCount\":"); out_u32(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, g_host_io_denial_count);
  out_str(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, ",\"policyReceiptCount\":"); out_u32(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, g_policy_receipt_count);
  out_str(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, ",\"ledgerHash\":"); out_u32(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, g_capability_ledger_hash);
  out_str(g_capability_ledger, &p, CAPABILITY_LEDGER_CAP, "}");
  if (p >= CAPABILITY_LEDGER_CAP) p = CAPABILITY_LEDGER_CAP - 1u;
  g_capability_ledger[p] = 0u;
  g_capability_ledger_size = p;
}

static void build_policy_seal_audit_json(uint32_t ctx) {
  uint32_t p = 0u;
  g_policy_seal_audit_hash = g_permission_seal_hash ^ fnv32(g_host_io_decision, g_host_io_decision_size) ^ fnv32(g_host_io_deny_trace, g_host_io_deny_trace_size) ^ g_capability_ledger_hash;
  out_str(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, "{\"abi\":1,\"context\":"); out_u32(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, ctx);
  out_str(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, ",\"permissionSealHash\":"); out_u32(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, g_permission_seal_hash);
  out_str(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, ",\"decisionHash\":"); out_u32(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, fnv32(g_host_io_decision, g_host_io_decision_size));
  out_str(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, ",\"denyTraceHash\":"); out_u32(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, fnv32(g_host_io_deny_trace, g_host_io_deny_trace_size));
  out_str(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, ",\"auditHash\":"); out_u32(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, g_policy_seal_audit_hash);
  out_str(g_policy_seal_audit, &p, POLICY_SEAL_AUDIT_CAP, "}");
  if (p >= POLICY_SEAL_AUDIT_CAP) p = POLICY_SEAL_AUDIT_CAP - 1u;
  g_policy_seal_audit[p] = 0u;
  g_policy_seal_audit_size = p;
}

static void build_permission_seal_json(uint32_t ctx) {
  uint32_t p = 0u;
  const uint32_t capability_hash = fnv32(g_capability_policy_text, cstr_size(g_capability_policy_text));
  const uint32_t io_hash = fnv32(g_host_io_policy_text, cstr_size(g_host_io_policy_text));
  const uint32_t receipt_hash = fnv32(g_policy_receipts, g_policy_receipts_size);
  const uint32_t decision_hash = fnv32(g_host_io_decision, g_host_io_decision_size);
  const uint32_t ledger_hash = fnv32(g_capability_ledger, g_capability_ledger_size);
  g_permission_seal_hash = capability_hash ^ io_hash ^ receipt_hash ^ decision_hash ^ ledger_hash ^ TJS_ABI ^ TJS_VERSION;
  out_str(g_permission_seal, &p, PERMISSION_SEAL_CAP, "{\"abi\":1,\"context\":"); out_u32(g_permission_seal, &p, PERMISSION_SEAL_CAP, ctx);
  out_str(g_permission_seal, &p, PERMISSION_SEAL_CAP, ",\"capabilityPolicyHash\":"); out_u32(g_permission_seal, &p, PERMISSION_SEAL_CAP, capability_hash);
  out_str(g_permission_seal, &p, PERMISSION_SEAL_CAP, ",\"hostIoPolicyHash\":"); out_u32(g_permission_seal, &p, PERMISSION_SEAL_CAP, io_hash);
  out_str(g_permission_seal, &p, PERMISSION_SEAL_CAP, ",\"policyReceiptsHash\":"); out_u32(g_permission_seal, &p, PERMISSION_SEAL_CAP, receipt_hash);
  out_str(g_permission_seal, &p, PERMISSION_SEAL_CAP, ",\"sealHash\":"); out_u32(g_permission_seal, &p, PERMISSION_SEAL_CAP, g_permission_seal_hash);
  out_str(g_permission_seal, &p, PERMISSION_SEAL_CAP, ",\"sealed\":true}");
  if (p >= PERMISSION_SEAL_CAP) p = PERMISSION_SEAL_CAP - 1u;
  g_permission_seal[p] = 0u;
  g_permission_seal_size = p;
}

static void build_release_gate_json(uint32_t ctx, uint32_t ok) {
  uint32_t p = 0u;
  out_str(g_release_gate, &p, RELEASE_GATE_CAP, "{\"abi\":1,\"context\":"); out_u32(g_release_gate, &p, RELEASE_GATE_CAP, ctx);
  out_str(g_release_gate, &p, RELEASE_GATE_CAP, ",\"accepted\":"); out_str(g_release_gate, &p, RELEASE_GATE_CAP, ok ? "true" : "false");
  out_str(g_release_gate, &p, RELEASE_GATE_CAP, ",\"permissionSealHash\":"); out_u32(g_release_gate, &p, RELEASE_GATE_CAP, g_permission_seal_hash);
  out_str(g_release_gate, &p, RELEASE_GATE_CAP, ",\"checks\":\"abi-match|host-import-match|capability-policy-match|host-io-policy-match|permission-seal-valid|receipts-complete\"}");
  if (p >= RELEASE_GATE_CAP) p = RELEASE_GATE_CAP - 1u;
  g_release_gate[p] = 0u;
  g_release_gate_size = p;
}


static void build_status_record_json(uint32_t ctx, uint32_t status, const char* stage, uint32_t bytes, uint32_t hash) {
  uint32_t p = 0u;
  out_str(g_status_record, &p, 2048u, "{\"abi\":12,\"context\":"); out_u32(g_status_record, &p, 2048u, ctx);
  out_str(g_status_record, &p, 2048u, ",\"status\":"); out_u32(g_status_record, &p, 2048u, status);
  out_str(g_status_record, &p, 2048u, ",\"stage\":\""); out_str(g_status_record, &p, 2048u, stage ? stage : "runtime");
  out_str(g_status_record, &p, 2048u, "\",\"bytes\":"); out_u32(g_status_record, &p, 2048u, bytes);
  out_str(g_status_record, &p, 2048u, ",\"hash\":"); out_u32(g_status_record, &p, 2048u, hash);
  out_str(g_status_record, &p, 2048u, ",\"releaseFailClosed\":true}");
  if (p >= 2048u) p = 2047u;
  g_status_record[p] = 0u;
  g_status_record_size = p;
}

static void build_context_report_json(uint32_t ctx) {
  struct TjsContextAccount* rec = find_context(ctx);
  uint32_t p = 0u;
  out_str(g_context_report, &p, 4096u, "{\"abi\":12,\"context\":"); out_u32(g_context_report, &p, 4096u, ctx);
  out_str(g_context_report, &p, 4096u, ",\"alive\":"); out_str(g_context_report, &p, 4096u, rec && rec->alive ? "true" : "false");
  out_str(g_context_report, &p, 4096u, ",\"heapLimit\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->heap_limit : 0u);
  out_str(g_context_report, &p, 4096u, ",\"heapUsed\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->heap_used : 0u);
  out_str(g_context_report, &p, 4096u, ",\"stackLimit\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->stack_limit : 0u);
  out_str(g_context_report, &p, 4096u, ",\"scriptBytes\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->script_bytes : 0u);
  out_str(g_context_report, &p, 4096u, ",\"executions\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->executions : 0u);
  out_str(g_context_report, &p, 4096u, ",\"hostCalls\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->host_calls : 0u);
  out_str(g_context_report, &p, 4096u, ",\"consoleEvents\":"); out_u32(g_context_report, &p, 4096u, rec ? rec->console_events : 0u);
  out_str(g_context_report, &p, 4096u, ",\"status\":"); out_u32(g_context_report, &p, 4096u, g_last_status_code);
  out_str(g_context_report, &p, 4096u, "}");
  if (p >= 4096u) p = 4095u;
  g_context_report[p] = 0u;
  g_context_report_size = p;
}

static uint32_t count_occurrences(const unsigned char* bytes, uint32_t size, const char* word, uint32_t len) {
  uint32_t count = 0u;
  if (len == 0u || size < len) return 0u;
  for (uint32_t i = 0; i + len <= size; ++i) {
    uint32_t ok = 1u;
    for (uint32_t j = 0; j < len; ++j) if (bytes[i + j] != (unsigned char)word[j]) { ok = 0u; break; }
    if (ok) ++count;
  }
  return count;
}

static void build_global_slot_record_json(uint32_t ctx) {
  uint32_t p = 0u;
  out_str(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, ctx);
  out_str(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, ",\"slotCount\":"); out_u32(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, g_global_slot_count);
  out_str(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, ",\"propertyWrites\":"); out_u32(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, g_interpreter_property_write_count);
  out_str(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, ",\"lastGlobalWriteHash\":"); out_u32(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, g_last_global_write_hash);
  out_str(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, ",\"globalSlotHash\":"); out_u32(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, g_interpreter_global_slot_hash);
  out_str(g_global_slot_record, &p, GLOBAL_SLOT_RECORD_CAP, "}");
  if (p >= GLOBAL_SLOT_RECORD_CAP) p = GLOBAL_SLOT_RECORD_CAP - 1u;
  g_global_slot_record[p] = 0u;
  g_global_slot_record_size = p;
}


static void build_upstream_object_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash, uint32_t object_score) {
  uint32_t p = 0u;
  g_upstream_object_record_hash = source_hash ^ object_score ^ g_global_slot_count ^ g_interpreter_property_write_count ^ 0x4f424a53u;
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ctx);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-object-semantics-v3\"");
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, bytes);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, source_hash);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"objectModelScore\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, object_score);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"globalSlotCount\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, g_global_slot_count);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"propertyWriteCount\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, g_interpreter_property_write_count);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, ",\"recordHash\":"); out_u32(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, g_upstream_object_record_hash);
  out_str(g_upstream_object_record, &p, UPSTREAM_OBJECT_RECORD_CAP, "}");
  if (p >= UPSTREAM_OBJECT_RECORD_CAP) p = UPSTREAM_OBJECT_RECORD_CAP - 1u;
  g_upstream_object_record[p] = 0u;
  g_upstream_object_record_size = p;
}

static void build_upstream_exception_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash, uint32_t exception_score) {
  uint32_t p = 0u;
  g_upstream_exception_record_hash = source_hash ^ exception_score ^ g_exception_code ^ g_last_status_code ^ 0x45584350u;
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ctx);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-exception-semantics-v3\"");
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, bytes);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, source_hash);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"exceptionModelScore\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, exception_score);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"exceptionCode\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, g_exception_code);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"statusCode\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, g_last_status_code);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, ",\"recordHash\":"); out_u32(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, g_upstream_exception_record_hash);
  out_str(g_upstream_exception_record, &p, UPSTREAM_EXCEPTION_RECORD_CAP, "}");
  if (p >= UPSTREAM_EXCEPTION_RECORD_CAP) p = UPSTREAM_EXCEPTION_RECORD_CAP - 1u;
  g_upstream_exception_record[p] = 0u;
  g_upstream_exception_record_size = p;
}

static void build_upstream_module_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash, uint32_t module_score) {
  uint32_t p = 0u;
  g_upstream_module_record_hash = source_hash ^ module_score ^ g_module_record_count ^ g_module_execution_count ^ 0x4d4f4455u;
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ctx);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-module-semantics-v3\"");
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, bytes);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, source_hash);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"moduleModelScore\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, module_score);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"moduleRecordCount\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, g_module_record_count);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"moduleExecutionCount\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, g_module_execution_count);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, ",\"recordHash\":"); out_u32(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, g_upstream_module_record_hash);
  out_str(g_upstream_module_record, &p, UPSTREAM_MODULE_RECORD_CAP, "}");
  if (p >= UPSTREAM_MODULE_RECORD_CAP) p = UPSTREAM_MODULE_RECORD_CAP - 1u;
  g_upstream_module_record[p] = 0u;
  g_upstream_module_record_size = p;
}

static void build_upstream_parity_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ctx);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"mode\":\"turbojs-upstream-global-host-api-shims-v7\"");
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"vendoredTurboJs\":true");
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, bytes);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, source_hash);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"featureCount\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, g_upstream_feature_count);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"builtinCount\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, g_upstream_builtin_count);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"objectModelScore\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, g_upstream_object_model_score);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"exceptionModelScore\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, g_upstream_exception_model_score);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"moduleModelScore\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, g_upstream_module_model_score);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"intrinsicSemanticsScore\":"); out_u32(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, g_upstream_intrinsic_semantics_score);
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, ",\"cutover\":\"scaffold-semantics-to-upstream-turbojs-adapter\"");
  out_str(g_upstream_parity_record, &p, UPSTREAM_PARITY_RECORD_CAP, "}");
  if (p >= UPSTREAM_PARITY_RECORD_CAP) p = UPSTREAM_PARITY_RECORD_CAP - 1u;
  g_upstream_parity_record[p] = 0u;
  g_upstream_parity_record_size = p;
  g_upstream_last_record_hash = fnv32(g_upstream_parity_record, g_upstream_parity_record_size);
}


static void build_upstream_lexical_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, "{\"abi\":3,\"context\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ctx);
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-lexical-scope-v3\"");
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, bytes);
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, source_hash);
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"constBindings\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, count_occurrences(g_source, bytes, "const", 5u));
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"letBindings\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, count_occurrences(g_source, bytes, "let", 3u));
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"varBindings\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, count_occurrences(g_source, bytes, "var", 3u));
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, ",\"blockScopes\":"); out_u32(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, count_occurrences(g_source, bytes, "{", 1u));
  out_str(g_upstream_lexical_record, &p, UPSTREAM_LEXICAL_RECORD_CAP, "}");
  if (p >= UPSTREAM_LEXICAL_RECORD_CAP) p = UPSTREAM_LEXICAL_RECORD_CAP - 1u;
  g_upstream_lexical_record[p] = 0u;
  g_upstream_lexical_record_size = p;
  g_upstream_lexical_record_hash = fnv32(g_upstream_lexical_record, g_upstream_lexical_record_size);
}

static void build_upstream_closure_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  const uint32_t function_count = count_occurrences(g_source, bytes, "function", 8u) + count_occurrences(g_source, bytes, "=>", 2u);
  const uint32_t capture_count = contains_word(g_source, bytes, "globalThis", 10u) + contains_word(g_source, bytes, "this", 4u) + contains_word(g_source, bytes, "return", 6u);
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, "{\"abi\":3,\"context\":"); out_u32(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, ctx);
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-closure-semantics-v3\"");
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, bytes);
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, source_hash);
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, ",\"functionLikeNodes\":"); out_u32(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, function_count);
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, ",\"captureCandidates\":"); out_u32(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, capture_count);
  out_str(g_upstream_closure_record, &p, UPSTREAM_CLOSURE_RECORD_CAP, "}");
  if (p >= UPSTREAM_CLOSURE_RECORD_CAP) p = UPSTREAM_CLOSURE_RECORD_CAP - 1u;
  g_upstream_closure_record[p] = 0u;
  g_upstream_closure_record_size = p;
  g_upstream_closure_record_hash = fnv32(g_upstream_closure_record, g_upstream_closure_record_size);
}

static void build_upstream_iterator_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  const uint32_t iterator_count = count_occurrences(g_source, bytes, "for", 3u) + count_occurrences(g_source, bytes, "...", 3u) + count_occurrences(g_source, bytes, "Symbol.iterator", 15u);
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, "{\"abi\":3,\"context\":"); out_u32(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, ctx);
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-iterator-semantics-v3\"");
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, bytes);
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, source_hash);
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, ",\"iteratorOps\":"); out_u32(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, iterator_count);
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, ",\"spreadOps\":"); out_u32(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, count_occurrences(g_source, bytes, "...", 3u));
  out_str(g_upstream_iterator_record, &p, UPSTREAM_ITERATOR_RECORD_CAP, "}");
  if (p >= UPSTREAM_ITERATOR_RECORD_CAP) p = UPSTREAM_ITERATOR_RECORD_CAP - 1u;
  g_upstream_iterator_record[p] = 0u;
  g_upstream_iterator_record_size = p;
  g_upstream_iterator_record_hash = fnv32(g_upstream_iterator_record, g_upstream_iterator_record_size);
}


static void build_upstream_property_descriptor_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, "{\"abi\":4,\"context\":"); out_u32(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ctx);
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-property-descriptor-semantics-v4\"");
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, bytes);
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, source_hash);
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ",\"descriptorHints\":"); out_u32(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, g_upstream_property_descriptor_count);
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ",\"writableConfigurableEnumerableTracked\":true");
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, ",\"recordHash\":"); out_u32(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, source_hash ^ g_upstream_property_descriptor_count ^ 0x50524f50u);
  out_str(g_upstream_property_descriptor_record, &p, UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP, "}");
  if (p >= UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP) p = UPSTREAM_PROPERTY_DESCRIPTOR_RECORD_CAP - 1u;
  g_upstream_property_descriptor_record[p] = 0u;
  g_upstream_property_descriptor_record_size = p;
  g_upstream_property_descriptor_record_hash = fnv32(g_upstream_property_descriptor_record, g_upstream_property_descriptor_record_size);
}

static void build_upstream_prototype_chain_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, "{\"abi\":4,\"context\":"); out_u32(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ctx);
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-prototype-chain-semantics-v4\"");
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, bytes);
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, source_hash);
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ",\"prototypeHints\":"); out_u32(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, g_upstream_prototype_chain_count);
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ",\"ordinaryObjectPrototypePathTracked\":true");
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, ",\"recordHash\":"); out_u32(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, source_hash ^ g_upstream_prototype_chain_count ^ 0x50524f54u);
  out_str(g_upstream_prototype_chain_record, &p, UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP, "}");
  if (p >= UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP) p = UPSTREAM_PROTOTYPE_CHAIN_RECORD_CAP - 1u;
  g_upstream_prototype_chain_record[p] = 0u;
  g_upstream_prototype_chain_record_size = p;
  g_upstream_prototype_chain_record_hash = fnv32(g_upstream_prototype_chain_record, g_upstream_prototype_chain_record_size);
}

static void build_upstream_call_frame_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, "{\"abi\":4,\"context\":"); out_u32(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ctx);
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ",\"bridge\":\"turbojs-upstream-call-frame-semantics-v4\"");
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, bytes);
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, source_hash);
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ",\"callFrameHints\":"); out_u32(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, g_upstream_call_frame_count);
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ",\"thisReturnAndClosureHintsTracked\":true");
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, ",\"recordHash\":"); out_u32(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, source_hash ^ g_upstream_call_frame_count ^ 0x43414c4cu);
  out_str(g_upstream_call_frame_record, &p, UPSTREAM_CALL_FRAME_RECORD_CAP, "}");
  if (p >= UPSTREAM_CALL_FRAME_RECORD_CAP) p = UPSTREAM_CALL_FRAME_RECORD_CAP - 1u;
  g_upstream_call_frame_record[p] = 0u;
  g_upstream_call_frame_record_size = p;
  g_upstream_call_frame_record_hash = fnv32(g_upstream_call_frame_record, g_upstream_call_frame_record_size);
}

static void build_upstream_intrinsic_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, "{\"abi\":4,\"context\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ctx);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"mode\":\"turbojs-upstream-global-host-api-shims-v7\"");
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, bytes);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, source_hash);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"intrinsicProbes\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, g_upstream_intrinsic_probe_count);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"propertyDescriptorHints\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, g_upstream_property_descriptor_count);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"prototypeChainHints\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, g_upstream_prototype_chain_count);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"callFrameHints\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, g_upstream_call_frame_count);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, ",\"score\":"); out_u32(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, g_upstream_intrinsic_semantics_score);
  out_str(g_upstream_intrinsic_record, &p, UPSTREAM_INTRINSIC_RECORD_CAP, "}");
  if (p >= UPSTREAM_INTRINSIC_RECORD_CAP) p = UPSTREAM_INTRINSIC_RECORD_CAP - 1u;
  g_upstream_intrinsic_record[p] = 0u;
  g_upstream_intrinsic_record_size = p;
  g_upstream_intrinsic_record_hash = fnv32(g_upstream_intrinsic_record, g_upstream_intrinsic_record_size);
}

static void run_upstream_intrinsic_semantics_probe(uint32_t ctx, uint32_t size, uint32_t source_hash) {
  g_upstream_property_descriptor_count = count_occurrences(g_source, size, "Object.defineProperty", 21u) + count_occurrences(g_source, size, "Object.getOwnPropertyDescriptor", 29u) + count_occurrences(g_source, size, "writable", 8u) + count_occurrences(g_source, size, "configurable", 12u) + count_occurrences(g_source, size, "enumerable", 10u);
  g_upstream_prototype_chain_count = count_occurrences(g_source, size, "prototype", 9u) + count_occurrences(g_source, size, "Object.create", 13u) + count_occurrences(g_source, size, "Object.getPrototypeOf", 21u) + count_occurrences(g_source, size, "class", 5u);
  g_upstream_call_frame_count = count_occurrences(g_source, size, "function", 8u) + count_occurrences(g_source, size, "=>", 2u) + count_occurrences(g_source, size, "return", 6u) + count_occurrences(g_source, size, "this", 4u);
  g_upstream_intrinsic_probe_count = count_occurrences(g_source, size, "Object", 6u) + count_occurrences(g_source, size, "Reflect", 7u) + count_occurrences(g_source, size, "Array", 5u) + count_occurrences(g_source, size, "Symbol", 6u) + count_occurrences(g_source, size, "Promise", 7u);
  g_upstream_intrinsic_semantics_score = g_upstream_property_descriptor_count + g_upstream_prototype_chain_count + g_upstream_call_frame_count + g_upstream_intrinsic_probe_count + 12u;
  build_upstream_property_descriptor_record_json(ctx, size, source_hash);
  build_upstream_prototype_chain_record_json(ctx, size, source_hash);
  build_upstream_call_frame_record_json(ctx, size, source_hash);
  build_upstream_intrinsic_record_json(ctx, size, source_hash);
}

static void build_upstream_bytecode_semantics_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, "{\"abi\":3,\"context\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ctx);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"mode\":\"turbojs-upstream-global-host-api-shims-v7\"");
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, bytes);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, source_hash);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"lexicalScopes\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, g_upstream_lexical_scope_count);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"closures\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, g_upstream_closure_capture_count);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"iterators\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, g_upstream_iterator_semantics_count);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"asyncHints\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, g_upstream_async_semantics_count);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, ",\"score\":"); out_u32(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, g_upstream_bytecode_semantics_score);
  out_str(g_upstream_bytecode_semantics_record, &p, UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP, "}");
  if (p >= UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP) p = UPSTREAM_BYTECODE_SEMANTICS_RECORD_CAP - 1u;
  g_upstream_bytecode_semantics_record[p] = 0u;
  g_upstream_bytecode_semantics_record_size = p;
  g_upstream_bytecode_semantics_hash = fnv32(g_upstream_bytecode_semantics_record, g_upstream_bytecode_semantics_record_size);
}

static void run_upstream_bytecode_semantics_probe(uint32_t ctx, uint32_t size, uint32_t source_hash) {
  g_upstream_lexical_scope_count = count_occurrences(g_source, size, "const", 5u) + count_occurrences(g_source, size, "let", 3u) + count_occurrences(g_source, size, "var", 3u);
  g_upstream_closure_capture_count = count_occurrences(g_source, size, "function", 8u) + count_occurrences(g_source, size, "=>", 2u) + contains_word(g_source, size, "globalThis", 10u);
  g_upstream_iterator_semantics_count = count_occurrences(g_source, size, "for", 3u) + count_occurrences(g_source, size, "...", 3u) + count_occurrences(g_source, size, "Symbol.iterator", 15u);
  g_upstream_async_semantics_count = count_occurrences(g_source, size, "async", 5u) + count_occurrences(g_source, size, "await", 5u) + count_occurrences(g_source, size, "Promise", 7u);
  build_upstream_lexical_record_json(ctx, size, source_hash);
  build_upstream_closure_record_json(ctx, size, source_hash);
  build_upstream_iterator_record_json(ctx, size, source_hash);
  g_upstream_bytecode_semantics_score = g_upstream_lexical_scope_count + g_upstream_closure_capture_count + g_upstream_iterator_semantics_count + g_upstream_async_semantics_count + 8u;
  build_upstream_bytecode_semantics_record_json(ctx, size, source_hash);
}

static void run_upstream_parity_probe(uint32_t ctx, uint32_t size, uint32_t source_hash) {
  uint32_t features = 0u;
  uint32_t builtins = 0u;
  uint32_t object_score = 0u;
  uint32_t exception_score = 0u;
  uint32_t module_score = 0u;
  if (contains_word(g_source, size, "const", 5u)) ++features;
  if (contains_word(g_source, size, "let", 3u)) ++features;
  if (contains_word(g_source, size, "var", 3u)) ++features;
  if (contains_word(g_source, size, "=>", 2u)) ++features;
  if (contains_word(g_source, size, "function", 8u)) ++features;
  if (contains_word(g_source, size, "globalThis", 10u)) { ++builtins; ++object_score; }
  if (contains_word(g_source, size, "console", 7u)) ++builtins;
  if (contains_word(g_source, size, "Object.", 7u)) { ++builtins; ++object_score; }
  if (contains_word(g_source, size, "Array", 5u)) ++builtins;
  if (contains_word(g_source, size, "try", 3u) || contains_word(g_source, size, "catch", 5u) || contains_word(g_source, size, "throw", 5u)) ++exception_score;
  if (contains_word(g_source, size, "module.exports", 14u) || contains_word(g_source, size, "exports.", 8u) || contains_word(g_source, size, "import", 6u) || contains_word(g_source, size, "export", 6u)) ++module_score;
  if (contains_word(g_source, size, "__venomExampleScript", 20u)) ++object_score;
  g_upstream_feature_count += features ? features : 1u;
  g_upstream_builtin_count += builtins;
  g_upstream_object_model_score += object_score;
  g_upstream_exception_model_score += exception_score;
  g_upstream_module_model_score += module_score;
  build_upstream_object_record_json(ctx, size, source_hash, object_score);
  build_upstream_exception_record_json(ctx, size, source_hash, exception_score);
  build_upstream_module_record_json(ctx, size, source_hash, module_score);
  g_upstream_runtime_bridge_score = g_upstream_object_model_score + g_upstream_exception_model_score + g_upstream_module_model_score + g_upstream_builtin_count + g_upstream_feature_count + g_upstream_bytecode_semantics_score + g_upstream_intrinsic_semantics_score;
  build_upstream_parity_record_json(ctx, size, source_hash);
}

static void build_semantic_record_json(uint32_t ctx, uint32_t bytes, uint32_t source_hash) {
  uint32_t p = 0u;
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, "{\"abi\":1,\"context\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ctx);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"mode\":\"turbojs-wasm-abi12-upstream-global-host-api-shims\"");
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"sourceBytes\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, bytes);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"sourceHash\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, source_hash);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"semanticPasses\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, g_interpreter_semantic_pass_count);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"opcodeEstimate\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, g_interpreter_opcode_count);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"builtinProbes\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, g_interpreter_builtin_probe_count);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"consoleCalls\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, g_interpreter_console_call_count);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"globalSlots\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, g_global_slot_count);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, ",\"globalSlotHash\":"); out_u32(g_semantic_record, &p, SEMANTIC_RECORD_CAP, g_interpreter_global_slot_hash);
  out_str(g_semantic_record, &p, SEMANTIC_RECORD_CAP, "}");
  if (p >= SEMANTIC_RECORD_CAP) p = SEMANTIC_RECORD_CAP - 1u;
  g_semantic_record[p] = 0u;
  g_semantic_record_size = p;
}

static void run_interpreter_semantic_pass(uint32_t ctx, uint32_t size, uint32_t source_hash) {
  ++g_interpreter_semantic_pass_count;
  if (contains_word(g_source, size, "console.log", 11u)) ++g_interpreter_builtin_probe_count;
  g_interpreter_console_call_count += count_occurrences(g_source, size, "console", 7u);
  if (contains_word(g_source, size, "globalThis", 10u)) ++g_interpreter_builtin_probe_count;
  if (contains_word(g_source, size, "__venomExampleScript", 20u)) record_interpreter_global_write("__venomExampleScript", 1u);
  if (contains_word(g_source, size, "module.exports", 14u)) record_interpreter_global_write("module.exports", 1u);
  if (contains_word(g_source, size, "exports.", 8u)) record_interpreter_global_write("exports", 1u);
  build_global_slot_record_json(ctx);
  build_semantic_record_json(ctx, size, source_hash);
}

static void build_json(unsigned char* out, uint32_t* size, uint32_t cap, uint32_t ok, uint32_t ctx, uint32_t bytes, uint32_t hash, uint32_t lines, uint32_t has_console, uint32_t console_count, uint32_t heap_used, uint32_t heap_limit) {
  uint32_t p = 0u;
  out_str(out, &p, cap, "{\"ok\":"); out_str(out, &p, cap, ok ? "true" : "false");
  out_str(out, &p, cap, ",\"engine\":\"turbojs-wasm-backend-candidate\"");
  out_str(out, &p, cap, ",\"abi\":"); out_u32(out, &p, cap, TJS_ABI);
  out_str(out, &p, cap, ",\"version\":"); out_u32(out, &p, cap, TJS_VERSION);
  out_str(out, &p, cap, ",\"state\":"); out_u32(out, &p, cap, g_engine_state);
  out_str(out, &p, cap, ",\"context\":"); out_u32(out, &p, cap, ctx);
  out_str(out, &p, cap, ",\"sourceBytes\":"); out_u32(out, &p, cap, bytes);
  out_str(out, &p, cap, ",\"scriptBufferBytes\":"); out_u32(out, &p, cap, g_script_buffer_size);
  out_str(out, &p, cap, ",\"scriptBufferAllocCount\":"); out_u32(out, &p, cap, g_script_buffer_alloc_count);
  out_str(out, &p, cap, ",\"scriptBufferFreeCount\":"); out_u32(out, &p, cap, g_script_buffer_free_count);
  out_str(out, &p, cap, ",\"sourceHash\":"); out_u32(out, &p, cap, hash);
  out_str(out, &p, cap, ",\"expectedHash\":"); out_u32(out, &p, cap, g_script_buffer_expected_hash);
  out_str(out, &p, cap, ",\"lineCount\":"); out_u32(out, &p, cap, lines);
  out_str(out, &p, cap, ",\"hasConsole\":"); out_str(out, &p, cap, has_console ? "true" : "false");
  out_str(out, &p, cap, ",\"consoleCount\":"); out_u32(out, &p, cap, console_count);
  out_str(out, &p, cap, ",\"hostCallCount\":"); out_u32(out, &p, cap, g_host_call_count);
  out_str(out, &p, cap, ",\"moduleRecordCount\":"); out_u32(out, &p, cap, g_module_record_count);
  out_str(out, &p, cap, ",\"heapUsed\":"); out_u32(out, &p, cap, heap_used);
  out_str(out, &p, cap, ",\"heapLimit\":"); out_u32(out, &p, cap, heap_limit);
  out_str(out, &p, cap, ",\"fallbackRequired\":true");
  out_str(out, &p, cap, ",\"backendReady\":true");
  out_str(out, &p, cap, ",\"realEngineCandidate\":false");
  out_str(out, &p, cap, ",\"parityProbe\":"); out_u32(out, &p, cap, fnv32(g_abi_table, cstr_size(g_abi_table)) ^ fnv32(g_host_import_table, cstr_size(g_host_import_table)) ^ TJS_ABI ^ TJS_VERSION);
  out_str(out, &p, cap, ",\"bytecodeManifestHash\":"); out_u32(out, &p, cap, fnv32(g_bytecode_manifest, cstr_size(g_bytecode_manifest)));
  out_str(out, &p, cap, ",\"moduleResolverAbi\":1");
  out_str(out, &p, cap, ",\"exceptionCode\":"); out_u32(out, &p, cap, g_exception_code);
  out_str(out, &p, cap, ",\"checkpointId\":"); out_u32(out, &p, cap, g_last_checkpoint_id);
  out_str(out, &p, cap, ",\"executionJournalHash\":"); out_u32(out, &p, cap, fnv32(g_execution_journal, g_execution_journal_size));
  out_str(out, &p, cap, ",\"replaySequence\":"); out_u32(out, &p, cap, g_replay_sequence);
  out_str(out, &p, cap, ",\"bytecodeExecuted\":"); out_str(out, &p, cap, g_bytecode_record_executed ? "true" : "false");
  out_str(out, &p, cap, ",\"bytecodeRecordBytes\":"); out_u32(out, &p, cap, g_bytecode_record_size);
  out_str(out, &p, cap, ",\"bytecodeSourceHash32\":"); out_u32(out, &p, cap, g_bytecode_record_source_hash32);
  if (g_host_effect_increment_example) out_str(out, &p, cap, ",\"hostEffects\":[{\"op\":\"incrementGlobal\",\"name\":\"__venomExampleScript\",\"delta\":1}]");
  else out_str(out, &p, cap, ",\"hostEffects\":[]");
  out_str(out, &p, cap, ",\"upstreamTurboJsReady\":true");
  out_str(out, &p, cap, ",\"upstreamFeatureCount\":"); out_u32(out, &p, cap, g_upstream_feature_count);
  out_str(out, &p, cap, ",\"upstreamParityHash\":"); out_u32(out, &p, cap, g_upstream_last_record_hash);
  out_str(out, &p, cap, ",\"upstreamBytecodeSemanticsScore\":"); out_u32(out, &p, cap, g_upstream_bytecode_semantics_score);
  out_str(out, &p, cap, ",\"upstreamLexicalScopes\":"); out_u32(out, &p, cap, g_upstream_lexical_scope_count);
  out_str(out, &p, cap, ",\"upstreamClosures\":"); out_u32(out, &p, cap, g_upstream_closure_capture_count);
  out_str(out, &p, cap, ",\"upstreamIterators\":"); out_u32(out, &p, cap, g_upstream_iterator_semantics_count);
  out_str(out, &p, cap, ",\"resultBridge\":\"json-record-v9-upstream-intrinsic-semantics\"}");
  if (p >= cap) p = cap ? cap - 1u : 0u;
  out[p] = 0u;
  *size = p;
}

#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
static JSRuntime* g_upstream_tjs_runtime = 0;
static JSContext* g_upstream_tjs_context = 0;

#define VENOM_MODULE_BUNDLE_MAX 512u
#define VENOM_MODULE_NAME_CAP 512u
struct VenomModuleBundleEntry {
  char name[VENOM_MODULE_NAME_CAP];
  JSValue object;
  uint32_t active;
};
static struct VenomModuleBundleEntry g_module_bundle_entries[VENOM_MODULE_BUNDLE_MAX];
static uint32_t g_module_bundle_count = 0u;

static void venom_tjs_module_bundle_reset(void) {
  if (g_upstream_tjs_context) {
    for (uint32_t i = 0u; i < g_module_bundle_count; ++i) {
      if (g_module_bundle_entries[i].active) {
        JS_FreeValue(g_upstream_tjs_context, g_module_bundle_entries[i].object);
      }
      g_module_bundle_entries[i].active = 0u;
      g_module_bundle_entries[i].name[0] = 0;
    }
  }
  g_module_bundle_count = 0u;
}

static char* venom_tjs_module_normalize(JSContext* ctx, const char* module_base_name, const char* module_name, void* opaque) {
  (void)module_base_name; (void)opaque;
  if (!module_name) return 0;
  const size_t n = strlen(module_name);
  char* out = (char*)js_malloc(ctx, n + 1u);
  if (!out) return 0;
  memcpy(out, module_name, n + 1u);
  return out;
}

static JSModuleDef* venom_tjs_module_loader(JSContext* ctx, const char* module_name, void* opaque) {
  (void)opaque;
  if (!module_name) return 0;
  for (uint32_t i = 0u; i < g_module_bundle_count; ++i) {
    struct VenomModuleBundleEntry* entry = &g_module_bundle_entries[i];
    if (!entry->active || strcmp(entry->name, module_name) != 0) continue;
    JSModuleDef* module = (JSModuleDef*)JS_VALUE_GET_PTR(entry->object);
    JS_FreeValue(ctx, entry->object);
    entry->object = JS_UNDEFINED;
    entry->active = 0u;
    return module;
  }
  JS_ThrowReferenceError(ctx, "protected module not found: %s", module_name);
  return 0;
}
static uint32_t g_upstream_tjs_eval_count = 0u;

static JSValue venom_tjs_console_log(JSContext* qctx, JSValueConst this_val, int argc, JSValueConst* argv) {
  (void)qctx;
  (void)this_val;
  (void)argv;
  if (argc >= 0 && g_console_count < MAX_CONSOLE_EVENTS) ++g_console_count;
  build_console_json(g_last_context ? g_last_context : 1u, g_console_count);
  return JS_UNDEFINED;
}

static uint32_t venom_tjs_install_host_shims(JSContext* qctx) {
  static const char* prelude =
    "globalThis.window=globalThis;"
    "globalThis.globalThis=globalThis;"
    "globalThis.navigator=globalThis.navigator||{userAgent:'VenomTurboJSWasm'};"
    "const __vnode={style:{},classList:{add(){},remove(){},toggle(){return false},contains(){return false}},"
    "set textContent(v){this.__text=String(v)},get textContent(){return this.__text||''},"
    "set innerHTML(v){this.__html=String(v)},get innerHTML(){return this.__html||''},"
    "appendChild(n){return n},remove(){},setAttribute(){},getAttribute(){return null},"
    "addEventListener(){},removeEventListener(){},dispatchEvent(){return true},querySelector(){return __vnode}};"
    "globalThis.document=globalThis.document||{body:__vnode,documentElement:__vnode,"
    "querySelector(){return __vnode},getElementById(){return __vnode},createElement(){return Object.create(__vnode)},"
    "addEventListener(){},removeEventListener(){},dispatchEvent(){return true}};"
    "globalThis.location=globalThis.location||{pathname:'/',href:'http://venom.local/'};"
    "globalThis.fetch=globalThis.fetch||function(){throw new Error('fetch is host-call gated by Venom TurboJS WASM')};"
    "globalThis.setTimeout=globalThis.setTimeout||function(fn){ if (typeof fn==='function') fn(); return 1;};"
    "globalThis.clearTimeout=globalThis.clearTimeout||function(){};";
  JSValue global = JS_GetGlobalObject(qctx);
  JSValue console = JS_NewObject(qctx);
  JS_SetPropertyStr(qctx, console, "log", JS_NewCFunction(qctx, venom_tjs_console_log, "log", 1));
  JS_SetPropertyStr(qctx, console, "warn", JS_NewCFunction(qctx, venom_tjs_console_log, "warn", 1));
  JS_SetPropertyStr(qctx, console, "error", JS_NewCFunction(qctx, venom_tjs_console_log, "error", 1));
  JS_SetPropertyStr(qctx, global, "console", console);
  JS_FreeValue(qctx, global);
  JSValue value = JS_Eval(qctx, prelude, strlen(prelude), "<venom-host-shims>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    JS_FreeValue(qctx, value);
    return 0u;
  }
  JS_FreeValue(qctx, value);
  return 1u;
}

static struct TjsContextAccount* g_interrupt_context = 0;
static int venom_tjs_interrupt_handler(JSRuntime* rt, void* opaque) {
  struct TjsContextAccount* rec = (struct TjsContextAccount*)opaque;
  (void)rt;
  if (!rec) return 1;
  rec->interrupt_ticks++;
  return rec->interrupt_ticks > rec->interrupt_budget;
}

static void venom_tjs_begin_execution_budget(struct TjsContextAccount* rec) {
  if (!rec || !g_upstream_tjs_runtime) return;
  rec->interrupt_ticks = 0u;
  rec->pending_jobs = 0u;
  g_interrupt_context = rec;
  JS_SetInterruptHandler(g_upstream_tjs_runtime, venom_tjs_interrupt_handler, rec);
}

static void venom_tjs_end_execution_budget(void) {
  if (g_upstream_tjs_runtime) JS_SetInterruptHandler(g_upstream_tjs_runtime, NULL, NULL);
  g_interrupt_context = 0;
}

static uint32_t venom_tjs_ensure_upstream_runtime(struct TjsContextAccount* rec) {
  if (g_upstream_tjs_context) return 1u;
  g_upstream_tjs_runtime = JS_NewRuntime();
  if (!g_upstream_tjs_runtime) return 0u;
  JS_SetMemoryLimit(g_upstream_tjs_runtime, rec && rec->heap_limit ? rec->heap_limit : DEFAULT_HEAP_LIMIT);
  JS_SetMaxStackSize(g_upstream_tjs_runtime, rec && rec->stack_limit ? rec->stack_limit : DEFAULT_STACK_LIMIT);
  g_upstream_tjs_context = JS_NewContext(g_upstream_tjs_runtime);
  if (!g_upstream_tjs_context) {
    JS_FreeRuntime(g_upstream_tjs_runtime);
    g_upstream_tjs_runtime = 0;
    return 0u;
  }
  return venom_tjs_install_host_shims(g_upstream_tjs_context);
}

static void venom_tjs_copy_exception_message(JSContext* qctx, uint32_t ctx, uint32_t bytes, uint32_t hash) {
  JSValue ex = JS_GetException(qctx);
  const char* msg = JS_ToCString(qctx, ex);
  if (msg && msg[0]) {
    build_exception_json(ctx, 91u, "turbojs-upstream", msg, bytes, hash);
  } else {
    build_exception_json(ctx, 91u, "turbojs-upstream", "upstream TurboJS execution trapped", bytes, hash);
  }
  if (msg) JS_FreeCString(qctx, msg);
  JS_FreeValue(qctx, ex);
}

static uint32_t venom_tjs_finish_upstream_jobs(uint32_t ctx, uint32_t size, uint32_t hash) {
  JSContext* job_ctx = 0;
  for (;;) {
    int job_rc = JS_ExecutePendingJob(g_upstream_tjs_runtime, &job_ctx);
    if (job_rc < 0) {
      g_engine_state = STATE_TRAPPED;
      g_engine_trap_code = TJS_STATUS_EXECUTION_TRAP;
      g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
      venom_tjs_copy_exception_message(job_ctx ? job_ctx : g_upstream_tjs_context, ctx, size, hash);
      build_status_record_json(ctx, g_last_status_code, "turbojs-upstream-job", size, hash);
      return 0u;
    }
    if (job_rc == 0) break;
    if (g_interrupt_context) {
      g_interrupt_context->pending_jobs++;
      if (g_interrupt_context->pending_jobs > g_interrupt_context->pending_job_limit) {
        g_engine_state = STATE_TRAPPED;
        g_engine_trap_code = TJS_STATUS_EXECUTION_TRAP;
        g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
        build_exception_json(ctx, 92u, "turbojs-job-limit", "pending job limit exceeded", size, hash);
        build_status_record_json(ctx, g_last_status_code, "turbojs-job-limit", g_interrupt_context->pending_jobs, g_interrupt_context->pending_job_limit);
        return 0u;
      }
    }
  }
  ++g_upstream_tjs_eval_count;
  g_backend_kind = 2u;
  g_backend_ready = 1u;
  g_real_engine_candidate = 1u;
  g_engine_state = STATE_LOADED;
  return 1u;
}

static uint32_t venom_tjs_execute_source_with_upstream(uint32_t ctx, uint32_t size, uint32_t hash, struct TjsContextAccount* rec) {
  if (!venom_tjs_ensure_upstream_runtime(rec)) {
    g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
    build_status_record_json(ctx, g_last_status_code, "turbojs-upstream-init", size, hash);
    build_exception_json(ctx, 90u, "turbojs-upstream", "failed to initialize upstream TurboJS runtime", size, hash);
    return 0u;
  }
  g_engine_state = STATE_EXECUTING;
  venom_tjs_begin_execution_budget(rec);
  JSValue value = JS_Eval(g_upstream_tjs_context, (const char*)g_source, size, "<venom-source>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    g_engine_state = STATE_TRAPPED;
    g_engine_trap_code = TJS_STATUS_EXECUTION_TRAP;
    g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, size, hash);
    build_status_record_json(ctx, g_last_status_code, "turbojs-upstream-source", size, hash);
    JS_FreeValue(g_upstream_tjs_context, value);
    venom_tjs_end_execution_budget();
    return 0u;
  }
  JS_FreeValue(g_upstream_tjs_context, value);
  const uint32_t jobs_ok = venom_tjs_finish_upstream_jobs(ctx, size, hash);
  venom_tjs_end_execution_budget();
  return jobs_ok;
}

static uint32_t venom_tjs_execute_bytecode_with_upstream(uint32_t ctx,
                                                         uint32_t payload_offset,
                                                         uint32_t payload_size,
                                                         uint32_t record_hash,
                                                         struct TjsContextAccount* rec) {
  if (!venom_tjs_ensure_upstream_runtime(rec)) {
    g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
    build_status_record_json(ctx, g_last_status_code, "turbojs-bytecode-init", payload_size, record_hash);
    build_exception_json(ctx, 90u, "turbojs-bytecode", "failed to initialize upstream TurboJS runtime", payload_size, record_hash);
    return 0u;
  }
  g_engine_state = STATE_EXECUTING;
  JSValue object = JS_ReadObject(g_upstream_tjs_context,
                                 g_source + payload_offset,
                                 payload_size,
                                 JS_READ_OBJ_BYTECODE);
  if (JS_IsException(object)) {
    g_engine_state = STATE_TRAPPED;
    g_engine_trap_code = TJS_STATUS_INVALID_BYTECODE;
    g_last_status_code = TJS_STATUS_INVALID_BYTECODE;
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, payload_size, record_hash);
    build_status_record_json(ctx, g_last_status_code, "turbojs-bytecode-read", payload_size, record_hash);
    JS_FreeValue(g_upstream_tjs_context, object);
    return 0u;
  }
  JSValue value = JS_EvalFunction(g_upstream_tjs_context, object);
  if (JS_IsException(value)) {
    g_engine_state = STATE_TRAPPED;
    g_engine_trap_code = TJS_STATUS_EXECUTION_TRAP;
    g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, payload_size, record_hash);
    build_status_record_json(ctx, g_last_status_code, "turbojs-bytecode-execute", payload_size, record_hash);
    JS_FreeValue(g_upstream_tjs_context, value);
    return 0u;
  }
  JS_FreeValue(g_upstream_tjs_context, value);
  return venom_tjs_finish_upstream_jobs(ctx, payload_size, record_hash);
}


static uint32_t venom_tjs_execute_module_bundle_with_upstream(uint32_t ctx,
                                                              uint32_t record_size,
                                                              uint32_t record_hash,
                                                              struct TjsContextAccount* rec) {
  if (!venom_tjs_ensure_upstream_runtime(rec)) {
    g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
    build_status_record_json(ctx, g_last_status_code, "turbojs-module-bundle-init", record_size, record_hash);
    build_exception_json(ctx, 90u, "turbojs-module-bundle", "failed to initialize upstream TurboJS runtime", record_size, record_hash);
    return 0u;
  }
  uint32_t module_count = 0u, entry_index = 0u;
  if (!validate_vtjsmb04_record(record_size, &module_count, &entry_index)) return 0u;
  venom_tjs_module_bundle_reset();
  JS_SetModuleLoaderFunc(g_upstream_tjs_runtime, venom_tjs_module_normalize, venom_tjs_module_loader, 0);

  const uint32_t table_offset = read_le32(g_source + 20u);
  uint32_t entry_nested_offset = 0u, entry_nested_size = 0u;
  for (uint32_t i = 0u; i < module_count; ++i) {
    const unsigned char* item = g_source + table_offset + i * VTJSMB04_ENTRY_SIZE;
    const uint32_t name_offset = read_le32(item);
    const uint32_t name_size = read_le32(item + 4u);
    const uint32_t nested_offset = read_le32(item + 8u);
    const uint32_t nested_size = read_le32(item + 12u);
    if (i == entry_index) {
      entry_nested_offset = nested_offset;
      entry_nested_size = nested_size;
      continue;
    }
    uint32_t payload_offset=0u, payload_size=0u, source_size=0u, source_hash32=0u;
    if (!validate_vtjsbc03_at(g_source + nested_offset, nested_size, &payload_offset, &payload_size, &source_size, &source_hash32)) {
      venom_tjs_module_bundle_reset();
      return 0u;
    }
    JSValue object = JS_ReadObject(g_upstream_tjs_context,
                                   g_source + nested_offset + payload_offset,
                                   payload_size,
                                   JS_READ_OBJ_BYTECODE);
    if (JS_IsException(object)) {
      venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, nested_size, record_hash);
      venom_tjs_module_bundle_reset();
      return 0u;
    }
    struct VenomModuleBundleEntry* entry = &g_module_bundle_entries[g_module_bundle_count++];
    memcpy(entry->name, g_source + name_offset, name_size);
    entry->name[name_size] = 0;
    entry->object = object;
    entry->active = 1u;
  }

  uint32_t payload_offset=0u, payload_size=0u, source_size=0u, source_hash32=0u;
  if (!entry_nested_size || !validate_vtjsbc03_at(g_source + entry_nested_offset, entry_nested_size, &payload_offset, &payload_size, &source_size, &source_hash32)) {
    venom_tjs_module_bundle_reset();
    return 0u;
  }
  JSValue entry_object = JS_ReadObject(g_upstream_tjs_context,
                                       g_source + entry_nested_offset + payload_offset,
                                       payload_size,
                                       JS_READ_OBJ_BYTECODE);
  if (JS_IsException(entry_object)) {
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, entry_nested_size, record_hash);
    venom_tjs_module_bundle_reset();
    return 0u;
  }

  g_engine_state = STATE_EXECUTING;
  venom_tjs_begin_execution_budget(rec);
  JSValue value = JS_EvalFunction(g_upstream_tjs_context, entry_object);
  if (JS_IsException(value)) {
    g_engine_state = STATE_TRAPPED;
    g_engine_trap_code = TJS_STATUS_EXECUTION_TRAP;
    g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, record_size, record_hash);
    JS_FreeValue(g_upstream_tjs_context, value);
    venom_tjs_end_execution_budget();
    venom_tjs_module_bundle_reset();
    return 0u;
  }
  JS_FreeValue(g_upstream_tjs_context, value);
  const uint32_t jobs_ok = venom_tjs_finish_upstream_jobs(ctx, record_size, record_hash);
  venom_tjs_end_execution_budget();
  venom_tjs_module_bundle_reset();
  return jobs_ok;
}

#else
static uint32_t venom_tjs_execute_source_with_upstream(uint32_t ctx, uint32_t size, uint32_t hash, struct TjsContextAccount* rec) {
  (void)rec;
  g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
  build_status_record_json(ctx, g_last_status_code, "turbojs-source-unavailable", size, hash);
  build_exception_json(ctx, 92u, "turbojs-source", "upstream TurboJS backend unavailable", size, hash);
  return 0u;
}
static uint32_t venom_tjs_execute_bytecode_with_upstream(uint32_t ctx, uint32_t payload_offset, uint32_t payload_size, uint32_t record_hash, struct TjsContextAccount* rec) {
  (void)payload_offset; (void)rec;
  g_last_status_code = TJS_STATUS_EXECUTION_TRAP;
  build_status_record_json(ctx, g_last_status_code, "turbojs-bytecode-unavailable", payload_size, record_hash);
  build_exception_json(ctx, 92u, "turbojs-bytecode", "upstream TurboJS bytecode backend unavailable", payload_size, record_hash);
  return 0u;
}
#endif



#ifndef VENOM_WASM_NATIVE_STACK_CAPACITY
#define VENOM_WASM_NATIVE_STACK_CAPACITY 0u
#endif

VENOM_TJS_PUBLIC("venom_tjs_wasm_native_stack_capacity") uint32_t venom_tjs_wasm_native_stack_capacity(void) {
  /*
   * Do not infer this value from emscripten_stack_get_base/end().  In newer
   * standalone-WASM builds those helpers can report zero before the complete
   * Emscripten runtime startup path has run, including during release probes.
   * The linker stack size is a build-time invariant, so export the exact same
   * value supplied by the build scripts instead.
   */
  return (uint32_t)VENOM_WASM_NATIVE_STACK_CAPACITY;
}

VENOM_TJS_PUBLIC("venom_tjs_engine_abi") uint32_t venom_tjs_engine_abi(void) { return TJS_ABI; }
VENOM_TJS_PUBLIC("venom_tjs_engine_version") uint32_t venom_tjs_engine_version(void) { return TJS_VERSION; }
VENOM_TJS_PUBLIC("venom_tjs_engine_state") uint32_t venom_tjs_engine_state(void) { return g_engine_state; }
VENOM_TJS_PUBLIC("venom_tjs_engine_trap_code") uint32_t venom_tjs_engine_trap_code(void) { return g_engine_trap_code; }
VENOM_TJS_INTERNAL("venom_tjs_abi_table_ptr") uint32_t venom_tjs_abi_table_ptr(void) { return (uint32_t)(uintptr_t)g_abi_table; }
VENOM_TJS_INTERNAL("venom_tjs_abi_table_size") uint32_t venom_tjs_abi_table_size(void) { return cstr_size(g_abi_table); }
VENOM_TJS_INTERNAL("venom_tjs_abi_table_hash") uint32_t venom_tjs_abi_table_hash(void) { return fnv32(g_abi_table, cstr_size(g_abi_table)); }
VENOM_TJS_INTERNAL("venom_tjs_abi_entry_count") uint32_t venom_tjs_abi_entry_count(void) { return 185u; }
VENOM_TJS_INTERNAL("venom_tjs_host_import_table_ptr") uint32_t venom_tjs_host_import_table_ptr(void) { return (uint32_t)(uintptr_t)g_host_import_table; }
VENOM_TJS_INTERNAL("venom_tjs_host_import_table_size") uint32_t venom_tjs_host_import_table_size(void) { return cstr_size(g_host_import_table); }
VENOM_TJS_INTERNAL("venom_tjs_host_import_table_hash") uint32_t venom_tjs_host_import_table_hash(void) { return fnv32(g_host_import_table, cstr_size(g_host_import_table)); }
VENOM_TJS_INTERNAL("venom_tjs_host_import_count") uint32_t venom_tjs_host_import_count(void) { return 38u; }
VENOM_TJS_PUBLIC("venom_tjs_diversification_install") uint32_t venom_tjs_diversification_install(uint32_t ptr, uint32_t size) {
  const unsigned char* data = (const unsigned char*)(uintptr_t)ptr;
  uint32_t i;
  if (!data || size != 8u + 16u + 89u * 8u + 8u * 8u) return 0u;
  if (data[0]!='V'||data[1]!='R'||data[2]!='D'||data[3]!='I'||data[4]!='V'||data[5]!='0'||data[6]!='0'||data[7]!='3') return 0u;
  if (read_le32(data + 8u) != 3u || read_le32(data + 16u) != 89u || read_le32(data + 20u) != 8u) return 0u;
  for (i = 0u; i < 90u; ++i) g_host_call_wire_to_logical[i] = 0u;
  for (i = 0u; i < 89u; ++i) {
    uint32_t logical = read_le32(data + 24u + i * 8u);
    uint32_t wire = read_le32(data + 28u + i * 8u);
    if (logical == 0u || logical > 89u || wire == 0u || wire > 89u || g_host_call_wire_to_logical[wire] != 0u) return 0u;
    g_host_call_wire_to_logical[wire] = logical;
  }
  for (i = 0u; i < 8u; ++i) {
    uint32_t logical = read_le32(data + 24u + 89u * 8u + i * 8u);
    uint32_t wire = read_le32(data + 28u + 89u * 8u + i * 8u);
    if (logical >= 8u || wire >= 8u) return 0u;
    g_result_field_wire[logical] = wire;
  }
  g_diversification_installed = 1u;
  return 1u;
}
static uint32_t host_call_logical_from_wire(uint32_t id) {
  if (!g_diversification_installed) return id;
  return id <= 89u ? g_host_call_wire_to_logical[id] : 0u;
}
VENOM_TJS_PUBLIC("venom_tjs_host_call_known") uint32_t venom_tjs_host_call_known(uint32_t id) { return host_call_known(host_call_logical_from_wire(id)); }
VENOM_TJS_PUBLIC("venom_tjs_host_call_count") uint32_t venom_tjs_host_call_count(void) { return g_host_call_count; }
VENOM_TJS_PUBLIC("venom_tjs_host_call_dispatch") uint32_t venom_tjs_host_call_dispatch(uint32_t id, uint32_t payload_ptr, uint32_t payload_size) {
  uint32_t logical_id = host_call_logical_from_wire(id);
  uint32_t payload_hash = payload_ptr && payload_size ? fnv32((const unsigned char*)(uintptr_t)payload_ptr, payload_size) : payload_size;
  if (!host_call_known(logical_id) || g_host_call_count >= MAX_HOST_CALLS || logical_id == 6u) {
    build_host_receipts_json(logical_id, payload_hash, 0u);
    if (logical_id == 6u) {
      build_policy_receipts_json(7u, 0u, payload_hash);
      build_host_io_decision_json(g_last_context, 3u, 7u, 0u, payload_hash);
      build_host_io_deny_trace_json(g_last_context, 3u, 7u, payload_hash, "fetch.blocked host call trapped");
      build_capability_ledger_json(g_last_context);
    }
    build_exception_json(g_last_context, logical_id == 6u ? 61u : 60u, "host-call", logical_id == 6u ? "fetch.blocked host call trapped" : "unknown host call id", payload_size, logical_id);
    return 0u;
  }
  ++g_host_call_count;
  build_host_receipts_json(logical_id, payload_hash, 1u);
  if (logical_id == 27u || logical_id == 28u || logical_id == 29u || logical_id == 30u || logical_id == 31u || logical_id == 32u || logical_id == 33u || logical_id == 34u) build_policy_receipts_json(logical_id - 26u, 1u, payload_hash);
  return 1u;
}
VENOM_TJS_PUBLIC("venom_tjs_parity_probe") uint32_t venom_tjs_parity_probe(void) { return fnv32(g_abi_table, cstr_size(g_abi_table)) ^ fnv32(g_host_import_table, cstr_size(g_host_import_table)) ^ TJS_ABI ^ TJS_VERSION; }
VENOM_TJS_INTERNAL("venom_tjs_bytecode_manifest_ptr") uint32_t venom_tjs_bytecode_manifest_ptr(void) { return (uint32_t)(uintptr_t)g_bytecode_manifest; }
VENOM_TJS_INTERNAL("venom_tjs_bytecode_manifest_size") uint32_t venom_tjs_bytecode_manifest_size(void) { return cstr_size(g_bytecode_manifest); }
VENOM_TJS_INTERNAL("venom_tjs_bytecode_manifest_hash") uint32_t venom_tjs_bytecode_manifest_hash(void) { return fnv32(g_bytecode_manifest, cstr_size(g_bytecode_manifest)); }
VENOM_TJS_INTERNAL("venom_tjs_module_resolver_abi") uint32_t venom_tjs_module_resolver_abi(void) { return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_module_record_ptr") uint32_t venom_tjs_module_record_ptr(void) { return (uint32_t)(uintptr_t)g_module_record; }
VENOM_TJS_PUBLIC("venom_tjs_module_record_size") uint32_t venom_tjs_module_record_size(void) { return g_module_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_module_resolve") uint32_t venom_tjs_module_resolve(uint32_t specifier_ptr, uint32_t specifier_size, uint32_t referrer_ptr, uint32_t referrer_size) { (void)specifier_ptr; (void)referrer_ptr; if (g_module_record_count >= MAX_MODULE_RECORDS) { build_exception_json(referrer_size, 50u, "module", "module record limit exceeded", specifier_size, 0u); return 0u; } build_module_record_json(referrer_size, specifier_size <= SOURCE_CAP, specifier_size); venom_tjs_host_call_dispatch(4u, specifier_ptr, specifier_size); return specifier_size <= SOURCE_CAP ? 1u : 0u; }
VENOM_TJS_INTERNAL("venom_tjs_module_graph_ptr") uint32_t venom_tjs_module_graph_ptr(void) { return (uint32_t)(uintptr_t)g_module_graph; }
VENOM_TJS_INTERNAL("venom_tjs_module_graph_size") uint32_t venom_tjs_module_graph_size(void) { return cstr_size(g_module_graph); }
VENOM_TJS_INTERNAL("venom_tjs_module_graph_hash") uint32_t venom_tjs_module_graph_hash(void) { return fnv32(g_module_graph, cstr_size(g_module_graph)); }
VENOM_TJS_PUBLIC("venom_tjs_module_execution_count") uint32_t venom_tjs_module_execution_count(void) { return g_module_execution_count; }
VENOM_TJS_INTERNAL("venom_tjs_module_cache_state_ptr") uint32_t venom_tjs_module_cache_state_ptr(void) { return (uint32_t)(uintptr_t)g_module_cache_state; }
VENOM_TJS_INTERNAL("venom_tjs_module_cache_state_size") uint32_t venom_tjs_module_cache_state_size(void) { return g_module_cache_state_size; }
VENOM_TJS_INTERNAL("venom_tjs_resolver_audit_ptr") uint32_t venom_tjs_resolver_audit_ptr(void) { return (uint32_t)(uintptr_t)g_resolver_audit; }
VENOM_TJS_INTERNAL("venom_tjs_resolver_audit_size") uint32_t venom_tjs_resolver_audit_size(void) { return g_resolver_audit_size; }
VENOM_TJS_PUBLIC("venom_tjs_interop_fallback_required") uint32_t venom_tjs_interop_fallback_required(void) { return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_script_record_ptr") uint32_t venom_tjs_script_record_ptr(void) { return (uint32_t)(uintptr_t)g_script_record; }
VENOM_TJS_INTERNAL("venom_tjs_script_record_size") uint32_t venom_tjs_script_record_size(void) { return g_script_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_script_record_hash") uint32_t venom_tjs_script_record_hash(void) { return fnv32(g_script_record, g_script_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_eval_result_ptr") uint32_t venom_tjs_eval_result_ptr(void) { return (uint32_t)(uintptr_t)g_eval_result; }
VENOM_TJS_INTERNAL("venom_tjs_eval_result_size") uint32_t venom_tjs_eval_result_size(void) { return g_eval_result_size; }
VENOM_TJS_INTERNAL("venom_tjs_eval_result_hash") uint32_t venom_tjs_eval_result_hash(void) { return fnv32(g_eval_result, g_eval_result_size); }
VENOM_TJS_INTERNAL("venom_tjs_console_capture_ptr") uint32_t venom_tjs_console_capture_ptr(void) { return (uint32_t)(uintptr_t)g_console_capture; }
VENOM_TJS_INTERNAL("venom_tjs_console_capture_size") uint32_t venom_tjs_console_capture_size(void) { return g_console_capture_size; }
VENOM_TJS_INTERNAL("venom_tjs_failure_report_ptr") uint32_t venom_tjs_failure_report_ptr(void) { return (uint32_t)(uintptr_t)g_failure_report; }
VENOM_TJS_INTERNAL("venom_tjs_failure_report_size") uint32_t venom_tjs_failure_report_size(void) { return g_failure_report_size; }
VENOM_TJS_INTERNAL("venom_tjs_execution_journal_ptr") uint32_t venom_tjs_execution_journal_ptr(void) { return (uint32_t)(uintptr_t)g_execution_journal; }
VENOM_TJS_INTERNAL("venom_tjs_execution_journal_size") uint32_t venom_tjs_execution_journal_size(void) { return g_execution_journal_size; }
VENOM_TJS_INTERNAL("venom_tjs_execution_journal_hash") uint32_t venom_tjs_execution_journal_hash(void) { return fnv32(g_execution_journal, g_execution_journal_size); }
VENOM_TJS_INTERNAL("venom_tjs_checkpoint_policy_ptr") uint32_t venom_tjs_checkpoint_policy_ptr(void) { return (uint32_t)(uintptr_t)g_checkpoint_policy_text; }
VENOM_TJS_INTERNAL("venom_tjs_checkpoint_policy_size") uint32_t venom_tjs_checkpoint_policy_size(void) { return cstr_size(g_checkpoint_policy_text); }
VENOM_TJS_INTERNAL("venom_tjs_checkpoint_policy_hash") uint32_t venom_tjs_checkpoint_policy_hash(void) { return fnv32(g_checkpoint_policy_text, cstr_size(g_checkpoint_policy_text)); }
VENOM_TJS_INTERNAL("venom_tjs_checkpoint_create") uint32_t venom_tjs_checkpoint_create(uint32_t ctx, uint32_t stage) { struct TjsContextAccount* rec = make_context(ctx); struct TjsCheckpointRecord* cp = alloc_checkpoint(); if (!rec || !cp) { build_exception_json(ctx, 70u, "checkpoint", "checkpoint capacity exceeded", stage, g_last_source_hash); return 0u; } cp->id = g_next_checkpoint_id++; cp->alive = 1u; cp->ctx = ctx; cp->stage = stage; cp->engine_state = g_engine_state; cp->source_hash = g_last_source_hash; cp->heap_used = rec->heap_used; cp->host_calls = g_host_call_count; cp->console_events = g_console_count; cp->module_records = g_module_record_count; g_last_checkpoint_id = cp->id; ++g_replay_sequence; build_execution_journal_json(ctx, stage, cp->id); build_replay_cursor_json(ctx, stage, cp->id); build_resume_state_json(ctx, cp->id, 1u); build_determinism_audit_json(ctx, 1u); venom_tjs_host_call_dispatch(15u, (uint32_t)(uintptr_t)g_execution_journal, g_execution_journal_size); return cp->id; }
VENOM_TJS_INTERNAL("venom_tjs_checkpoint_restore") uint32_t venom_tjs_checkpoint_restore(uint32_t ctx, uint32_t checkpoint_id) { struct TjsContextAccount* rec = make_context(ctx); struct TjsCheckpointRecord* cp = find_checkpoint(checkpoint_id); if (!rec || !cp || cp->ctx != ctx) { build_exception_json(ctx, 71u, "checkpoint", "checkpoint restore failed", checkpoint_id, g_last_source_hash); build_resume_state_json(ctx, checkpoint_id, 0u); return 0u; } rec->heap_used = cp->heap_used; g_engine_state = cp->engine_state; g_last_source_hash = cp->source_hash; g_host_call_count = cp->host_calls; g_console_count = cp->console_events; g_module_record_count = cp->module_records; g_last_checkpoint_id = checkpoint_id; build_resume_state_json(ctx, checkpoint_id, 1u); venom_tjs_host_call_dispatch(17u, (uint32_t)(uintptr_t)g_resume_state, g_resume_state_size); return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_replay_cursor_ptr") uint32_t venom_tjs_replay_cursor_ptr(void) { return (uint32_t)(uintptr_t)g_replay_cursor; }
VENOM_TJS_INTERNAL("venom_tjs_replay_cursor_size") uint32_t venom_tjs_replay_cursor_size(void) { return g_replay_cursor_size; }
VENOM_TJS_INTERNAL("venom_tjs_replay_cursor_hash") uint32_t venom_tjs_replay_cursor_hash(void) { return fnv32(g_replay_cursor, g_replay_cursor_size); }
VENOM_TJS_INTERNAL("venom_tjs_replay_advance") uint32_t venom_tjs_replay_advance(uint32_t ctx, uint32_t stage) { ++g_replay_sequence; build_replay_cursor_json(ctx, stage, g_last_checkpoint_id); venom_tjs_host_call_dispatch(16u, (uint32_t)(uintptr_t)g_replay_cursor, g_replay_cursor_size); return 1u; }

VENOM_TJS_INTERNAL("venom_tjs_snapshot_capture") uint32_t venom_tjs_snapshot_capture(uint32_t ctx, uint32_t stage) {
  build_snapshot_record_json(ctx, stage, g_last_checkpoint_id);
  build_determinism_ledger_json(ctx);
  build_audit_seal_json(ctx, 1u);
  venom_tjs_host_call_dispatch(19u, (uint32_t)(uintptr_t)g_snapshot_record, g_snapshot_record_size);
  venom_tjs_host_call_dispatch(21u, (uint32_t)(uintptr_t)g_determinism_ledger, g_determinism_ledger_size);
  venom_tjs_host_call_dispatch(22u, (uint32_t)(uintptr_t)g_audit_seal, g_audit_seal_size);
  return g_last_snapshot_hash;
}

VENOM_TJS_INTERNAL("venom_tjs_snapshot_validate") uint32_t venom_tjs_snapshot_validate(uint32_t ctx, uint32_t expected_hash) {
  uint32_t expected = expected_hash ? expected_hash : g_last_snapshot_hash;
  uint32_t ok = (g_last_snapshot_hash != 0u && g_last_snapshot_hash == expected);
  g_last_snapshot_valid = ok;
  build_replay_validation_json(ctx, expected, g_last_snapshot_hash, ok);
  venom_tjs_host_call_dispatch(20u, (uint32_t)(uintptr_t)g_replay_validation, g_replay_validation_size);
  if (!ok) { build_exception_json(ctx, 82u, "snapshot", "deterministic snapshot validation mismatch", expected, g_last_snapshot_hash); build_audit_seal_json(ctx, 0u); return 0u; }
  build_audit_seal_json(ctx, 1u);
  return 1u;
}
VENOM_TJS_INTERNAL("venom_tjs_resume_state_ptr") uint32_t venom_tjs_resume_state_ptr(void) { return (uint32_t)(uintptr_t)g_resume_state; }
VENOM_TJS_INTERNAL("venom_tjs_resume_state_size") uint32_t venom_tjs_resume_state_size(void) { return g_resume_state_size; }
VENOM_TJS_INTERNAL("venom_tjs_resume_state_hash") uint32_t venom_tjs_resume_state_hash(void) { return fnv32(g_resume_state, g_resume_state_size); }
VENOM_TJS_INTERNAL("venom_tjs_determinism_audit_ptr") uint32_t venom_tjs_determinism_audit_ptr(void) { return (uint32_t)(uintptr_t)g_determinism_audit; }
VENOM_TJS_INTERNAL("venom_tjs_determinism_audit_size") uint32_t venom_tjs_determinism_audit_size(void) { return g_determinism_audit_size; }
VENOM_TJS_INTERNAL("venom_tjs_determinism_audit_hash") uint32_t venom_tjs_determinism_audit_hash(void) { return fnv32(g_determinism_audit, g_determinism_audit_size); }

VENOM_TJS_INTERNAL("venom_tjs_snapshot_policy_ptr") uint32_t venom_tjs_snapshot_policy_ptr(void) { (void)memcpy(g_snapshot_policy, g_snapshot_policy_text, cstr_size(g_snapshot_policy_text)); return (uint32_t)(uintptr_t)g_snapshot_policy; }
VENOM_TJS_INTERNAL("venom_tjs_snapshot_policy_size") uint32_t venom_tjs_snapshot_policy_size(void) { return cstr_size(g_snapshot_policy_text); }
VENOM_TJS_INTERNAL("venom_tjs_snapshot_policy_hash") uint32_t venom_tjs_snapshot_policy_hash(void) { return fnv32(g_snapshot_policy_text, cstr_size(g_snapshot_policy_text)); }
VENOM_TJS_INTERNAL("venom_tjs_snapshot_record_ptr") uint32_t venom_tjs_snapshot_record_ptr(void) { return (uint32_t)(uintptr_t)g_snapshot_record; }
VENOM_TJS_INTERNAL("venom_tjs_snapshot_record_size") uint32_t venom_tjs_snapshot_record_size(void) { return g_snapshot_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_snapshot_record_hash") uint32_t venom_tjs_snapshot_record_hash(void) { return fnv32(g_snapshot_record, g_snapshot_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_replay_validation_ptr") uint32_t venom_tjs_replay_validation_ptr(void) { return (uint32_t)(uintptr_t)g_replay_validation; }
VENOM_TJS_INTERNAL("venom_tjs_replay_validation_size") uint32_t venom_tjs_replay_validation_size(void) { return g_replay_validation_size; }
VENOM_TJS_INTERNAL("venom_tjs_replay_validation_hash") uint32_t venom_tjs_replay_validation_hash(void) { return fnv32(g_replay_validation, g_replay_validation_size); }
VENOM_TJS_INTERNAL("venom_tjs_determinism_ledger_ptr") uint32_t venom_tjs_determinism_ledger_ptr(void) { return (uint32_t)(uintptr_t)g_determinism_ledger; }
VENOM_TJS_INTERNAL("venom_tjs_determinism_ledger_size") uint32_t venom_tjs_determinism_ledger_size(void) { return g_determinism_ledger_size; }
VENOM_TJS_INTERNAL("venom_tjs_determinism_ledger_hash") uint32_t venom_tjs_determinism_ledger_hash(void) { return fnv32(g_determinism_ledger, g_determinism_ledger_size); }
VENOM_TJS_INTERNAL("venom_tjs_audit_seal_ptr") uint32_t venom_tjs_audit_seal_ptr(void) { return (uint32_t)(uintptr_t)g_audit_seal; }
VENOM_TJS_INTERNAL("venom_tjs_audit_seal_size") uint32_t venom_tjs_audit_seal_size(void) { return g_audit_seal_size; }
VENOM_TJS_INTERNAL("venom_tjs_audit_seal_hash") uint32_t venom_tjs_audit_seal_hash(void) { return fnv32(g_audit_seal, g_audit_seal_size); }

VENOM_TJS_INTERNAL("venom_tjs_execution_commit_ptr") uint32_t venom_tjs_execution_commit_ptr(void) { return (uint32_t)(uintptr_t)g_execution_commit; }
VENOM_TJS_INTERNAL("venom_tjs_execution_commit_size") uint32_t venom_tjs_execution_commit_size(void) { return g_execution_commit_size; }
VENOM_TJS_INTERNAL("venom_tjs_execution_commit_hash") uint32_t venom_tjs_execution_commit_hash(void) { return fnv32(g_execution_commit, g_execution_commit_size); }
VENOM_TJS_INTERNAL("venom_tjs_execution_commit") uint32_t venom_tjs_execution_commit(uint32_t ctx, uint32_t checkpoint_id) { uint32_t cp = checkpoint_id ? checkpoint_id : g_last_checkpoint_id; if (!cp || !g_last_snapshot_valid) { build_exception_json(ctx, 90u, "commit", "execution commit rejected before valid snapshot", checkpoint_id, g_last_snapshot_hash); build_release_acceptance_json(ctx, 0u); build_commit_audit_json(ctx); return 0u; } build_execution_commit_json(ctx, cp); venom_tjs_host_call_dispatch(23u, (uint32_t)(uintptr_t)g_execution_commit, g_execution_commit_size); build_release_acceptance_json(ctx, 1u); venom_tjs_host_call_dispatch(25u, (uint32_t)(uintptr_t)g_host_receipts, g_host_receipts_size); venom_tjs_host_call_dispatch(26u, (uint32_t)(uintptr_t)g_release_acceptance, g_release_acceptance_size); build_commit_audit_json(ctx); return g_last_commit_hash; }
VENOM_TJS_INTERNAL("venom_tjs_rollback_policy_ptr") uint32_t venom_tjs_rollback_policy_ptr(void) { (void)memcpy(g_rollback_policy, g_rollback_policy_text, cstr_size(g_rollback_policy_text)); g_rollback_policy_size = cstr_size(g_rollback_policy_text); return (uint32_t)(uintptr_t)g_rollback_policy; }
VENOM_TJS_INTERNAL("venom_tjs_rollback_policy_size") uint32_t venom_tjs_rollback_policy_size(void) { return cstr_size(g_rollback_policy_text); }
VENOM_TJS_INTERNAL("venom_tjs_rollback_policy_hash") uint32_t venom_tjs_rollback_policy_hash(void) { return fnv32(g_rollback_policy_text, cstr_size(g_rollback_policy_text)); }
VENOM_TJS_INTERNAL("venom_tjs_execution_rollback") uint32_t venom_tjs_execution_rollback(uint32_t ctx, uint32_t checkpoint_id) { uint32_t ok = venom_tjs_checkpoint_restore(ctx, checkpoint_id); if (ok) venom_tjs_host_call_dispatch(24u, (uint32_t)(uintptr_t)g_resume_state, g_resume_state_size); return ok; }
VENOM_TJS_INTERNAL("venom_tjs_host_receipts_ptr") uint32_t venom_tjs_host_receipts_ptr(void) { return (uint32_t)(uintptr_t)g_host_receipts; }
VENOM_TJS_INTERNAL("venom_tjs_host_receipts_size") uint32_t venom_tjs_host_receipts_size(void) { return g_host_receipts_size; }
VENOM_TJS_INTERNAL("venom_tjs_host_receipts_hash") uint32_t venom_tjs_host_receipts_hash(void) { return fnv32(g_host_receipts, g_host_receipts_size); }
VENOM_TJS_INTERNAL("venom_tjs_release_acceptance_ptr") uint32_t venom_tjs_release_acceptance_ptr(void) { return (uint32_t)(uintptr_t)g_release_acceptance; }
VENOM_TJS_INTERNAL("venom_tjs_release_acceptance_size") uint32_t venom_tjs_release_acceptance_size(void) { return g_release_acceptance_size; }
VENOM_TJS_INTERNAL("venom_tjs_release_acceptance_hash") uint32_t venom_tjs_release_acceptance_hash(void) { return fnv32(g_release_acceptance, g_release_acceptance_size); }
VENOM_TJS_INTERNAL("venom_tjs_release_accept") uint32_t venom_tjs_release_accept(uint32_t ctx) { uint32_t ok = g_last_commit_hash != 0u && g_last_snapshot_valid != 0u && g_engine_trap_code == 0u; build_release_acceptance_json(ctx, ok); if (!ok) { build_exception_json(ctx, 91u, "release-acceptance", "release acceptance gate rejected execution", g_last_commit_hash, g_last_snapshot_hash); return 0u; } venom_tjs_host_call_dispatch(26u, (uint32_t)(uintptr_t)g_release_acceptance, g_release_acceptance_size); build_commit_audit_json(ctx); return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_commit_audit_ptr") uint32_t venom_tjs_commit_audit_ptr(void) { return (uint32_t)(uintptr_t)g_commit_audit; }
VENOM_TJS_INTERNAL("venom_tjs_commit_audit_size") uint32_t venom_tjs_commit_audit_size(void) { return g_commit_audit_size; }
VENOM_TJS_INTERNAL("venom_tjs_commit_audit_hash") uint32_t venom_tjs_commit_audit_hash(void) { return fnv32(g_commit_audit, g_commit_audit_size); }

VENOM_TJS_INTERNAL("venom_tjs_capability_policy_ptr") uint32_t venom_tjs_capability_policy_ptr(void) { return (uint32_t)(uintptr_t)g_capability_policy_text; }
VENOM_TJS_INTERNAL("venom_tjs_capability_policy_size") uint32_t venom_tjs_capability_policy_size(void) { return cstr_size(g_capability_policy_text); }
VENOM_TJS_INTERNAL("venom_tjs_capability_policy_hash") uint32_t venom_tjs_capability_policy_hash(void) { return fnv32(g_capability_policy_text, cstr_size(g_capability_policy_text)); }
VENOM_TJS_INTERNAL("venom_tjs_host_io_policy_ptr") uint32_t venom_tjs_host_io_policy_ptr(void) { return (uint32_t)(uintptr_t)g_host_io_policy_text; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_policy_size") uint32_t venom_tjs_host_io_policy_size(void) { return cstr_size(g_host_io_policy_text); }
VENOM_TJS_INTERNAL("venom_tjs_host_io_policy_hash") uint32_t venom_tjs_host_io_policy_hash(void) { return fnv32(g_host_io_policy_text, cstr_size(g_host_io_policy_text)); }
VENOM_TJS_INTERNAL("venom_tjs_policy_receipts_ptr") uint32_t venom_tjs_policy_receipts_ptr(void) { return (uint32_t)(uintptr_t)g_policy_receipts; }
VENOM_TJS_INTERNAL("venom_tjs_policy_receipts_size") uint32_t venom_tjs_policy_receipts_size(void) { return g_policy_receipts_size; }
VENOM_TJS_INTERNAL("venom_tjs_policy_receipts_hash") uint32_t venom_tjs_policy_receipts_hash(void) { return fnv32(g_policy_receipts, g_policy_receipts_size); }
VENOM_TJS_INTERNAL("venom_tjs_permission_seal_ptr") uint32_t venom_tjs_permission_seal_ptr(void) { return (uint32_t)(uintptr_t)g_permission_seal; }
VENOM_TJS_INTERNAL("venom_tjs_permission_seal_size") uint32_t venom_tjs_permission_seal_size(void) { return g_permission_seal_size; }
VENOM_TJS_INTERNAL("venom_tjs_permission_seal_hash") uint32_t venom_tjs_permission_seal_hash(void) { return g_permission_seal_size ? fnv32(g_permission_seal, g_permission_seal_size) : g_permission_seal_hash; }
VENOM_TJS_INTERNAL("venom_tjs_capability_check") uint32_t venom_tjs_capability_check(uint32_t ctx, uint32_t capability_id) { (void)ctx; uint32_t ok = capability_allowed(capability_id); build_policy_receipts_json(capability_id, ok, g_last_source_hash); venom_tjs_host_call_dispatch(27u, (uint32_t)(uintptr_t)g_policy_receipts, g_policy_receipts_size); return ok; }
VENOM_TJS_INTERNAL("venom_tjs_permission_seal") uint32_t venom_tjs_permission_seal(uint32_t ctx) { build_capability_ledger_json(ctx); build_permission_seal_json(ctx); build_policy_seal_audit_json(ctx); venom_tjs_host_call_dispatch(29u, (uint32_t)(uintptr_t)g_permission_seal, g_permission_seal_size); venom_tjs_host_call_dispatch(34u, (uint32_t)(uintptr_t)g_policy_seal_audit, g_policy_seal_audit_size); return g_permission_seal_hash; }
VENOM_TJS_INTERNAL("venom_tjs_release_gate_ptr") uint32_t venom_tjs_release_gate_ptr(void) { return (uint32_t)(uintptr_t)g_release_gate; }
VENOM_TJS_INTERNAL("venom_tjs_release_gate_size") uint32_t venom_tjs_release_gate_size(void) { return g_release_gate_size; }
VENOM_TJS_INTERNAL("venom_tjs_release_gate_hash") uint32_t venom_tjs_release_gate_hash(void) { return fnv32(g_release_gate, g_release_gate_size); }
VENOM_TJS_INTERNAL("venom_tjs_release_gate") uint32_t venom_tjs_release_gate(uint32_t ctx) { uint32_t ok = g_release_accepted && g_permission_seal_hash != 0u && g_policy_receipt_count != 0u && g_host_io_denial_count == 0u && g_engine_trap_code == 0u; build_release_gate_json(ctx, ok); if (!ok) { build_exception_json(ctx, 101u, "release-gate", "release gate rejected unsealed or denied TurboJS host I/O policy", g_permission_seal_hash, g_policy_receipt_count); return 0u; } venom_tjs_host_call_dispatch(30u, (uint32_t)(uintptr_t)g_release_gate, g_release_gate_size); return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_decision_ptr") uint32_t venom_tjs_host_io_decision_ptr(void) { return (uint32_t)(uintptr_t)g_host_io_decision; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_decision_size") uint32_t venom_tjs_host_io_decision_size(void) { return g_host_io_decision_size; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_decision_hash") uint32_t venom_tjs_host_io_decision_hash(void) { return fnv32(g_host_io_decision, g_host_io_decision_size); }
VENOM_TJS_INTERNAL("venom_tjs_host_io_request") uint32_t venom_tjs_host_io_request(uint32_t ctx, uint32_t io_class, uint32_t capability_id, uint32_t payload_hash) { uint32_t policy_ok = (io_class == 1u || io_class == 2u) && capability_allowed(capability_id); build_policy_receipts_json(capability_id, policy_ok, payload_hash); build_host_io_decision_json(ctx, io_class, capability_id, policy_ok, payload_hash); if (!policy_ok) { build_host_io_deny_trace_json(ctx, io_class, capability_id, payload_hash, "host I/O rejected by runtime denylist"); build_capability_ledger_json(ctx); venom_tjs_host_call_dispatch(32u, (uint32_t)(uintptr_t)g_host_io_deny_trace, g_host_io_deny_trace_size); return 0u; } build_capability_ledger_json(ctx); venom_tjs_host_call_dispatch(31u, (uint32_t)(uintptr_t)g_host_io_decision, g_host_io_decision_size); venom_tjs_host_call_dispatch(33u, (uint32_t)(uintptr_t)g_capability_ledger, g_capability_ledger_size); return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_deny_trace_ptr") uint32_t venom_tjs_host_io_deny_trace_ptr(void) { return (uint32_t)(uintptr_t)g_host_io_deny_trace; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_deny_trace_size") uint32_t venom_tjs_host_io_deny_trace_size(void) { return g_host_io_deny_trace_size; }
VENOM_TJS_INTERNAL("venom_tjs_host_io_deny_trace_hash") uint32_t venom_tjs_host_io_deny_trace_hash(void) { return fnv32(g_host_io_deny_trace, g_host_io_deny_trace_size); }
VENOM_TJS_INTERNAL("venom_tjs_capability_ledger_ptr") uint32_t venom_tjs_capability_ledger_ptr(void) { return (uint32_t)(uintptr_t)g_capability_ledger; }
VENOM_TJS_INTERNAL("venom_tjs_capability_ledger_size") uint32_t venom_tjs_capability_ledger_size(void) { return g_capability_ledger_size; }
VENOM_TJS_INTERNAL("venom_tjs_capability_ledger_hash") uint32_t venom_tjs_capability_ledger_hash(void) { return g_capability_ledger_hash ? g_capability_ledger_hash : fnv32(g_capability_ledger, g_capability_ledger_size); }
VENOM_TJS_INTERNAL("venom_tjs_policy_seal_audit_ptr") uint32_t venom_tjs_policy_seal_audit_ptr(void) { return (uint32_t)(uintptr_t)g_policy_seal_audit; }
VENOM_TJS_INTERNAL("venom_tjs_policy_seal_audit_size") uint32_t venom_tjs_policy_seal_audit_size(void) { return g_policy_seal_audit_size; }
VENOM_TJS_INTERNAL("venom_tjs_policy_seal_audit_hash") uint32_t venom_tjs_policy_seal_audit_hash(void) { return g_policy_seal_audit_hash ? g_policy_seal_audit_hash : fnv32(g_policy_seal_audit, g_policy_seal_audit_size); }
VENOM_TJS_INTERNAL("venom_tjs_runtime_denylist_ptr") uint32_t venom_tjs_runtime_denylist_ptr(void) { return (uint32_t)(uintptr_t)g_runtime_denylist_text; }
VENOM_TJS_INTERNAL("venom_tjs_runtime_denylist_size") uint32_t venom_tjs_runtime_denylist_size(void) { return cstr_size(g_runtime_denylist_text); }
VENOM_TJS_INTERNAL("venom_tjs_runtime_denylist_hash") uint32_t venom_tjs_runtime_denylist_hash(void) { return fnv32(g_runtime_denylist_text, cstr_size(g_runtime_denylist_text)); }
VENOM_TJS_INTERNAL("venom_tjs_exception_abi") uint32_t venom_tjs_exception_abi(void) { return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_exception_ptr") uint32_t venom_tjs_exception_ptr(void) { return (uint32_t)(uintptr_t)g_exception; }
VENOM_TJS_PUBLIC("venom_tjs_exception_size") uint32_t venom_tjs_exception_size(void) { return g_exception_size; }
VENOM_TJS_PUBLIC("venom_tjs_exception_code") uint32_t venom_tjs_exception_code(void) { return g_exception_code; }
VENOM_TJS_PUBLIC("venom_tjs_exception_message_ptr") uint32_t venom_tjs_exception_message_ptr(void) { return (uint32_t)(uintptr_t)g_exception_message; }
VENOM_TJS_PUBLIC("venom_tjs_exception_message_size") uint32_t venom_tjs_exception_message_size(void) { return g_exception_message_size; }
VENOM_TJS_PUBLIC("venom_tjs_exception_clear") uint32_t venom_tjs_exception_clear(void) { uint32_t code = g_exception_code; clear_exception(); if (g_engine_state == STATE_TRAPPED) g_engine_state = STATE_LOADED; return code; }
VENOM_TJS_INTERNAL("venom_tjs_host_trap_policy_ptr") uint32_t venom_tjs_host_trap_policy_ptr(void) { return (uint32_t)(uintptr_t)g_host_trap_policy; }
VENOM_TJS_INTERNAL("venom_tjs_host_trap_policy_size") uint32_t venom_tjs_host_trap_policy_size(void) { return cstr_size(g_host_trap_policy); }
VENOM_TJS_INTERNAL("venom_tjs_host_trap_policy_hash") uint32_t venom_tjs_host_trap_policy_hash(void) { return fnv32(g_host_trap_policy, cstr_size(g_host_trap_policy)); }

VENOM_TJS_INTERNAL("venom_tjs_source_ptr") uint32_t venom_tjs_source_ptr(void) { return (uint32_t)(uintptr_t)g_source; }
VENOM_TJS_INTERNAL("venom_tjs_source_capacity") uint32_t venom_tjs_source_capacity(void) { return SOURCE_CAP; }
VENOM_TJS_PUBLIC("venom_tjs_compact_result_ptr") uint32_t venom_tjs_compact_result_ptr(void) {
  uint32_t logical[8];
  uint32_t i;
  logical[0] = 0x33524356u; /* VCR3 */
  logical[1] = g_last_status_code;
  logical[2] = g_exception_code;
  logical[3] = g_host_call_count;
  logical[4] = g_console_count;
  logical[5] = g_fallback_required;
  logical[6] = g_result_size;
  logical[7] = g_last_status_code == TJS_STATUS_OK ? 1u : 0u;
  for (i = 0u; i < 8u; ++i) g_compact_result[g_result_field_wire[i]] = logical[i];
  return (uint32_t)(uintptr_t)g_compact_result;
}
VENOM_TJS_PUBLIC("venom_tjs_result_ptr") uint32_t venom_tjs_result_ptr(void) { return (uint32_t)(uintptr_t)g_result; }
VENOM_TJS_PUBLIC("venom_tjs_result_size") uint32_t venom_tjs_result_size(void) { return g_result_size; }
VENOM_TJS_PUBLIC("venom_tjs_execution_record_ptr") uint32_t venom_tjs_execution_record_ptr(void) { return (uint32_t)(uintptr_t)g_record; }
VENOM_TJS_PUBLIC("venom_tjs_execution_record_size") uint32_t venom_tjs_execution_record_size(void) { return g_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_last_context") uint32_t venom_tjs_last_context(void) { return g_last_context; }
VENOM_TJS_INTERNAL("venom_tjs_last_source_size") uint32_t venom_tjs_last_source_size(void) { return g_last_source_size; }
VENOM_TJS_INTERNAL("venom_tjs_last_source_hash") uint32_t venom_tjs_last_source_hash(void) { return g_last_source_hash; }
VENOM_TJS_INTERNAL("venom_tjs_console_count") uint32_t venom_tjs_console_count(void) { return g_console_count; }
VENOM_TJS_INTERNAL("venom_tjs_console_callback_abi") uint32_t venom_tjs_console_callback_abi(void) { return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_console_event_ptr") uint32_t venom_tjs_console_event_ptr(void) { return (uint32_t)(uintptr_t)g_console; }
VENOM_TJS_PUBLIC("venom_tjs_console_event_size") uint32_t venom_tjs_console_event_size(void) { return g_console_size; }
VENOM_TJS_PUBLIC("venom_tjs_console_event_count") uint32_t venom_tjs_console_event_count(void) { return g_console_count; }
VENOM_TJS_PUBLIC("venom_tjs_console_clear") uint32_t venom_tjs_console_clear(void) { uint32_t n = g_console_count; g_console_count = 0u; g_console_size = 0u; return n; }
VENOM_TJS_PUBLIC("venom_tjs_fallback_required") uint32_t venom_tjs_fallback_required(void) { return g_fallback_required; }
VENOM_TJS_PUBLIC("venom_tjs_backend_kind") uint32_t venom_tjs_backend_kind(void) { return g_backend_kind; }
VENOM_TJS_PUBLIC("venom_tjs_backend_ready") uint32_t venom_tjs_backend_ready(void) { return g_backend_ready; }
VENOM_TJS_PUBLIC("venom_tjs_real_engine_candidate") uint32_t venom_tjs_real_engine_candidate(void) { return g_real_engine_candidate; }


VENOM_TJS_PUBLIC("venom_tjs_status_code") uint32_t venom_tjs_status_code(void) { return g_last_status_code; }
VENOM_TJS_PUBLIC("venom_tjs_status_record_ptr") uint32_t venom_tjs_status_record_ptr(void) { return (uint32_t)(uintptr_t)g_status_record; }
VENOM_TJS_PUBLIC("venom_tjs_status_record_size") uint32_t venom_tjs_status_record_size(void) { return g_status_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_status_record_hash") uint32_t venom_tjs_status_record_hash(void) { return fnv32(g_status_record, g_status_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_runtime_limits_ptr") uint32_t venom_tjs_runtime_limits_ptr(void) { return (uint32_t)(uintptr_t)g_runtime_limits_text; }
VENOM_TJS_INTERNAL("venom_tjs_runtime_limits_size") uint32_t venom_tjs_runtime_limits_size(void) { return cstr_size(g_runtime_limits_text); }
VENOM_TJS_INTERNAL("venom_tjs_runtime_limits_hash") uint32_t venom_tjs_runtime_limits_hash(void) { return fnv32(g_runtime_limits_text, cstr_size(g_runtime_limits_text)); }
VENOM_TJS_INTERNAL("venom_tjs_context_report_ptr") uint32_t venom_tjs_context_report_ptr(uint32_t ctx) { build_context_report_json(ctx); return (uint32_t)(uintptr_t)g_context_report; }
VENOM_TJS_INTERNAL("venom_tjs_context_report_size") uint32_t venom_tjs_context_report_size(void) { return g_context_report_size; }
VENOM_TJS_INTERNAL("venom_tjs_context_report_hash") uint32_t venom_tjs_context_report_hash(void) { return fnv32(g_context_report, g_context_report_size); }
VENOM_TJS_PUBLIC("venom_tjs_bytecode_record_hash32") uint32_t venom_tjs_bytecode_record_hash32(void) { return g_bytecode_record_hash32; }
VENOM_TJS_PUBLIC("venom_tjs_bytecode_payload_size") uint32_t venom_tjs_bytecode_payload_size(void) { return g_bytecode_payload_size; }
VENOM_TJS_PUBLIC("venom_tjs_bytecode_expected_source_hash32") uint32_t venom_tjs_bytecode_expected_source_hash32(void) { return g_bytecode_expected_source_hash32; }
VENOM_TJS_INTERNAL("venom_tjs_backend_contract_ptr") uint32_t venom_tjs_backend_contract_ptr(void) { return (uint32_t)(uintptr_t)g_backend_contract_text; }
VENOM_TJS_INTERNAL("venom_tjs_backend_contract_size") uint32_t venom_tjs_backend_contract_size(void) { return cstr_size(g_backend_contract_text); }
VENOM_TJS_INTERNAL("venom_tjs_backend_contract_hash") uint32_t venom_tjs_backend_contract_hash(void) { return fnv32(g_backend_contract_text, cstr_size(g_backend_contract_text)); }
VENOM_TJS_PUBLIC("venom_tjs_release_status") uint32_t venom_tjs_release_status(void) { return g_last_status_code == TJS_STATUS_OK ? 1u : 0u; }
VENOM_TJS_PUBLIC("venom_tjs_interpreter_ready") uint32_t venom_tjs_interpreter_ready(void) { return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_contract_ptr") uint32_t venom_tjs_interpreter_contract_ptr(void) { return (uint32_t)(uintptr_t)g_interpreter_contract_text; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_contract_size") uint32_t venom_tjs_interpreter_contract_size(void) { return cstr_size(g_interpreter_contract_text); }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_contract_hash") uint32_t venom_tjs_interpreter_contract_hash(void) { return fnv32(g_interpreter_contract_text, cstr_size(g_interpreter_contract_text)); }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_dispatch_count") uint32_t venom_tjs_interpreter_dispatch_count(void) { return g_interpreter_dispatch_count; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_opcode_count") uint32_t venom_tjs_interpreter_opcode_count(void) { return g_interpreter_opcode_count; }
VENOM_TJS_INTERNAL("venom_tjs_global_slot_count") uint32_t venom_tjs_global_slot_count(void) { return g_global_slot_count; }
VENOM_TJS_INTERNAL("venom_tjs_last_global_write_hash") uint32_t venom_tjs_last_global_write_hash(void) { return g_last_global_write_hash; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_semantic_pass_count") uint32_t venom_tjs_interpreter_semantic_pass_count(void) { return g_interpreter_semantic_pass_count; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_property_write_count") uint32_t venom_tjs_interpreter_property_write_count(void) { return g_interpreter_property_write_count; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_builtin_probe_count") uint32_t venom_tjs_interpreter_builtin_probe_count(void) { return g_interpreter_builtin_probe_count; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_console_call_count") uint32_t venom_tjs_interpreter_console_call_count(void) { return g_interpreter_console_call_count; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_semantic_record_ptr") uint32_t venom_tjs_interpreter_semantic_record_ptr(void) { return (uint32_t)(uintptr_t)g_semantic_record; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_semantic_record_size") uint32_t venom_tjs_interpreter_semantic_record_size(void) { return g_semantic_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_interpreter_semantic_record_hash") uint32_t venom_tjs_interpreter_semantic_record_hash(void) { return fnv32(g_semantic_record, g_semantic_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_global_slot_record_ptr") uint32_t venom_tjs_global_slot_record_ptr(void) { return (uint32_t)(uintptr_t)g_global_slot_record; }
VENOM_TJS_INTERNAL("venom_tjs_global_slot_record_size") uint32_t venom_tjs_global_slot_record_size(void) { return g_global_slot_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_global_slot_record_hash") uint32_t venom_tjs_global_slot_record_hash(void) { return fnv32(g_global_slot_record, g_global_slot_record_size); }
VENOM_TJS_PUBLIC("venom_tjs_upstream_turbojs_ready") uint32_t venom_tjs_upstream_turbojs_ready(void) { return 1u; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_parity_record_ptr") uint32_t venom_tjs_upstream_parity_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_parity_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_parity_record_size") uint32_t venom_tjs_upstream_parity_record_size(void) { return g_upstream_parity_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_parity_record_hash") uint32_t venom_tjs_upstream_parity_record_hash(void) { return fnv32(g_upstream_parity_record, g_upstream_parity_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_feature_count") uint32_t venom_tjs_upstream_feature_count(void) { return g_upstream_feature_count; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_builtin_count") uint32_t venom_tjs_upstream_builtin_count(void) { return g_upstream_builtin_count; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_object_model_score") uint32_t venom_tjs_upstream_object_model_score(void) { return g_upstream_object_model_score; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_exception_model_score") uint32_t venom_tjs_upstream_exception_model_score(void) { return g_upstream_exception_model_score; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_module_model_score") uint32_t venom_tjs_upstream_module_model_score(void) { return g_upstream_module_model_score; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_object_record_ptr") uint32_t venom_tjs_upstream_object_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_object_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_object_record_size") uint32_t venom_tjs_upstream_object_record_size(void) { return g_upstream_object_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_object_record_hash") uint32_t venom_tjs_upstream_object_record_hash(void) { return g_upstream_object_record_hash ? g_upstream_object_record_hash : fnv32(g_upstream_object_record, g_upstream_object_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_exception_record_ptr") uint32_t venom_tjs_upstream_exception_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_exception_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_exception_record_size") uint32_t venom_tjs_upstream_exception_record_size(void) { return g_upstream_exception_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_exception_record_hash") uint32_t venom_tjs_upstream_exception_record_hash(void) { return g_upstream_exception_record_hash ? g_upstream_exception_record_hash : fnv32(g_upstream_exception_record, g_upstream_exception_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_module_record_ptr") uint32_t venom_tjs_upstream_module_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_module_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_module_record_size") uint32_t venom_tjs_upstream_module_record_size(void) { return g_upstream_module_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_module_record_hash") uint32_t venom_tjs_upstream_module_record_hash(void) { return g_upstream_module_record_hash ? g_upstream_module_record_hash : fnv32(g_upstream_module_record, g_upstream_module_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_runtime_bridge_score") uint32_t venom_tjs_upstream_runtime_bridge_score(void) { return g_upstream_runtime_bridge_score; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_bytecode_semantics_record_ptr") uint32_t venom_tjs_upstream_bytecode_semantics_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_bytecode_semantics_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_bytecode_semantics_record_size") uint32_t venom_tjs_upstream_bytecode_semantics_record_size(void) { return g_upstream_bytecode_semantics_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_bytecode_semantics_record_hash") uint32_t venom_tjs_upstream_bytecode_semantics_record_hash(void) { return g_upstream_bytecode_semantics_hash ? g_upstream_bytecode_semantics_hash : fnv32(g_upstream_bytecode_semantics_record, g_upstream_bytecode_semantics_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_bytecode_semantics_score") uint32_t venom_tjs_upstream_bytecode_semantics_score(void) { return g_upstream_bytecode_semantics_score; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_lexical_scope_record_ptr") uint32_t venom_tjs_upstream_lexical_scope_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_lexical_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_lexical_scope_record_size") uint32_t venom_tjs_upstream_lexical_scope_record_size(void) { return g_upstream_lexical_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_lexical_scope_record_hash") uint32_t venom_tjs_upstream_lexical_scope_record_hash(void) { return g_upstream_lexical_record_hash ? g_upstream_lexical_record_hash : fnv32(g_upstream_lexical_record, g_upstream_lexical_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_closure_record_ptr") uint32_t venom_tjs_upstream_closure_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_closure_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_closure_record_size") uint32_t venom_tjs_upstream_closure_record_size(void) { return g_upstream_closure_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_closure_record_hash") uint32_t venom_tjs_upstream_closure_record_hash(void) { return g_upstream_closure_record_hash ? g_upstream_closure_record_hash : fnv32(g_upstream_closure_record, g_upstream_closure_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_iterator_record_ptr") uint32_t venom_tjs_upstream_iterator_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_iterator_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_iterator_record_size") uint32_t venom_tjs_upstream_iterator_record_size(void) { return g_upstream_iterator_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_iterator_record_hash") uint32_t venom_tjs_upstream_iterator_record_hash(void) { return g_upstream_iterator_record_hash ? g_upstream_iterator_record_hash : fnv32(g_upstream_iterator_record, g_upstream_iterator_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_intrinsic_record_ptr") uint32_t venom_tjs_upstream_intrinsic_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_intrinsic_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_intrinsic_record_size") uint32_t venom_tjs_upstream_intrinsic_record_size(void) { return g_upstream_intrinsic_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_intrinsic_record_hash") uint32_t venom_tjs_upstream_intrinsic_record_hash(void) { return g_upstream_intrinsic_record_hash ? g_upstream_intrinsic_record_hash : fnv32(g_upstream_intrinsic_record, g_upstream_intrinsic_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_intrinsic_semantics_score") uint32_t venom_tjs_upstream_intrinsic_semantics_score(void) { return g_upstream_intrinsic_semantics_score; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_property_descriptor_record_ptr") uint32_t venom_tjs_upstream_property_descriptor_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_property_descriptor_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_property_descriptor_record_size") uint32_t venom_tjs_upstream_property_descriptor_record_size(void) { return g_upstream_property_descriptor_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_property_descriptor_record_hash") uint32_t venom_tjs_upstream_property_descriptor_record_hash(void) { return g_upstream_property_descriptor_record_hash ? g_upstream_property_descriptor_record_hash : fnv32(g_upstream_property_descriptor_record, g_upstream_property_descriptor_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_prototype_chain_record_ptr") uint32_t venom_tjs_upstream_prototype_chain_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_prototype_chain_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_prototype_chain_record_size") uint32_t venom_tjs_upstream_prototype_chain_record_size(void) { return g_upstream_prototype_chain_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_prototype_chain_record_hash") uint32_t venom_tjs_upstream_prototype_chain_record_hash(void) { return g_upstream_prototype_chain_record_hash ? g_upstream_prototype_chain_record_hash : fnv32(g_upstream_prototype_chain_record, g_upstream_prototype_chain_record_size); }
VENOM_TJS_INTERNAL("venom_tjs_upstream_call_frame_record_ptr") uint32_t venom_tjs_upstream_call_frame_record_ptr(void) { return (uint32_t)(uintptr_t)g_upstream_call_frame_record; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_call_frame_record_size") uint32_t venom_tjs_upstream_call_frame_record_size(void) { return g_upstream_call_frame_record_size; }
VENOM_TJS_INTERNAL("venom_tjs_upstream_call_frame_record_hash") uint32_t venom_tjs_upstream_call_frame_record_hash(void) { return g_upstream_call_frame_record_hash ? g_upstream_call_frame_record_hash : fnv32(g_upstream_call_frame_record, g_upstream_call_frame_record_size); }

VENOM_TJS_PUBLIC("venom_tjs_context_alloc") uint32_t venom_tjs_context_alloc(void) { uint32_t id = g_context_counter++; g_engine_state = STATE_LOADED; return make_context(id) ? id : 0u; }
VENOM_TJS_PUBLIC("venom_tjs_context_free") uint32_t venom_tjs_context_free(uint32_t ctx) { struct TjsContextAccount* rec = find_context(ctx); if (!rec) return 0u; rec->alive = 0u; rec->heap_used = 0u; rec->script_bytes = 0u; rec->interrupt_ticks = 0u; rec->pending_jobs = 0u; if (g_active_script_ctx == ctx) { if (g_script_buffer_size) venom_tjs_secure_zero(g_source, venom_tjs_bounded_size(g_script_buffer_size, SOURCE_CAP)); g_active_script_ctx = 0u; g_script_buffer_size = 0u; g_script_buffer_expected_hash = 0u; } if (g_bridge_active_ctx == ctx) { if (g_bridge_input_size) venom_tjs_secure_zero(g_bridge_input, venom_tjs_bounded_size(g_bridge_input_size, BRIDGE_INPUT_CAP)); if (g_bridge_result_size) venom_tjs_secure_zero(g_bridge_result, venom_tjs_bounded_size(g_bridge_result_size, BRIDGE_RESULT_CAP)); g_bridge_input_size = 0u; g_bridge_result_size = 0u; g_bridge_active_ctx = 0u; }
#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
  if (g_upstream_tjs_context) { JS_FreeContext(g_upstream_tjs_context); g_upstream_tjs_context = 0; }
  if (g_upstream_tjs_runtime) { JS_FreeRuntime(g_upstream_tjs_runtime); g_upstream_tjs_runtime = 0; }
#endif
  return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_context_set_limits") uint32_t venom_tjs_context_set_limits(uint32_t ctx, uint32_t heap_limit, uint32_t stack_limit) { struct TjsContextAccount* rec = make_context(ctx); if (!rec) { g_last_status_code = TJS_STATUS_BAD_CONTEXT; build_status_record_json(ctx, g_last_status_code, "context", 0u, 0u); return 0u; } rec->heap_limit = heap_limit ? heap_limit : DEFAULT_HEAP_LIMIT; rec->stack_limit = stack_limit ? stack_limit : DEFAULT_STACK_LIMIT; if (rec->heap_limit > DEFAULT_HEAP_LIMIT) rec->heap_limit = DEFAULT_HEAP_LIMIT; if (rec->stack_limit > DEFAULT_STACK_LIMIT) rec->stack_limit = DEFAULT_STACK_LIMIT; if (rec->heap_used > rec->heap_limit) { g_last_status_code = TJS_STATUS_ALLOC_LIMIT; build_status_record_json(ctx, g_last_status_code, "context", rec->heap_used, rec->heap_limit); return 0u; } g_last_status_code = TJS_STATUS_OK; build_status_record_json(ctx, g_last_status_code, "context", rec->heap_limit, rec->stack_limit); g_engine_state = STATE_CONFIGURED; return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_context_set_execution_limits") uint32_t venom_tjs_context_set_execution_limits(uint32_t ctx, uint32_t interrupt_budget, uint32_t pending_job_limit) {
  struct TjsContextAccount* rec = make_context(ctx);
  if (!rec) return 0u;
  rec->interrupt_budget = interrupt_budget ? interrupt_budget : DEFAULT_INTERRUPT_BUDGET;
  rec->pending_job_limit = pending_job_limit ? pending_job_limit : DEFAULT_PENDING_JOB_LIMIT;
  if (rec->interrupt_budget > 10000000u) rec->interrupt_budget = 10000000u;
  if (rec->pending_job_limit > 4096u) rec->pending_job_limit = 4096u;
  return 1u;
}
VENOM_TJS_PUBLIC("venom_tjs_context_heap_limit") uint32_t venom_tjs_context_heap_limit(uint32_t ctx) { struct TjsContextAccount* rec = find_context(ctx); return rec ? rec->heap_limit : 0u; }
VENOM_TJS_PUBLIC("venom_tjs_context_heap_used") uint32_t venom_tjs_context_heap_used(uint32_t ctx) { struct TjsContextAccount* rec = find_context(ctx); return rec ? rec->heap_used : 0u; }
VENOM_TJS_PUBLIC("venom_tjs_context_stack_limit") uint32_t venom_tjs_context_stack_limit(uint32_t ctx) { struct TjsContextAccount* rec = find_context(ctx); return rec ? rec->stack_limit : 0u; }

VENOM_TJS_PUBLIC("venom_tjs_script_buffer_alloc") uint32_t venom_tjs_script_buffer_alloc(uint32_t ctx, uint32_t size) {
  struct TjsContextAccount* rec = make_context(ctx);
  if (!rec || size == 0u || size > SOURCE_CAP || size > rec->heap_limit) { g_last_status_code = !rec ? TJS_STATUS_BAD_CONTEXT : TJS_STATUS_ALLOC_LIMIT; build_status_record_json(ctx, g_last_status_code, "script-buffer", size, 0u); build_exception_json(ctx, 40u, "script-buffer", size == 0u ? "zero-size script buffer rejected" : "script buffer allocation limit exceeded", size, 0u); return 0u; }
  if (g_script_buffer_size) venom_tjs_secure_zero(g_source, venom_tjs_bounded_size(g_script_buffer_size, SOURCE_CAP));
  g_script_buffer_expected_hash = 0u;
  rec->heap_used = size;
  rec->script_bytes = size;
  g_script_buffer_size = size;
  g_active_script_ctx = rec->id;
  ++g_script_buffer_alloc_count;
  g_last_status_code = TJS_STATUS_OK;
  build_status_record_json(ctx, g_last_status_code, "script-buffer", size, fnv32(g_source, 0u));
  g_engine_state = STATE_LOADED;
  return (uint32_t)(uintptr_t)g_source;
}
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_ptr") uint32_t venom_tjs_script_buffer_ptr(void) { return (uint32_t)(uintptr_t)g_source; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_size") uint32_t venom_tjs_script_buffer_size(void) { return g_script_buffer_size; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_capacity") uint32_t venom_tjs_script_buffer_capacity(void) { return SOURCE_CAP; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_set_expected_hash") uint32_t venom_tjs_script_buffer_set_expected_hash(uint32_t ctx, uint32_t hash) { (void)ctx; g_script_buffer_expected_hash = hash; return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_expected_hash") uint32_t venom_tjs_script_buffer_expected_hash(void) { return g_script_buffer_expected_hash; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_alloc_count") uint32_t venom_tjs_script_buffer_alloc_count(void) { return g_script_buffer_alloc_count; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_free_count") uint32_t venom_tjs_script_buffer_free_count(void) { return g_script_buffer_free_count; }
VENOM_TJS_PUBLIC("venom_tjs_script_buffer_free") uint32_t venom_tjs_script_buffer_free(uint32_t ctx, uint32_t ptr) {
  /* Compatibility-only release.  g_source is a static arena; the JS host
     wipes the validated range before calling any lifecycle operation.  Keep
     this export write-free so corrupted post-execution accounting cannot turn
     cleanup into an out-of-bounds trap. */
  if (ptr != (uint32_t)(uintptr_t)g_source || g_active_script_ctx != ctx) return 0u;
  g_active_script_ctx = 0u;
  g_script_buffer_size = 0u;
  g_script_buffer_expected_hash = 0u;
  ++g_script_buffer_free_count;
  return 1u;
}


VENOM_TJS_PUBLIC("venom_tjs_bridge_abi") uint32_t venom_tjs_bridge_abi(void) { return 1u; }
VENOM_TJS_PUBLIC("venom_tjs_bridge_input_alloc") uint32_t venom_tjs_bridge_input_alloc(uint32_t ctx, uint32_t size) {
  struct TjsContextAccount* rec = find_context(ctx);
  if (!rec || size == 0u || size > BRIDGE_INPUT_CAP) return 0u;
  if (g_bridge_input_size) venom_tjs_secure_zero(g_bridge_input, venom_tjs_bounded_size(g_bridge_input_size, BRIDGE_INPUT_CAP));
  if (g_bridge_result_size) venom_tjs_secure_zero(g_bridge_result, venom_tjs_bounded_size(g_bridge_result_size, BRIDGE_RESULT_CAP));
  g_bridge_input_size = size;
  g_bridge_result_size = 0u;
  g_bridge_active_ctx = ctx;
  return (uint32_t)(uintptr_t)g_bridge_input;
}
VENOM_TJS_PUBLIC("venom_tjs_bridge_input_capacity") uint32_t venom_tjs_bridge_input_capacity(void) { return BRIDGE_INPUT_CAP; }
VENOM_TJS_PUBLIC("venom_tjs_bridge_result_ptr") uint32_t venom_tjs_bridge_result_ptr(void) { return (uint32_t)(uintptr_t)g_bridge_result; }
VENOM_TJS_PUBLIC("venom_tjs_bridge_result_size") uint32_t venom_tjs_bridge_result_size(void) { return g_bridge_result_size; }
VENOM_TJS_PUBLIC("venom_tjs_bridge_release") uint32_t venom_tjs_bridge_release(uint32_t ctx) {
  if (g_bridge_active_ctx != ctx) return 0u;
  if (g_bridge_input_size) venom_tjs_secure_zero(g_bridge_input, venom_tjs_bounded_size(g_bridge_input_size, BRIDGE_INPUT_CAP));
  if (g_bridge_result_size) venom_tjs_secure_zero(g_bridge_result, venom_tjs_bounded_size(g_bridge_result_size, BRIDGE_RESULT_CAP));
  g_bridge_input_size = 0u;
  g_bridge_result_size = 0u;
  g_bridge_active_ctx = 0u;
  return 1u;
}

#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
static uint32_t venom_tjs_bridge_copy_json_result(uint32_t ctx, JSValue value) {
  JSValue json = JS_JSONStringify(g_upstream_tjs_context, value, JS_UNDEFINED, JS_UNDEFINED);
  if (JS_IsException(json)) {
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, g_bridge_input_size, 0u);
    JS_FreeValue(g_upstream_tjs_context, json);
    return 0u;
  }
  size_t size = 0u;
  const char* text = JS_ToCStringLen(g_upstream_tjs_context, &size, json);
  if (!text || size == 0u || size > BRIDGE_RESULT_CAP) {
    if (text) JS_FreeCString(g_upstream_tjs_context, text);
    JS_FreeValue(g_upstream_tjs_context, json);
    build_exception_json(ctx, 101u, "bridge", "bridge result is not representable by binary-json-v2", (uint32_t)size, 0u);
    return 0u;
  }
  memcpy(g_bridge_result, text, size);
  g_bridge_result_size = (uint32_t)size;
  JS_FreeCString(g_upstream_tjs_context, text);
  JS_FreeValue(g_upstream_tjs_context, json);
  return 1u;
}
#endif

VENOM_TJS_PUBLIC("venom_tjs_bridge_invoke") uint32_t venom_tjs_bridge_invoke(uint32_t ctx, uint32_t size) {
  struct TjsContextAccount* rec = find_context(ctx);
  if (!rec || g_bridge_active_ctx != ctx || size == 0u || size > g_bridge_input_size) return 0u;
  if (g_bridge_result_size) venom_tjs_secure_zero(g_bridge_result, venom_tjs_bounded_size(g_bridge_result_size, BRIDGE_RESULT_CAP));
  g_bridge_result_size = 0u;
#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
  if (!venom_tjs_ensure_upstream_runtime(rec)) {
    build_exception_json(ctx, 90u, "bridge", "failed to initialize upstream TurboJS runtime", size, 0u);
    return 0u;
  }
  JSValue envelope = JS_ParseJSON(g_upstream_tjs_context, (const char*)g_bridge_input, size, "<venom-bridge-request>");
  if (JS_IsException(envelope)) {
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, size, 0u);
    JS_FreeValue(g_upstream_tjs_context, envelope);
    return 0u;
  }
  JSValue candidate = JS_GetPropertyStr(g_upstream_tjs_context, envelope, "candidate");
  JSValue args = JS_GetPropertyStr(g_upstream_tjs_context, envelope, "args");
  const char* candidate_name = JS_ToCString(g_upstream_tjs_context, candidate);
  if (!candidate_name || !JS_IsArray(args)) {
    if (candidate_name) JS_FreeCString(g_upstream_tjs_context, candidate_name);
    JS_FreeValue(g_upstream_tjs_context, candidate);
    JS_FreeValue(g_upstream_tjs_context, args);
    JS_FreeValue(g_upstream_tjs_context, envelope);
    build_exception_json(ctx, 102u, "bridge", "invalid bridge request envelope", size, 0u);
    return 0u;
  }
  JSValue global = JS_GetGlobalObject(g_upstream_tjs_context);
  JSValue registry = JS_GetPropertyStr(g_upstream_tjs_context, global, "__venomProtectedBridge");
  JSValue function = JS_IsObject(registry) ? JS_GetPropertyStr(g_upstream_tjs_context, registry, candidate_name) : JS_UNDEFINED;
  JS_FreeCString(g_upstream_tjs_context, candidate_name);
  if (!JS_IsFunction(g_upstream_tjs_context, function)) {
    JS_FreeValue(g_upstream_tjs_context, function);
    JS_FreeValue(g_upstream_tjs_context, registry);
    JS_FreeValue(g_upstream_tjs_context, global);
    JS_FreeValue(g_upstream_tjs_context, candidate);
    JS_FreeValue(g_upstream_tjs_context, args);
    JS_FreeValue(g_upstream_tjs_context, envelope);
    build_exception_json(ctx, 103u, "bridge", "protected bridge candidate is not registered", size, 0u);
    return 0u;
  }
  uint32_t argc = 0u;
  JSValue length = JS_GetPropertyStr(g_upstream_tjs_context, args, "length");
  JS_ToUint32(g_upstream_tjs_context, &argc, length);
  JS_FreeValue(g_upstream_tjs_context, length);
  if (argc > 64u) argc = 64u;
  JSValue argv[64];
  for (uint32_t i = 0u; i < argc; ++i) argv[i] = JS_GetPropertyUint32(g_upstream_tjs_context, args, i);
  venom_tjs_begin_execution_budget(rec);
  JSValue result = JS_Call(g_upstream_tjs_context, function, JS_UNDEFINED, (int)argc, argv);
  for (uint32_t i = 0u; i < argc; ++i) JS_FreeValue(g_upstream_tjs_context, argv[i]);
  JS_FreeValue(g_upstream_tjs_context, function);
  JS_FreeValue(g_upstream_tjs_context, registry);
  JS_FreeValue(g_upstream_tjs_context, global);
  JS_FreeValue(g_upstream_tjs_context, candidate);
  JS_FreeValue(g_upstream_tjs_context, args);
  JS_FreeValue(g_upstream_tjs_context, envelope);
  if (JS_IsException(result)) {
    venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, size, 0u);
    JS_FreeValue(g_upstream_tjs_context, result);
    venom_tjs_end_execution_budget();
    return 0u;
  }

  /* Protected exports may be declared async. Resolve their Promise inside the
   * TurboJS runtime before serializing the bridge result. Serializing the raw
   * Promise would produce an empty object and silently discard the real value. */
  if (JS_IsPromise(result)) {
    for (;;) {
      const JSPromiseStateEnum state = JS_PromiseState(g_upstream_tjs_context, result);
      if (state == JS_PROMISE_FULFILLED) {
        JSValue settled = JS_PromiseResult(g_upstream_tjs_context, result);
        JS_FreeValue(g_upstream_tjs_context, result);
        result = settled;
        break;
      }
      if (state == JS_PROMISE_REJECTED) {
        JSValue reason = JS_PromiseResult(g_upstream_tjs_context, result);
        JS_FreeValue(g_upstream_tjs_context, result);
        JS_Throw(g_upstream_tjs_context, reason);
        venom_tjs_copy_exception_message(g_upstream_tjs_context, ctx, size, 0u);
        venom_tjs_end_execution_budget();
        return 0u;
      }
      if (state != JS_PROMISE_PENDING) {
        JS_FreeValue(g_upstream_tjs_context, result);
        build_exception_json(ctx, 105u, "bridge", "invalid protected Promise state", size, 0u);
        venom_tjs_end_execution_budget();
        return 0u;
      }
      if (!venom_tjs_finish_upstream_jobs(ctx, size, 0u)) {
        JS_FreeValue(g_upstream_tjs_context, result);
        venom_tjs_end_execution_budget();
        return 0u;
      }
      if (JS_PromiseState(g_upstream_tjs_context, result) == JS_PROMISE_PENDING) {
        JS_FreeValue(g_upstream_tjs_context, result);
        build_exception_json(ctx, 106u, "bridge", "protected Promise did not settle", size, 0u);
        venom_tjs_end_execution_budget();
        return 0u;
      }
    }
  } else if (!venom_tjs_finish_upstream_jobs(ctx, size, 0u)) {
    JS_FreeValue(g_upstream_tjs_context, result);
    venom_tjs_end_execution_budget();
    return 0u;
  }

  const uint32_t copied = venom_tjs_bridge_copy_json_result(ctx, result);
  JS_FreeValue(g_upstream_tjs_context, result);
  venom_tjs_end_execution_budget();
  return copied;
#else
  build_exception_json(ctx, 104u, "bridge", "upstream TurboJS bridge executor unavailable", size, 0u);
  return 0u;
#endif
}
#if !defined(__EMSCRIPTEN__)
uint32_t venom_tjs_test_load_bytes(uint32_t ctx, const unsigned char* bytes, uint32_t size) {
  struct TjsContextAccount* rec = find_context(ctx);
  if (!rec || !bytes || size == 0u || size > SOURCE_CAP) return 0u;
  memcpy(g_source, bytes, size);
  g_script_buffer_size = size;
  g_active_script_ctx = ctx;
  rec->heap_used = size;
  rec->script_bytes = size;
  return 1u;
}
const char* venom_tjs_test_exception_message(void) { return (const char*)g_exception_message; }
uint32_t venom_tjs_test_source_byte(uint32_t index) { return index < SOURCE_CAP ? (uint32_t)g_source[index] : 0u; }
uint32_t venom_tjs_test_bridge_load_bytes(uint32_t ctx, const unsigned char* bytes, uint32_t size) {
  if (!find_context(ctx) || !bytes || size == 0u || size > BRIDGE_INPUT_CAP) return 0u;
  if (g_bridge_input_size) venom_tjs_secure_zero(g_bridge_input, venom_tjs_bounded_size(g_bridge_input_size, BRIDGE_INPUT_CAP));
  memcpy(g_bridge_input, bytes, size);
  g_bridge_input_size = size;
  g_bridge_result_size = 0u;
  g_bridge_active_ctx = ctx;
  return 1u;
}
uint32_t venom_tjs_test_bridge_result_byte(uint32_t index) { return index < g_bridge_result_size ? (uint32_t)g_bridge_result[index] : 0u; }
#endif

static uint32_t execute_native_bytecode_with_interpreter(uint32_t ctx,
                                                         uint32_t record_size,
                                                         uint32_t payload_offset,
                                                         uint32_t payload_size,
                                                         uint32_t source_size,
                                                         uint32_t source_hash32,
                                                         uint32_t record_hash) {
  struct TjsContextAccount* rec = make_context(ctx);
  if (!rec || record_size == 0u || record_size > SOURCE_CAP || payload_size == 0u || payload_size > rec->heap_limit) {
    g_result_size = 0u;
    g_last_status_code = !rec ? TJS_STATUS_BAD_CONTEXT : TJS_STATUS_EXECUTION_TRAP;
    build_status_record_json(ctx, g_last_status_code, "bytecode-dispatch", record_size, record_hash);
    build_exception_json(ctx, !rec ? 46u : 47u, "bytecode", !rec ? "TurboJS context not found" : "native TurboJS bytecode limit exceeded", record_size, record_hash);
    build_script_record_json(ctx, source_size, source_hash32, 0u);
    build_eval_result_json(ctx, 0u, source_size, source_hash32, 0u, 0u);
    build_module_record_json(ctx, 0u, source_size);
    build_json(g_result, &g_result_size, RESULT_CAP, 0u, ctx, source_size, source_hash32, 0u, 0u, 0u, rec ? rec->heap_used : 0u, rec ? rec->heap_limit : 0u);
    g_record_size = g_result_size;
    for (uint32_t i = 0; i < g_result_size && i < RECORD_CAP; ++i) g_record[i] = g_result[i];
    return 0u;
  }
  clear_exception();
  g_last_status_code = TJS_STATUS_OK;
  build_status_record_json(ctx, g_last_status_code, "bytecode-dispatch", record_size, record_hash);
  build_script_record_json(ctx, source_size, source_hash32, 0u);
  venom_tjs_checkpoint_create(ctx, 1u);
  venom_tjs_host_call_dispatch(11u, (uint32_t)(uintptr_t)g_script_record, g_script_record_size);
  g_engine_state = STATE_EXECUTING;
  ++g_interpreter_dispatch_count;
  g_interpreter_opcode_count = 0u; /* Native bytecode remains opaque to the bridge. */
  g_last_interpreter_hash = record_hash;
  build_module_record_json(ctx, 1u, source_size);
  if (g_script_buffer_size == 0u) g_script_buffer_size = record_size;
  rec->heap_used = g_script_buffer_size;
  rec->script_bytes += source_size;
  ++rec->executions;
  g_last_context = ctx;
  g_last_source_size = source_size;
  g_last_source_hash = source_hash32;
  if (!venom_tjs_execute_bytecode_with_upstream(ctx, payload_offset, payload_size, record_hash, rec)) {
    build_eval_result_json(ctx, 0u, source_size, source_hash32, 0u, g_console_count);
    build_json(g_result, &g_result_size, RESULT_CAP, 0u, ctx, source_size, source_hash32, 0u, g_console_count ? 1u : 0u, g_console_count, rec->heap_used, rec->heap_limit);
    g_record_size = g_result_size;
    for (uint32_t i = 0; i < g_result_size && i < RECORD_CAP; ++i) g_record[i] = g_result[i];
    return 0u;
  }
  g_fallback_required = 0u;
  g_engine_state = STATE_LOADED;
  build_eval_result_json(ctx, 1u, source_size, source_hash32, 0u, g_console_count);
  venom_tjs_checkpoint_create(ctx, 4u);
  venom_tjs_snapshot_capture(ctx, 4u);
  venom_tjs_snapshot_validate(ctx, g_last_snapshot_hash);
  venom_tjs_replay_advance(ctx, 4u);
  venom_tjs_execution_commit(ctx, g_last_checkpoint_id);
  venom_tjs_release_accept(ctx);
  venom_tjs_capability_check(ctx, 1u);
  venom_tjs_permission_seal(ctx);
  venom_tjs_release_gate(ctx);
  venom_tjs_host_call_dispatch(12u, (uint32_t)(uintptr_t)g_script_record, g_script_record_size);
  venom_tjs_host_call_dispatch(13u, (uint32_t)(uintptr_t)g_eval_result, g_eval_result_size);
  build_json(g_result, &g_result_size, RESULT_CAP, 1u, ctx, source_size, source_hash32, 0u, g_console_count ? 1u : 0u, g_console_count, rec->heap_used, rec->heap_limit);
  build_json(g_record, &g_record_size, RECORD_CAP, 1u, ctx, source_size, source_hash32, 0u, g_console_count ? 1u : 0u, g_console_count, rec->heap_used, rec->heap_limit);
  return 1u;
}

VENOM_TJS_PUBLIC("venom_tjs_execute_source") uint32_t venom_tjs_execute_source(uint32_t ctx, uint32_t size) {
  struct TjsContextAccount* rec = make_context(ctx);
  uint32_t hash = size <= SOURCE_CAP ? fnv32(g_source, size) : 0u;
  if (!rec || size == 0u || size > SOURCE_CAP || size > rec->heap_limit || (g_script_buffer_expected_hash && hash != g_script_buffer_expected_hash)) {
    g_result_size = 0u;
    g_last_status_code = (g_script_buffer_expected_hash && hash != g_script_buffer_expected_hash) ? TJS_STATUS_HASH_MISMATCH : TJS_STATUS_EXECUTION_TRAP;
    build_status_record_json(ctx, g_last_status_code, "execute-source", size, hash);
    build_exception_json(ctx, size == 0u ? 41u : (g_script_buffer_expected_hash && hash != g_script_buffer_expected_hash ? 42u : 43u), "execution", size == 0u ? "zero-size script execution rejected" : (g_script_buffer_expected_hash && hash != g_script_buffer_expected_hash ? "script buffer hash mismatch" : "TurboJS source or heap limit exceeded"), size, hash);
    build_script_record_json(ctx, size, hash, 0u);
    build_eval_result_json(ctx, 0u, size, hash, 0u, 0u);
    venom_tjs_checkpoint_create(ctx, 5u);
    venom_tjs_snapshot_capture(ctx, 5u);
    venom_tjs_snapshot_validate(ctx, g_last_snapshot_hash);
    build_module_record_json(ctx, 0u, size);
    build_json(g_result, &g_result_size, RESULT_CAP, 0u, ctx, size, hash, 0u, 0u, 0u, rec ? rec->heap_used : 0u, rec ? rec->heap_limit : 0u);
    g_record_size = g_result_size;
    for (uint32_t i = 0; i < g_result_size && i < RECORD_CAP; ++i) g_record[i] = g_result[i];
    return 0u;
  }
  clear_exception();
  g_last_status_code = TJS_STATUS_OK;
  build_status_record_json(ctx, g_last_status_code, "execute-source", size, hash);
  build_script_record_json(ctx, size, hash, 0u);
  venom_tjs_checkpoint_create(ctx, 1u);
  venom_tjs_host_call_dispatch(11u, (uint32_t)(uintptr_t)g_script_record, g_script_record_size);
  g_engine_state = STATE_EXECUTING;
  build_module_record_json(ctx, 1u, size);
  if (g_script_buffer_size == 0u) g_script_buffer_size = size;
  rec->heap_used = g_script_buffer_size;
  rec->script_bytes += size;
  rec->executions++;
  g_last_context = ctx;
  g_last_source_size = size;
  g_last_source_hash = hash;
  if (!venom_tjs_execute_source_with_upstream(ctx, size, hash, rec)) {
    build_eval_result_json(ctx, 0u, size, hash, count_lines(g_source, size), g_console_count);
    build_json(g_result, &g_result_size, RESULT_CAP, 0u, ctx, size, hash, count_lines(g_source, size), g_console_count ? 1u : 0u, g_console_count, rec->heap_used, rec->heap_limit);
    g_record_size = g_result_size;
    for (uint32_t i = 0; i < g_result_size && i < RECORD_CAP; ++i) g_record[i] = g_result[i];
    return 0u;
  }
  if (contains_word(g_source, size, "console", 7u)) { venom_tjs_host_io_request(ctx, 1u, 1u, hash); if (g_console_count < MAX_CONSOLE_EVENTS) ++g_console_count; build_console_json(ctx, g_console_count); venom_tjs_host_call_dispatch(1u, (uint32_t)(uintptr_t)g_console, g_console_size); build_console_capture_json(ctx); venom_tjs_host_call_dispatch(14u, (uint32_t)(uintptr_t)g_console_capture, g_console_capture_size); }
  if (contains_word(g_source, size, "fetch", 5u)) { venom_tjs_host_io_request(ctx, 3u, 7u, hash); }
  g_fallback_required = 0u;
  const uint32_t lines = count_lines(g_source, size);
  g_engine_state = STATE_LOADED;
  build_eval_result_json(ctx, 1u, size, g_last_source_hash, lines, g_console_count);
  venom_tjs_checkpoint_create(ctx, 4u);
  venom_tjs_snapshot_capture(ctx, 4u);
  venom_tjs_snapshot_validate(ctx, g_last_snapshot_hash);
  venom_tjs_replay_advance(ctx, 4u);
  venom_tjs_execution_commit(ctx, g_last_checkpoint_id);
  venom_tjs_release_accept(ctx);
  venom_tjs_capability_check(ctx, 1u);
  venom_tjs_permission_seal(ctx);
  venom_tjs_release_gate(ctx);
  venom_tjs_host_call_dispatch(12u, (uint32_t)(uintptr_t)g_script_record, g_script_record_size);
  venom_tjs_host_call_dispatch(13u, (uint32_t)(uintptr_t)g_eval_result, g_eval_result_size);
  build_json(g_result, &g_result_size, RESULT_CAP, 1u, ctx, size, g_last_source_hash, lines, g_console_count ? 1u : 0u, g_console_count, rec->heap_used, rec->heap_limit);
  build_json(g_record, &g_record_size, RECORD_CAP, 1u, ctx, size, g_last_source_hash, lines, g_console_count ? 1u : 0u, g_console_count, rec->heap_used, rec->heap_limit);
  return 1u;
}


VENOM_TJS_PUBLIC("venom_tjs_bytecode_validate") uint32_t venom_tjs_bytecode_validate(uint32_t ctx, uint32_t size) {
  const uint32_t record_hash = size <= SOURCE_CAP ? fnv32(g_source, size) : 0u;
  g_bytecode_record_hash32 = record_hash;
  g_bytecode_payload_size = 0u;
  g_bytecode_expected_source_hash32 = 0u;
  if (g_script_buffer_expected_hash && record_hash != g_script_buffer_expected_hash) {
    g_last_validation_code = TJS_STATUS_HASH_MISMATCH;
    g_last_status_code = TJS_STATUS_HASH_MISMATCH;
    build_status_record_json(ctx, g_last_status_code, "bytecode-validate", size, record_hash);
    return g_last_validation_code;
  }
  uint32_t payload_offset = 0u, payload_size = 0u, source_size = 0u, source_hash32 = 0u;
  uint32_t module_count = 0u, entry_index = 0u;
  if (validate_vtjsbc03_record(size, &payload_offset, &payload_size, &source_size, &source_hash32)) {
    g_bytecode_payload_size = payload_size;
    g_bytecode_expected_source_hash32 = source_hash32;
  } else if (validate_vtjsmb04_record(size, &module_count, &entry_index)) {
    g_bytecode_payload_size = size;
    g_bytecode_expected_source_hash32 = module_count ^ (entry_index << 16u);
  } else {
    g_last_validation_code = TJS_STATUS_INVALID_BYTECODE;
    g_last_status_code = TJS_STATUS_INVALID_BYTECODE;
    build_status_record_json(ctx, g_last_status_code, "bytecode-validate", size, record_hash);
    return g_last_validation_code;
  }
  g_last_validation_code = TJS_STATUS_OK;
  g_last_status_code = TJS_STATUS_OK;
  build_status_record_json(ctx, g_last_status_code, "bytecode-validate", size, record_hash);
  return TJS_STATUS_OK;
}

VENOM_TJS_PUBLIC("venom_tjs_execute_bytecode") uint32_t venom_tjs_execute_bytecode(uint32_t ctx, uint32_t size) {
  const uint32_t record_hash = size <= SOURCE_CAP ? fnv32(g_source, size) : 0u;
  const uint32_t validation = venom_tjs_bytecode_validate(ctx, size);
  if (validation != TJS_STATUS_OK) {
    build_exception_json(ctx,
                         validation == TJS_STATUS_HASH_MISMATCH ? 44u : 45u,
                         "bytecode",
                         validation == TJS_STATUS_HASH_MISMATCH ? "protected bytecode record hash mismatch" : "invalid protected TurboJS bytecode record",
                         size,
                         record_hash);
    return 0u;
  }
  g_script_buffer_expected_hash = 0u;
  g_bytecode_record_executed = 0u;
  g_bytecode_record_size = size;
  g_host_effect_increment_example = 0u;

  uint32_t module_count = 0u, entry_index = 0u;
  if (validate_vtjsmb04_record(size, &module_count, &entry_index)) {
    struct TjsContextAccount* rec = make_context(ctx);
    if (!rec) return 0u;
#ifdef VENOM_ENABLE_UPSTREAM_TJS_WASM
    const uint32_t ok = venom_tjs_execute_module_bundle_with_upstream(ctx, size, record_hash, rec);
#else
    const uint32_t ok = 0u;
    build_exception_json(ctx, 90u, "turbojs-module-bundle", "upstream TurboJS unavailable", size, record_hash);
#endif
    g_bytecode_record_source_hash32 = module_count ^ (entry_index << 16u);
    g_bytecode_payload_size = size;
    g_bytecode_expected_source_hash32 = g_bytecode_record_source_hash32;
    g_bytecode_record_executed = ok ? 1u : 0u;
    g_last_status_code = ok ? TJS_STATUS_OK : TJS_STATUS_EXECUTION_TRAP;
    build_status_record_json(ctx, g_last_status_code, "module-bundle-execute", size, record_hash);
    return ok;
  }

  uint32_t payload_offset = 0u, payload_size = 0u, source_size = 0u, source_hash32 = 0u;
  if (!validate_vtjsbc03_record(size, &payload_offset, &payload_size, &source_size, &source_hash32)) {
    g_last_validation_code = TJS_STATUS_INVALID_BYTECODE;
    g_last_status_code = TJS_STATUS_INVALID_BYTECODE;
    build_status_record_json(ctx, g_last_status_code, "bytecode-read", size, record_hash);
    build_exception_json(ctx, 45u, "bytecode", "invalid native TurboJS bytecode record", size, record_hash);
    return 0u;
  }
  g_bytecode_record_source_hash32 = source_hash32;
  g_bytecode_payload_size = payload_size;
  g_bytecode_expected_source_hash32 = source_hash32;
  const uint32_t ok = execute_native_bytecode_with_interpreter(ctx,
                                                               size,
                                                               payload_offset,
                                                               payload_size,
                                                               source_size,
                                                               source_hash32,
                                                               record_hash);
  g_bytecode_record_executed = ok ? 1u : 0u;
  g_last_status_code = ok ? TJS_STATUS_OK : TJS_STATUS_EXECUTION_TRAP;
  build_status_record_json(ctx, g_last_status_code, "bytecode-execute", size, record_hash);
  return ok;
}

VENOM_TJS_PUBLIC("venom_tjs_bytecode_record_executed") uint32_t venom_tjs_bytecode_record_executed(void) { return g_bytecode_record_executed; }
VENOM_TJS_PUBLIC("venom_tjs_bytecode_record_size") uint32_t venom_tjs_bytecode_record_size(void) { return g_bytecode_record_size; }
VENOM_TJS_PUBLIC("venom_tjs_bytecode_record_source_hash32") uint32_t venom_tjs_bytecode_record_source_hash32(void) { return g_bytecode_record_source_hash32; }

VENOM_TJS_PUBLIC("venom_tjs_module_execute") uint32_t venom_tjs_module_execute(uint32_t ctx, uint32_t module_id, uint32_t size) {
  uint32_t ok = venom_tjs_execute_source(ctx, size);
  if (ok) {
    ++g_module_execution_count;
    venom_tjs_host_call_dispatch(7u, (uint32_t)(uintptr_t)g_module_record, g_module_record_size);
    venom_tjs_host_call_dispatch(8u, (uint32_t)(uintptr_t)g_module_record, g_module_record_size);
    venom_tjs_host_call_dispatch(10u, (uint32_t)(uintptr_t)g_module_cache_state, g_module_cache_state_size);
  }
  build_module_cache_json(ctx, module_id, ok);
  build_resolver_audit_json(ctx, module_id, ok);
  return ok;
}
