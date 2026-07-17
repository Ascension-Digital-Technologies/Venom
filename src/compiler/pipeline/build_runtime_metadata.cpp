#include "compiler/pipeline/build_runtime_metadata.hpp"
#include "compiler/pipeline/build_support.hpp"
#include "compiler/pipeline/assets.hpp"
#include "package/hash.hpp"

#include "quickjs/abi.hpp"
#include "quickjs/bridge.hpp"
#include "quickjs/engine.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace venom::compiler::build_runtime_detail {

using namespace build_detail;
using namespace build_package_detail;

std::string make_host_bridge_metadata(const Profile& profile,
                                      const std::string& runtime_mode,
                                      const JsBridge& js_bridge) {
  auto contains_any = [](const std::string& text, std::initializer_list<const char*> needles) {
    for (const char* needle : needles) {
      if (text.find(needle) != std::string::npos) return true;
    }
    return false;
  };

  bool has_scripts = !js_bridge.chunks.empty();
  bool needs_fetch = false;
  bool needs_timers = false;
  bool needs_events = false;
  for (const auto& chunk : js_bridge.chunks) {
    const std::string& code = chunk.code;
    needs_fetch = needs_fetch || contains_any(code, {"fetch(", "XMLHttpRequest", "WebSocket("});
    needs_timers = needs_timers || contains_any(code, {"setTimeout(", "clearTimeout(", "setInterval(", "clearInterval("});
    needs_events = needs_events || contains_any(code, {"addEventListener(", "removeEventListener(", ".onclick", ".onsubmit", ".onchange", ".oninput"});
  }
  // Script execution itself requires the event queue only when an event surface is present.
  // Static/script-free sites therefore receive the smallest bridge contract.
  const std::uint32_t capability_count = 6u + (needs_fetch ? 3u : 0u) + (needs_timers ? 2u : 0u) + (needs_events ? 2u : 0u);

  std::ostringstream out;
  out << "VENOM_HOST_BRIDGE_V14\n"
      << "version=14\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "host_call_count=" << capability_count << "\n"
      << "capability_count=" << capability_count << "\n"
      << "capability_model=application-specialized-schema-v1\n"
      << "unknown_host_call=deny\n"
      << "generic_global_lookup=false\n"
      << "generic_property_traversal=false\n"
      << "generic_method_invoke=false\n"
      << "source_strings_cross_boundary=false\n"
      << "function_objects_cross_boundary=false\n"
      << "application_specialized=true\n"
      << "scripts_present=" << (has_scripts ? "true" : "false") << "\n"
      << "fetch_enabled=" << (needs_fetch ? "true" : "false") << "\n"
      << "timers_enabled=" << (needs_timers ? "true" : "false") << "\n"
      << "events_enabled=" << (needs_events ? "true" : "false") << "\n"
      << "max_request_bytes=65536\n"
      << "max_response_bytes=1048576\n"
      << "max_string_bytes=32768\n"
      << "max_array_items=4096\n"
      << "max_object_depth=16\n"
      << "max_outstanding_async=256\n";

  auto capability = [&](std::uint32_t id, const char* name, const char* request, const char* response,
                        std::uint32_t max_request, std::uint32_t max_response, const char* surface) {
    out << "capability\t" << id << "\t" << name << "\t" << request << "\t" << response
        << "\t" << max_request << "\t" << max_response << "\n";
    out << "host_call\t" << id << "\t" << name << "\t" << surface << "\t" << runtime_mode << "\n";
  };

  capability(1u, "dom.createElement", "string:tag", "object:handle", 1024u, 64u, "dom");
  capability(2u, "dom.setAttribute", "handle,string:name,string:value", "void", 33792u, 0u, "dom");
  capability(3u, "dom.appendChild", "handle:parent,handle:child", "void", 16u, 0u, "dom");
  capability(4u, "event.bindInline", "handle,string:event,u32:binding", "void", 1040u, 0u, "events");
  capability(5u, "asset.resolve", "string:path", "string:url", 32768u, 32768u, "assets");
  capability(6u, "route.navigate", "string:path,u32:mode", "void", 32772u, 0u, "routes");
  if (needs_fetch) {
    capability(7u, "fetch.request", "string:url,string:method,bytes:body", "u32:request_id", 65536u, 4u, "network");
    capability(8u, "fetch.response", "u32:request_id,u32:status,bytes:body", "void", 1048576u, 0u, "network");
    capability(9u, "fetch.error", "u32:request_id,string:error", "void", 32772u, 0u, "network");
  }
  if (needs_timers) {
    capability(10u, "timer.schedule", "u32:delay,u32:token", "void", 8u, 0u, "timers");
    capability(11u, "timer.cancel", "u32:token", "void", 4u, 0u, "timers");
  }
  if (needs_events) {
    capability(12u, "event.queue", "u32:binding,bytes:event", "void", 65536u, 0u, "events");
    capability(13u, "event.dispatch", "u32:binding,bytes:event", "void", 65536u, 0u, "events");
  }
  return out.str();
}

std::string make_fetch_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_FETCH_BRIDGE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "request_host_call=fetch.request\n"
      << "response_host_call=fetch.response\n"
      << "error_host_call=fetch.error\n"
      << "queue=async-host-queue.vahq\n"
      << "cache_policy=browser-default\n"
      << "credentials_policy=same-origin\n"
      << "body_bridge=uint8array\n"
      << "text_response=true\n"
      << "json_response=true\n";
  return out.str();
}

std::string make_timer_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_TIMER_BRIDGE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "schedule_host_call=timer.schedule\n"
      << "cancel_host_call=timer.cancel\n"
      << "queue=async-host-queue.vahq\n"
      << "clock=browser-event-loop\n"
      << "timer_id_bits=32\n"
      << "max_active=128\n"
      << "max_scheduled_per_route=512\n"
      << "minimum_delay_ms=0\n"
      << "wasm_response_boundary=planned\n"
      << "quickjs_timer_boundary=planned\n";
  return out.str();
}

std::string make_event_queue_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_EVENT_QUEUE_V1\n"
      << "version=1\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "queue_host_call=event.queue\n"
      << "dispatch_host_call=event.dispatch\n"
      << "inline_event_bindings=true\n"
      << "max_records=256\n"
      << "max_dispatches_per_route=1024\n"
      << "record_payload=eventName|attrName|route|targetTag\n"
      << "wasm_event_boundary=dom-op-bindEvent\n"
      << "quickjs_event_boundary=planned\n";
  return out.str();
}

std::string make_quickjs_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_BRIDGE_V9\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=engine-backend-select\n"
      << "call_host_call=quickjs.call\n"
      << "result_host_call=quickjs.result\n"
      << "queue=async-host-queue.vahq\n"
      << "script_isolation=route-scoped\n"
      << "bytecode_input=chunk-records\n"
      << "chunk_metadata=quickjs-chunks.vqbc\n"
      << "capability_policy=script-policy.vscp\n"
      << "request_record=quickjs.call\n"
      << "result_record=quickjs.result\n"
      << "wasm_engine=module-loader\n"
      << "native_engine=compile-time-probe\n"
      << "engine_metadata=quickjs-engine.vqse\n"
      << "script_engine_policy=script-engine-policy.vsep\n"
      << "legacy_fallback_policy=module-fallback-policy\n"
      << "context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "host_capabilities=host-capabilities.vhcap\n"
      << "adapter_diagnostics=quickjs-adapter-diagnostics.vqad\n"
      << "wasm_runtime=quickjs-wasm-runtime.vqwr\n"
      << "source_transfer=quickjs-source-transfer.vqst\n"
      << "console_bridge=quickjs-console-bridge.vqcb\n"
      << "execution_records=quickjs-execution-records.vqer\n"
      << "result_bridge=quickjs-result-bridge.vqrb\n"
      << "fallback_policy=quickjs-fallback-policy.vqfp\n"
      << "runtime_abi=quickjs-runtime-abi.vqra\n"
      << "host_imports=quickjs-host-imports.vqhi\n"
      << "heap_limits=quickjs-heap-limits.vqhl\n"
      << "script_buffer=quickjs-script-buffer.vqsb\n"
      << "console_callback_abi=quickjs-console-abi.vqca\n"
      << "parity_probe=quickjs-parity-probe.vqpp\n"
      << "execution_lifecycle=quickjs-execution-lifecycle.vqel\n"
      << "script_buffer_policy=quickjs-script-buffer-policy.vqsp\n"
      << "context_limit_policy=quickjs-context-limit-policy.vqlp\n"
      << "host_call_dispatch=quickjs-host-call-dispatch.vqhd\n"
      << "parity_contract=quickjs-parity-contract.vqpc\n"
      << "release_failclosed=quickjs-release-failclosed.vqfc\n"
      << "module_graph=quickjs-module-graph.vqmg\n"
      << "module_execution=quickjs-module-execution.vqmx\n"
      << "module_cache=quickjs-module-cache.vqmc\n"
      << "resolver_audit=quickjs-resolver-audit.vqra\n"
      << "interop_fallback=quickjs-interop-fallback.vqif\n"
      << "execution_pipeline=quickjs-execution-pipeline.vqxp\n"
      << "script_records=quickjs-script-records.vqsr\n"
      << "eval_results=quickjs-eval-results.vqev\n"
      << "console_capture=quickjs-console-capture.vqcc\n"
      << "failure_reports=quickjs-failure-reports.vqfr\n";
  return out.str();
}

std::string make_async_host_queue_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_ASYNC_HOST_QUEUE_V10\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "request_id_bits=32\n"
      << "state=pending|fulfilled|rejected\n"
      << "max_pending=128\n"
      << "max_completed=128\n"
      << "max_host_calls_per_route=8192\n"
      << "max_concurrent_fetches=16\n"
      << "max_dom_handles=4096\n"
      << "max_fetch_response_bytes=1048576\n"
      << "max_fetch_request_bytes=65536\n"
      << "max_timer_delay_ms=86400000\n"
      << "max_route_lifetime_ms=86400000\n"
      << "teardown=cancel-timers|abort-fetches|reject-pending|destroy-contexts\n"
      << "host_call=fetch.request\n"
      << "host_call=fetch.response\n"
      << "host_call=fetch.error\n"
      << "host_call=timer.schedule\n"
      << "host_call=timer.cancel\n"
      << "host_call=event.queue\n"
      << "host_call=event.dispatch\n"
      << "host_call=quickjs.call\n"
      << "host_call=quickjs.result\n"
      << "host_call=quickjs.chunk\n"
      << "host_call=quickjs.bytecodeBoundary\n"
      << "host_call=script.scopeCreate\n"
      << "host_call=script.chunkStart\n"
      << "host_call=script.chunkFinish\n"
      << "host_call=script.policyCheck\n"
      << "host_call=quickjs.engineBoot\n"
      << "host_call=quickjs.contextCreate\n"
      << "host_call=quickjs.executeChunk\n"
      << "host_call=quickjs.executionResult\n"
      << "host_call=quickjs.contextReuse\n"
      << "host_call=quickjs.contextDestroy\n"
      << "host_call=quickjs.hostCapabilityInject\n"
      << "host_call=quickjs.adapterStatus\n"
      << "host_call=quickjs.moduleContextCreate\n"
      << "host_call=quickjs.moduleContextDestroy\n"
      << "host_call=quickjs.executionRecord\n"
      << "host_call=quickjs.resultBridge\n"
      << "host_call=quickjs.fallbackPolicyCheck\n"
      << "host_call=quickjs.consoleEvent\n"
      << "host_call=quickjs.consoleFlush\n"
      << "host_call=quickjs.wasmResultDecode\n"
      << "host_call=quickjs.executionSnapshot\n"
      << "host_call=script.fallbackDecision\n"
      << "host_call=script.engineFallback\n"
      << "host_call=script.capabilityBind\n"
      << "host_call=quickjs.abiTable\n"
      << "host_call=quickjs.contextSetLimits\n"
      << "host_call=quickjs.contextHeapAccounting\n"
      << "host_call=quickjs.scriptBufferAlloc\n"
      << "host_call=quickjs.scriptBufferFree\n"
      << "host_call=quickjs.consoleCallbackAbi\n"
      << "host_call=quickjs.consoleEventDrain\n"
      << "host_call=quickjs.hostImportTable\n"
      << "host_call=quickjs.wasmParityProbe\n"
      << "host_call=quickjs.releaseFallbackDeny\n"
      << "timer_bridge=enabled\n"
      << "event_queue=enabled\n"
      << "quickjs_bridge=engine-module\n"
      << "script_isolation=route-scoped\n"
      << "wasm_request_boundary=planned\n"
      << "quickjs_request_boundary=engine-records\n";
  return out.str();
}


std::string make_script_isolation_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_SCRIPT_ISOLATION_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "mode=route-scoped\n"
      << "scope_key=route|source|order\n"
      << "global_policy=shared-browser-global-with-route-wrapper\n"
      << "document_access=bridge-parameter\n"
      << "window_access=bridge-parameter\n"
      << "quickjs_boundary=engine-bootstrap\n"
      << "engine_context=route-scoped\n"
      << "context_lifecycle=tracked-reuse-destroy\n"
      << "engine_fallback=deny-host-js-source-eval\n"
      << "adapter_context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\t" << protected_source_size(profile, chunk) << "\t" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_script_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_SCRIPT_POLICY_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "default_capabilities=console,document,window,fetch,timers,events,quickjs-engine-bootstrap,host-capability-injection\n"
      << "remote_scripts=vendored-package-bytecode\n"
      << "module_scripts=engine-fallback-wrapper\n"
      << "inline_scripts=route-wrapper\n"
      << "policy_check_host_call=script.policyCheck\n"
      << "chunk_start_host_call=script.chunkStart\n"
      << "chunk_finish_host_call=script.chunkFinish\n"
      << "engine_fallback=deny-host-js-source-eval\n"
      << "adapter_context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "policy\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tcapabilities=console,document,window,fetch,timers,events\n";
  }
  return out.str();
}

std::string make_quickjs_chunk_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const auto protected_chunk_count = static_cast<std::size_t>(std::count_if(
      bridge.chunks.begin(), bridge.chunks.end(),
      [](const JsChunk& chunk) { return (chunk.flags & JsChunkBrowser) == 0u; }));
  const auto protected_count = protected_chunk_count + (bridge.bridge_registry_bytecode.empty() ? 0u : 1u);
  std::ostringstream out;
  out << "VENOM_QUICKJS_CHUNKS_V7\n"
      << "version=7\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "mode=engine-input\n"
      << "bytecode_provider=source-chunks-v1\n"
      << "bytecode_chunk_section=route-scoped-script-chunk.*.vjsb\n"
      << "request_host_call=quickjs.chunk\n"
      << "bytecode_boundary_host_call=quickjs.bytecodeBoundary\n"
      << "engine_execute_host_call=quickjs.executeChunk\n"
      << "engine_fallback=deny-host-js-source-eval\n"
      << "adapter_context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "chunk_count=" << protected_count << "\n";
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u) continue;
    out << "qjs_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  if (!bridge.bridge_registry_bytecode.empty()) {
    out << "qjs_chunk\tregistry\tprotected-bridge-registry\tnative-quickjs-object"
        << "\tbytes=" << bridge.bridge_registry_bytecode.size() << "\tflags=protected,isolated\n";
  }
  return out.str();
}


std::string make_quickjs_engine_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_ENGINE_V7\n"
      << "version=7\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=engine-backend-replacement-path\n"
      << "engine=quickjs-backend-selector\n"
      << "browser_engine=quickjs-runtime-wasm-replacement-candidate\n"
      << "native_engine=quickjs-compile-time-probe\n"
      << "context_model=route-scoped-reusable\n"
      << "context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "host_capability_bindings=host-capabilities.vhcap\n"
      << "bytecode_chunks=route-scoped-script-chunk.*.vjsb\n"
      << "capability_policy=script-engine-policy.vsep\n"
      << "context_create_host_call=quickjs.contextCreate\n"
      << "execute_host_call=quickjs.executeChunk\n"
      << "result_host_call=quickjs.executionResult\n"
      << "fallback_host_call=quickjs.moduleFallback\n"
      << "wasm_runtime=quickjs-runtime.wasm\n"
      << "wasm_runtime_asset=quickjs-runtime.wasm\n"
      << "source_transfer=quickjs-source-transfer.vqst\n"
      << "console_bridge=quickjs-console-bridge.vqcb\n"
      << "module_graph=quickjs-module-graph.vqmg\n"
      << "module_execution=quickjs-module-execution.vqmx\n"
      << "module_cache=quickjs-module-cache.vqmc\n"
      << "resolver_audit=quickjs-resolver-audit.vqra\n"
      << "interop_fallback=quickjs-interop-fallback.vqif\n"
      << "execution_pipeline=quickjs-execution-pipeline.vqxp\n"
      << "script_records=quickjs-script-records.vqsr\n"
      << "eval_results=quickjs-eval-results.vqev\n"
      << "console_capture=quickjs-console-capture.vqcc\n"
      << "failure_reports=quickjs-failure-reports.vqfr\n"
      << "snapshot_policy=quickjs-snapshot-policy.vqsk\n"
      << "snapshot_records=quickjs-snapshot-records.vqsn\n"
      << "replay_validation=quickjs-replay-validation.vqrv\n"
      << "determinism_ledger=quickjs-determinism-ledger.vqdl\n"
      << "audit_seal=quickjs-audit-seal.vqas\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "engine_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_quickjs_engine_module_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge, const std::string& engine_asset_name, const std::string& wasm_asset_name) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_ENGINE_MODULE_V7\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "asset_name=" << engine_asset_name << "\n"
      << "wasm_asset_name=" << wasm_asset_name << "\n"
      << "module_format=esm\n"
      << "loader=dynamic-import\n"
      << "export=createVenomQuickJsEngineModule\n"
      << "execution_mode=quickjs-wasm-abi12-upstream-global-host-api-shims\n"
      << "context_model=route-scoped-reusable\n"
      << "context_lifecycle=quickjs-context-lifecycle.vqcl\n"
      << "host_capability_bindings=host-capabilities.vhcap\n"
      << "fallback=fail-closed-no-host-js\n"
      << "bytecode_chunks=route-scoped-script-chunk.*.vjsb\n"
      << "wasm_loader=fetch-instantiate\n"
      << "interpreter_dispatch=enabled\n"
      << "source_decode_fallback=denied-in-protect\n"
      << "bytecode_transfer_api=venom_qjs_script_buffer_alloc|venom_qjs_script_buffer_ptr|venom_qjs_bytecode_validate|venom_qjs_execute_bytecode|venom_qjs_interpreter_dispatch_count|venom_qjs_result_ptr\n"
      << "console_bridge=quickjs-console-bridge.vqcb\n"
      << "module_graph=quickjs-module-graph.vqmg\n"
      << "module_execution=quickjs-module-execution.vqmx\n"
      << "module_cache=quickjs-module-cache.vqmc\n"
      << "resolver_audit=quickjs-resolver-audit.vqra\n"
      << "interop_fallback=quickjs-interop-fallback.vqif\n"
      << "execution_pipeline=quickjs-execution-pipeline.vqxp\n"
      << "script_records=quickjs-script-records.vqsr\n"
      << "eval_results=quickjs-eval-results.vqev\n"
      << "console_capture=quickjs-console-capture.vqcc\n"
      << "failure_reports=quickjs-failure-reports.vqfr\n"
      << "snapshot_policy=quickjs-snapshot-policy.vqsk\n"
      << "snapshot_records=quickjs-snapshot-records.vqsn\n"
      << "replay_validation=quickjs-replay-validation.vqrv\n"
      << "determinism_ledger=quickjs-determinism-ledger.vqdl\n"
      << "audit_seal=quickjs-audit-seal.vqas\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "module_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags) << "\n";
  }
  return out.str();
}

std::string make_quickjs_wasm_runtime_metadata(const Profile& profile, const std::string& runtime_mode, const std::string& wasm_asset_name) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_WASM_RUNTIME_V10\n"
      << "version=10\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "asset_name=" << wasm_asset_name << "\n"
      << "abi=" << venom::quickjs::kRuntimeAbiVersion << "\n"
      << "package_version=" << venom::quickjs::kRuntimePackageVersion << "\n"
      << "execution_mode=quickjs-wasm-abi12-upstream-global-host-api-shims\n"
      << "runtime_implementation=" << wasm_truth_value("runtime_implementation", "quickjs-wasm-contract-scaffold") << "\n"
      << "runtime_claim=" << wasm_truth_value("runtime_claim", "contract-boundary") << "\n"
      << "contract_only=" << (wasm_truth_bool("contract_only", true) ? "true" : "false") << "\n"
      << "scaffold_runtime=" << (wasm_truth_bool("scaffold_runtime", true) ? "true" : "false") << "\n"
      << "real_engine_candidate=" << (wasm_truth_bool("real_engine_candidate", false) ? "true" : "false") << "\n"
      << "full_upstream_quickjs=" << (wasm_truth_bool("full_upstream_quickjs", false) ? "true" : "false") << "\n"
      << "fallback_required=" << (wasm_truth_bool("fallback_required", false) ? "true" : "false") << "\n"
      << "finish_blocker=" << wasm_truth_value("finish_blocker", "replace_quickjs_runtime_scaffold_with_upstream_quickjs_wasm") << "\n"
      << "artifact_kind=" << wasm_truth_value("artifact_kind", "contract-scaffold") << "\n"
      << "wasm_sha256=" << quickjs_runtime_wasm_sha256() << "\n"
      << "required_exports_satisfied=" << (wasm_truth_bool("required_exports_satisfied", false) ? "true" : "false") << "\n"
      << "missing_export_count=" << wasm_truth_value("missing_export_count", "0") << "\n"
      << "exports=venom_qjs_backend_kind,venom_qjs_backend_ready,venom_qjs_bytecode_expected_source_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_record_executed,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_record_size,venom_qjs_bytecode_record_source_hash32,venom_qjs_bytecode_validate,venom_qjs_console_clear,venom_qjs_console_event_count,venom_qjs_console_event_ptr,venom_qjs_console_event_size,venom_qjs_context_alloc,venom_qjs_context_free,venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_set_limits,venom_qjs_context_stack_limit,venom_qjs_engine_abi,venom_qjs_engine_state,venom_qjs_engine_trap_code,venom_qjs_engine_version,venom_qjs_exception_clear,venom_qjs_exception_code,venom_qjs_exception_message_ptr,venom_qjs_exception_message_size,venom_qjs_exception_ptr,venom_qjs_exception_size,venom_qjs_execute_bytecode,venom_qjs_execute_source,venom_qjs_execution_record_ptr,venom_qjs_execution_record_size,venom_qjs_fallback_required,venom_qjs_host_call_count,venom_qjs_host_call_dispatch,venom_qjs_host_call_known,venom_qjs_interop_fallback_required,venom_qjs_interpreter_ready,venom_qjs_module_execute,venom_qjs_module_execution_count,venom_qjs_module_record_ptr,venom_qjs_module_record_size,venom_qjs_parity_probe,venom_qjs_real_engine_candidate,venom_qjs_release_status,venom_qjs_result_ptr,venom_qjs_result_size,venom_qjs_script_buffer_alloc,venom_qjs_script_buffer_alloc_count,venom_qjs_script_buffer_capacity,venom_qjs_script_buffer_expected_hash,venom_qjs_script_buffer_free,venom_qjs_script_buffer_free_count,venom_qjs_script_buffer_ptr,venom_qjs_script_buffer_set_expected_hash,venom_qjs_script_buffer_size,venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_upstream_quickjs_ready,venom_qjs_wasm_native_stack_capacity\n"
      << "fallback=deny-host-js-source-eval\n"
      << "abi_contract=quickjs-wasm-abi12-runtime\n"
      << "status_exports=venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_release_status\n"
      << "limit_exports=venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_stack_limit\n"
      << "bytecode_validation_exports=venom_qjs_bytecode_validate,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_expected_source_hash32\n"
      << "interpreter_exports=venom_qjs_interpreter_ready,venom_qjs_interpreter_contract_ptr,venom_qjs_interpreter_dispatch_count,venom_qjs_interpreter_opcode_count,venom_qjs_global_slot_count,venom_qjs_last_global_write_hash,venom_qjs_interpreter_semantic_pass_count,venom_qjs_interpreter_property_write_count,venom_qjs_interpreter_builtin_probe_count,venom_qjs_interpreter_console_call_count,venom_qjs_interpreter_semantic_record_ptr,venom_qjs_interpreter_semantic_record_size,venom_qjs_interpreter_semantic_record_hash,venom_qjs_global_slot_record_ptr,venom_qjs_global_slot_record_size,venom_qjs_global_slot_record_hash,venom_qjs_upstream_quickjs_ready,venom_qjs_upstream_parity_record_ptr,venom_qjs_upstream_parity_record_size,venom_qjs_upstream_parity_record_hash,venom_qjs_upstream_feature_count,venom_qjs_upstream_builtin_count,venom_qjs_upstream_object_model_score,venom_qjs_upstream_exception_model_score,venom_qjs_upstream_module_model_score,venom_qjs_upstream_object_record_ptr,venom_qjs_upstream_object_record_size,venom_qjs_upstream_object_record_hash,venom_qjs_upstream_exception_record_ptr,venom_qjs_upstream_exception_record_size,venom_qjs_upstream_exception_record_hash,venom_qjs_upstream_module_record_ptr,venom_qjs_upstream_module_record_size,venom_qjs_upstream_module_record_hash,venom_qjs_upstream_runtime_bridge_score,venom_qjs_upstream_bytecode_semantics_record_ptr,venom_qjs_upstream_bytecode_semantics_record_size,venom_qjs_upstream_bytecode_semantics_record_hash,venom_qjs_upstream_bytecode_semantics_score,venom_qjs_upstream_lexical_scope_record_ptr,venom_qjs_upstream_lexical_scope_record_size,venom_qjs_upstream_lexical_scope_record_hash,venom_qjs_upstream_closure_record_ptr,venom_qjs_upstream_closure_record_size,venom_qjs_upstream_closure_record_hash,venom_qjs_upstream_iterator_record_ptr,venom_qjs_upstream_iterator_record_size,venom_qjs_upstream_iterator_record_hash,venom_qjs_upstream_intrinsic_record_ptr,venom_qjs_upstream_intrinsic_record_size,venom_qjs_upstream_intrinsic_record_hash,venom_qjs_upstream_intrinsic_semantics_score,venom_qjs_upstream_property_descriptor_record_ptr,venom_qjs_upstream_property_descriptor_record_size,venom_qjs_upstream_property_descriptor_record_hash,venom_qjs_upstream_prototype_chain_record_ptr,venom_qjs_upstream_prototype_chain_record_size,venom_qjs_upstream_prototype_chain_record_hash,venom_qjs_upstream_call_frame_record_ptr,venom_qjs_upstream_call_frame_record_size,venom_qjs_upstream_call_frame_record_hash\n"
      << "engine_backend=quickjs-engine-backend.vqeb\n";
  return out.str();
}

std::string make_quickjs_wasm_execution_metadata(const Profile& profile,
                                                const BuildOptions& options,
                                                const ReleaseBuildPolicy& policy,
                                                const JsBridge& bridge,
                                                const std::string& wasm_asset_name) {
  const bool wasm_real = policy.backend == "wasm-real";
  const bool host_fallback_allowed = policy.fallback_allowed && !wasm_real;
  const auto protected_chunk_count = static_cast<std::size_t>(std::count_if(
      bridge.chunks.begin(), bridge.chunks.end(),
      [](const JsChunk& chunk) { return (chunk.flags & JsChunkBrowser) == 0u; }));
  const auto protected_count = protected_chunk_count + (bridge.bridge_registry_bytecode.empty() ? 0u : 1u);
  std::ostringstream out;
  out << "VENOM_QJS_WASM_EXECUTION_V2\n"
      << "version=2\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << options.runtime << "\n"
      << "security_target=" << options.security_target << "\n"
      << "enabled=" << (wasm_real ? "true" : "false") << "\n"
      << "selected_backend=" << (wasm_real ? "quickjs-wasm-real" : policy.backend) << "\n"
      << "backend_claim=" << (wasm_real ? wasm_truth_value("runtime_claim", "quickjs-wasm-contract") : policy.backend) << "\n"
      << "runtime_implementation=" << wasm_truth_value("runtime_implementation", "quickjs-wasm-contract-scaffold") << "\n"
      << "contract_only=" << (wasm_truth_bool("contract_only", true) ? "true" : "false") << "\n"
      << "scaffold_runtime=" << (wasm_truth_bool("scaffold_runtime", true) ? "true" : "false") << "\n"
      << "real_engine_candidate=" << (wasm_truth_bool("real_engine_candidate", false) ? "true" : "false") << "\n"
      << "full_upstream_quickjs=" << (wasm_truth_bool("full_upstream_quickjs", false) ? "true" : "false") << "\n"
      << "artifact_kind=" << wasm_truth_value("artifact_kind", "contract-scaffold") << "\n"
      << "required_exports_satisfied=" << (wasm_truth_bool("required_exports_satisfied", false) ? "true" : "false") << "\n"
      << "missing_export_count=" << wasm_truth_value("missing_export_count", "0") << "\n"
      << "backend_contract=quickjs-wasm-abi12-upstream-global-host-api-shims-v7\n"
      << "wasm_runtime_asset=" << wasm_asset_name << "\n"
      << "bytecode_format=VQJSBC03\n"
      << "bytecode_source_preserving=false\n"
      << "decode_boundary=wasm-owned-bytecode-abi12-upstream-semantics\n"
      << "source_eval_fallback=false\n"
      << "interpreter_dispatch=enabled\n"
      << "source_eval_in_runtime=false\n"
      << "host_js_fallback_allowed=" << (host_fallback_allowed ? "true" : "false") << "\n"
      << "silent_fallback_allowed=false\n"
      << "require_engine_module=true\n"
      << "require_wasm_asset_bound=true\n"
      << "require_result_commit=true\n"
      << "release_fail_closed=true\n"
      << "runtime_abi=" << venom::quickjs::kRuntimeAbiVersion << "\n"
      << "status_code_export=venom_qjs_status_code\n"
      << "status_record_export=venom_qjs_status_record_ptr|venom_qjs_status_record_size\n"
      << "bytecode_validate_export=venom_qjs_bytecode_validate\n"
      << "bytecode_hash_export=venom_qjs_bytecode_record_hash32\n"
      << "bytecode_payload_export=venom_qjs_bytecode_payload_size\n"
      << "backend_contract_export=venom_qjs_backend_contract_ptr|venom_qjs_backend_contract_size\n"
      << "runtime_limits_export=venom_qjs_runtime_limits_ptr|venom_qjs_runtime_limits_size\n"
      << "context_report_export=venom_qjs_context_report_ptr|venom_qjs_context_report_size\n"
      << "interpreter_ready_export=venom_qjs_interpreter_ready\n"
      << "interpreter_contract_export=venom_qjs_interpreter_contract_ptr|venom_qjs_interpreter_contract_size\n"
      << "interpreter_dispatch_export=venom_qjs_interpreter_dispatch_count\n"
      << "semantic_runtime=enabled\n"
      << "upstream_quickjs_bridge=enabled\n"
      << "upstream_interpreter_core=quickjs-upstream-global-host-api-shims-v7\n"
      << "upstream_parity_record_export=venom_qjs_upstream_parity_record_ptr|venom_qjs_upstream_parity_record_size\n"
      << "upstream_runtime_record_exports=venom_qjs_upstream_object_record_ptr|venom_qjs_upstream_exception_record_ptr|venom_qjs_upstream_module_record_ptr|venom_qjs_upstream_runtime_bridge_score|venom_qjs_upstream_bytecode_semantics_record_ptr|venom_qjs_upstream_lexical_scope_record_ptr|venom_qjs_upstream_closure_record_ptr|venom_qjs_upstream_iterator_record_ptr|venom_qjs_upstream_intrinsic_record_ptr|venom_qjs_upstream_property_descriptor_record_ptr|venom_qjs_upstream_prototype_chain_record_ptr|venom_qjs_upstream_call_frame_record_ptr\n"
      << "upstream_bytecode_semantics=enabled\n"
      << "upstream_bytecode_semantics_exports=venom_qjs_upstream_bytecode_semantics_record_ptr|venom_qjs_upstream_bytecode_semantics_score\n"
      << "upstream_intrinsic_semantics=enabled\n"
      << "upstream_intrinsic_semantics_exports=venom_qjs_upstream_intrinsic_record_ptr|venom_qjs_upstream_property_descriptor_record_ptr|venom_qjs_upstream_prototype_chain_record_ptr|venom_qjs_upstream_call_frame_record_ptr|venom_qjs_upstream_intrinsic_semantics_score\n"
      << "semantic_record_export=venom_qjs_interpreter_semantic_record_ptr|venom_qjs_interpreter_semantic_record_size\n"
      << "global_slot_record_export=venom_qjs_global_slot_record_ptr|venom_qjs_global_slot_record_size\n"
      << "chunk_count=" << protected_count << "\n";
  for (const auto& chunk : bridge.chunks) {
    if ((chunk.flags & JsChunkBrowser) != 0u) continue;
    out << "wasm_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tbytes=" << protected_source_size(profile, chunk) << "\tflags=" << js_flags_summary(chunk.flags)
        << "\tbackend=" << (wasm_real ? "quickjs-wasm-real" : policy.backend) << "\n";
  }
  if (!bridge.bridge_registry_bytecode.empty()) {
    out << "wasm_chunk\tregistry\tprotected-bridge-registry\tnative-quickjs-object"
        << "\tbytes=" << bridge.bridge_registry_bytecode.size() << "\tflags=protected,isolated"
        << "\tbackend=" << (wasm_real ? "quickjs-wasm-real" : policy.backend) << "\n";
  }
  return out.str();
}

std::string make_quickjs_source_transfer_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_SOURCE_TRANSFER_V7\n"
      << "version=7\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "source_ptr=venom_qjs_script_buffer_ptr\n"
      << "source_capacity=venom_qjs_script_buffer_capacity\n"
      << "execute_source=venom_qjs_execute_source\n"
      << "execute_bytecode=venom_qjs_execute_bytecode\n"
      << "validate_bytecode=venom_qjs_bytecode_validate\n"
      << "bytecode_status=venom_qjs_status_code\n"
      << "bytecode_record_hash=venom_qjs_bytecode_record_hash32\n"
      << "bytecode_payload_size=venom_qjs_bytecode_payload_size\n"
      << "transfer_mode=opaque-vqjsbc03-native-object\n"
      << "oversized_record_threshold=786432\n"
      << "oversized_record_path=wasm-native-object-read\n"
      << "oversized_execution_export=venom_qjs_execute_bytecode\n"
      << "host_source_eval=false\n"
      << "protected_source_execution=false\n"
      << "interpreter_dispatch=venom_qjs_interpreter_ready|venom_qjs_interpreter_dispatch_count\n"
      << "result_ptr=venom_qjs_result_ptr\n"
      << "result_size=venom_qjs_result_size\n"
      << "encoding=native-quickjs-object-v3\n"
      << "host_call=quickjs.sourceTransfer\n";
  return out.str();
}
std::string make_quickjs_console_bridge_metadata(const Profile& profile, const std::string& runtime_mode) {
  std::ostringstream out;
  out << "VENOM_QUICKJS_CONSOLE_BRIDGE_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "mode=host-console-forward\n"
      << "host_call=quickjs.consoleBridge\n"
      << "levels=debug,log,info,warn,error\n"
      << "callback_abi=venom_qjs_console_callback_abi\n"
      << "event_ptr=venom_qjs_console_event_ptr\n"
      << "event_size=venom_qjs_console_event_size\n"
      << "event_count=venom_qjs_console_event_count\n";
  return out.str();
}

std::string make_script_engine_policy_metadata(const Profile& profile, const std::string& runtime_mode, const JsBridge& bridge) {
  const bool allow_function_constructor = profile.kind == ProfileKind::Dev;
  std::ostringstream out;
  out << "VENOM_SCRIPT_ENGINE_POLICY_V4\n"
      << "version=4\n"
      << "profile=" << profile.name << "\n"
      << "runtime_mode=" << runtime_mode << "\n"
      << "enabled=true\n"
      << "fallback=deny-host-js-source-eval\n"
      << "abi_contract=quickjs-wasm-abi12-runtime\n"
      << "status_exports=venom_qjs_status_code,venom_qjs_status_record_ptr,venom_qjs_status_record_size,venom_qjs_release_status\n"
      << "limit_exports=venom_qjs_context_heap_limit,venom_qjs_context_heap_used,venom_qjs_context_stack_limit\n"
      << "bytecode_validation_exports=venom_qjs_bytecode_validate,venom_qjs_bytecode_record_hash32,venom_qjs_bytecode_payload_size,venom_qjs_bytecode_expected_source_hash32\n"
      << "allow_eval=false\n"
      << "allow_function_constructor=" << (allow_function_constructor ? "true" : "false") << "\n"
      << "capabilities=console,document,window,fetch,timers,events\n"
      << "bind_console=true\n"
      << "bind_document=true\n"
      << "bind_fetch=true\n"
      << "bind_timers=true\n"
      << "bind_events=true\n"
      << "route_scope=true\n"
      << "context_reuse=true\n"
      << "context_destroy=true\n"
      << "host_capability_injection=true\n"
      << "chunk_count=" << bridge.chunks.size() << "\n";
  for (const auto& chunk : bridge.chunks) {
    out << "policy_chunk\t" << chunk.order << "\t" << protected_route_label(profile, chunk) << "\t" << protected_source_label(profile, chunk)
        << "\tcapabilities=console,document,window,fetch,timers,events\n";
  }
  return out.str();
}

} // namespace venom::compiler::build_runtime_detail
