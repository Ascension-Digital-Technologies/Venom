#pragma once

#include "venom/pipeline/build.hpp"
#include "venom/internal/pipeline/build_package_metadata.hpp"
#include "venom/pipeline/js.hpp"
#include "venom/core/profile.hpp"
#include "venom/quickjs/bytecode.hpp"

#include <string>
#include <vector>

namespace venom::compiler::build_runtime_detail {

std::string make_host_bridge_metadata(const Profile& profile,
                                      const std::string& runtime_mode,
                                      const JsBridge& js_bridge);

std::string make_fetch_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_timer_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_event_queue_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_async_host_queue_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_script_isolation_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_script_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_chunk_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_engine_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_engine_module_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name, const std::string& wasm_asset_name);

std::string make_quickjs_wasm_runtime_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& wasm_asset_name);

std::string make_quickjs_wasm_execution_metadata(const Profile& profile,
                                                const BuildOptions& options,
                                                const build_package_detail::ReleaseBuildPolicy& policy,
                                                const JsBridge& bridge,
                                                const std::string& wasm_asset_name);

std::string make_quickjs_source_transfer_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_console_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_script_engine_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_execution_journal_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_checkpoint_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_replay_cursor_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_resume_state_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_determinism_audit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_snapshot_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_snapshot_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_replay_validation_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_determinism_ledger_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_audit_seal_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_execution_commit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_rollback_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_call_receipts_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_release_acceptance_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_commit_audit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_capability_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_io_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_permission_seal_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_policy_receipts_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_release_gate_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_io_decision_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_io_deny_trace_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_capability_ledger_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_policy_seal_audit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_runtime_denylist_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_context_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_host_capabilities_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_adapter_diagnostics_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name);

std::string make_quickjs_execution_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_result_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_fallback_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_engine_backend_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_native_parity_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& probe);

std::string make_quickjs_execution_mode_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_runtime_abi_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_imports_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_heap_limits_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_script_buffer_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_console_abi_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_parity_probe_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& native_eval);

std::string make_quickjs_release_fallback_metadata(const Profile& profile, const std::string& runtime_mode);

std::vector<venom::quickjs::BytecodeChunkRecord> make_quickjs_bytecode_records(const JsBridge& bridge, bool redact_metadata);

std::string make_quickjs_bytecode_manifest_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_module_resolver_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_exception_abi_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_trap_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_execution_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_script_buffer_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_context_limit_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_host_call_dispatch_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_parity_contract_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_release_failclosed_metadata(const Profile& profile, const std::string& runtime_mode);

std::size_t quickjs_module_count(const JsBridge& bridge);

std::string make_quickjs_module_graph_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_module_execution_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_module_cache_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_resolver_audit_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_interop_fallback_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_execution_pipeline_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_script_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_quickjs_eval_results_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_console_capture_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_quickjs_failure_reports_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_runtime_policy_metadata(const Profile& profile, const std::string& runtime_mode);

} // namespace venom::compiler::build_runtime_detail
