#pragma once

#include "core/site.hpp"
#include "graph/module_graph.hpp"
#include "graph/module_types.hpp"
#include "remote/remote.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "vm/polymorph.hpp"

namespace venom::compiler {

struct ProtectedRegistryChunk {
  std::string id;
  std::vector<std::string> candidates;
  std::vector<unsigned char> bytecode;
};

struct WorkerLazyRegistryChunk {
  std::string id;
  std::string asset;
  std::string sha256;
  std::vector<std::string> candidates;
  bool preload = false;
};

struct JsBridge {
  std::vector<JsChunk> chunks;
  std::vector<ModuleGraphEdge> module_edges;
  graph::CanonicalModuleGraph module_graph;
  std::vector<unsigned char> bundle;
  std::string diagnostics;
  std::string execution_plan_text;
  std::string execution_plan_json;
  std::string function_plan_text;
  std::string function_plan_json;
  std::string extraction_plan_text;
  std::string extraction_plan_json;
  std::string runtime_bridge_contract_json;
  std::vector<std::string> bridge_candidate_ids;
  std::vector<std::pair<std::string, std::string>> protected_exports;
  std::vector<std::string> protected_export_sources;
  std::string lazy_protected_manifest_json;
  std::vector<unsigned char> bridge_registry_bytecode;
  std::vector<ProtectedRegistryChunk> bridge_registry_chunks;
  std::string bridge_rewrite_report_json;
  std::string protection_intent_ledger_json;
  std::size_t protected_intents_requested = 0;
  std::size_t protected_intents_resolved = 0;
  std::size_t protected_whole_file_intents = 0;
  std::size_t expected_turbojs_records = 0;
  std::string preview;
  std::string protected_api_typescript;
  std::string protected_api_client_javascript;
  std::string protected_api_client_typescript;
  std::string protected_contracts_json;
  std::vector<RemoteVendorRecord> remote_vendors;
};

std::vector<unsigned char> encode_js_bundle(const std::vector<JsChunk>& chunks,
                                                   const graph::CanonicalModuleGraph& module_graph,
                                                   const std::string& diversification_salt,
                                                   bool development,
                                                   bool harden_public_javascript = true);

std::string make_loader_js(const std::string& runtime_asset_name,
                           const std::string& package_asset_name,
                           std::uint64_t expected_package_hash,
                           const std::string& expected_package_sha256,
                           bool hardened,
                           const std::string& turbojs_engine_asset_name = std::string{},
                           const std::string& turbojs_runtime_wasm_asset_name = std::string{},
                           const std::string& runtime_wasm_asset_name = std::string{},
                           const std::string& style_asset_name = std::string{},
                           const std::string& package_binding_token = std::string{},
                           const std::string& worker_asset_name = std::string{},
                           const std::string& turbojs_runtime_wasm_sha256 = std::string{},
                           const std::vector<std::pair<std::string, std::string>>& protected_exports = {},
                           std::uint32_t bridge_invoke_opcode = 0x31u,
                           std::uint32_t bridge_cancel_opcode = 0x32u,
                           std::uint32_t bridge_result_opcode = 0x33u,
                           std::uint32_t bridge_error_opcode = 0x34u);
std::string make_worker_runtime_js(const std::vector<std::string>& bridge_candidate_ids = {},
                                   const std::vector<unsigned char>& bridge_registry_bytecode = {},
                                   const std::vector<WorkerLazyRegistryChunk>& lazy_registry_chunks = {},
                                   std::uint32_t bridge_invoke_opcode = 0x31u,
                                   std::uint32_t bridge_cancel_opcode = 0x32u,
                                   std::uint32_t bridge_result_opcode = 0x33u,
                                   std::uint32_t bridge_error_opcode = 0x34u);
std::string make_browser_asset_runtime_js();
std::string make_runtime_js(const SiteGraph& graph, bool protected_release = false);
std::string make_runtime_wasm_bridge_js(const SiteGraph& graph,
                                        const std::string& wasm_asset_name,
                                        bool protected_release = false,
                                        const venom::vm::PolymorphicPlan* diversification = nullptr);
std::vector<unsigned char> make_runtime_wasm_module();
std::string make_turbojs_engine_module_js(bool protected_release = false);
std::vector<unsigned char> make_turbojs_runtime_wasm_module();
std::string turbojs_runtime_wasm_provenance_text();
std::string turbojs_runtime_wasm_sha256();
std::string bundle_js_preview(const SiteGraph& graph);
std::vector<unsigned char> encode_protected_bridge_registry(const JsBridge& bridge, const std::string& diversification_salt);
JsBridge compile_js_bridge(const SiteGraph& graph, const RemoteVendorOptions& remote_options = {}, bool development = false, bool harden_public_javascript = true);
std::string js_flags_summary(std::uint32_t flags);

} // namespace venom::compiler
