#pragma once

#include "pipeline/build.hpp"
#include "pipeline/build_package_metadata.hpp"
#include "pipeline/js.hpp"
#include "core/profile.hpp"
#include "turbojs/bytecode.hpp"

#include <string>
#include <vector>

namespace venom::compiler::build_runtime_detail {

std::string make_host_bridge_metadata(const Profile& profile,
                                      const std::string& runtime_mode,
                                      const JsBridge& js_bridge);

std::string make_fetch_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_timer_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_event_queue_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_async_host_queue_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_script_isolation_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_script_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_chunk_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_engine_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_engine_module_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name, const std::string& wasm_asset_name);

std::string make_turbojs_wasm_runtime_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& wasm_asset_name);

std::string make_turbojs_wasm_execution_metadata(const Profile& profile,
                                                const BuildOptions& options,
                                                const build_package_detail::ReleaseBuildPolicy& policy,
                                                const JsBridge& bridge,
                                                const std::string& wasm_asset_name);

std::string make_turbojs_source_transfer_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_console_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_script_engine_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_execution_journal_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_checkpoint_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_replay_cursor_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_resume_state_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_determinism_audit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_snapshot_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_snapshot_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_replay_validation_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_determinism_ledger_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_audit_seal_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_execution_commit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_rollback_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_call_receipts_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_release_acceptance_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_commit_audit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_capability_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_io_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_permission_seal_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_policy_receipts_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_release_gate_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_io_decision_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_io_deny_trace_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_capability_ledger_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_policy_seal_audit_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_runtime_denylist_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_context_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_host_capabilities_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_adapter_diagnostics_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name);

std::string make_turbojs_execution_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_result_bridge_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_fallback_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_engine_backend_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_native_parity_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& probe);

std::string make_turbojs_execution_mode_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_runtime_abi_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_imports_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_heap_limits_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_script_buffer_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_console_abi_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_parity_probe_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& native_eval);

std::string make_turbojs_release_fallback_metadata(const Profile& profile, const std::string& runtime_mode);

std::vector<venom::turbojs::BytecodeChunkRecord> make_turbojs_bytecode_records(const JsBridge& bridge, bool redact_metadata);

std::string make_turbojs_bytecode_manifest_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_module_resolver_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_exception_abi_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_trap_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_execution_lifecycle_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_script_buffer_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_context_limit_policy_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_host_call_dispatch_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_parity_contract_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_release_failclosed_metadata(const Profile& profile, const std::string& runtime_mode);

std::size_t turbojs_module_count(const JsBridge& bridge);

std::string make_turbojs_module_graph_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_module_execution_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_module_cache_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_resolver_audit_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_interop_fallback_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_execution_pipeline_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_script_records_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge);

std::string make_turbojs_eval_results_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_console_capture_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_turbojs_failure_reports_metadata(const Profile& profile, const std::string& runtime_mode);

std::string make_runtime_policy_metadata(const Profile& profile, const std::string& runtime_mode);

} // namespace venom::compiler::build_runtime_detail
