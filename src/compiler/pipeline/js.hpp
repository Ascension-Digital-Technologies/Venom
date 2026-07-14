#pragma once

#include "compiler/core/site.hpp"
#include "compiler/commands/remote.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "vm/polymorph.hpp"

namespace venom::compiler {

enum JsChunkFlags : std::uint32_t {
  JsChunkInline = 1u << 0u,
  JsChunkModule = 1u << 1u,
  JsChunkDefer = 1u << 2u,
  JsChunkAsync = 1u << 3u,
  JsChunkExternalFile = 1u << 4u,
  JsChunkRemoteUrl = 1u << 5u,
  JsChunkBytecodeEncoded = 1u << 6u,
  JsChunkVendoredRemote = 1u << 7u,
  JsChunkDependency = 1u << 8u,
  JsChunkDynamicImport = 1u << 9u,
  JsChunkModulePreload = 1u << 10u,
  JsChunkBrowser = 1u << 11u,
};

struct JsChunk {
  std::string route;
  std::string source;
  std::string code;
  std::uint32_t order = 0;
  std::uint32_t flags = 0;
  std::uint32_t kind = 0;
  std::string execution_reason;
  std::uint32_t execution_confidence = 0;
};

struct ModuleGraphEdge {
  std::string route;
  std::string referrer;
  std::string specifier;
  std::string resolved;
  bool dynamic = false;
  bool preload = false;
};

struct JsBridge {
  std::vector<JsChunk> chunks;
  std::vector<ModuleGraphEdge> module_edges;
  std::vector<unsigned char> bundle;
  std::string diagnostics;
  std::string execution_plan_text;
  std::string execution_plan_json;
  std::string function_plan_text;
  std::string function_plan_json;
  std::string extraction_plan_text;
  std::string extraction_plan_json;
  std::string realm_bridge_contract_json;
  std::vector<std::string> bridge_candidate_ids;
  std::vector<std::pair<std::string, std::string>> protected_exports;
  std::vector<unsigned char> bridge_registry_bytecode;
  std::string bridge_rewrite_report_json;
  std::string preview;
  std::string protected_api_typescript;
  std::vector<RemoteVendorRecord> remote_vendors;
};

std::string make_loader_js(const std::string& runtime_asset_name,
                           const std::string& package_asset_name,
                           std::uint64_t expected_package_hash,
                           const std::string& expected_package_sha256,
                           bool hardened,
                           const std::string& quickjs_engine_asset_name = std::string{},
                           const std::string& quickjs_runtime_wasm_asset_name = std::string{},
                           const std::string& runtime_wasm_asset_name = std::string{},
                           const std::string& style_asset_name = std::string{},
                           const std::string& package_binding_token = std::string{},
                           const std::string& worker_asset_name = std::string{},
                           const std::string& quickjs_runtime_wasm_sha256 = std::string{},
                           const std::vector<std::pair<std::string, std::string>>& protected_exports = {},
                           std::uint32_t bridge_invoke_opcode = 0x31u,
                           std::uint32_t bridge_cancel_opcode = 0x32u,
                           std::uint32_t bridge_result_opcode = 0x33u,
                           std::uint32_t bridge_error_opcode = 0x34u);
std::string make_worker_runtime_js(const std::vector<std::string>& bridge_candidate_ids = {},
                                   const std::vector<unsigned char>& bridge_registry_bytecode = {},
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
std::string make_quickjs_engine_module_js(bool protected_release = false);
std::vector<unsigned char> make_quickjs_runtime_wasm_module();
std::string quickjs_runtime_wasm_provenance_text();
std::string quickjs_runtime_wasm_sha256();
std::string bundle_js_preview(const SiteGraph& graph);
JsBridge compile_js_bridge(const SiteGraph& graph, const RemoteVendorOptions& remote_options = {}, bool development = false);
std::string js_flags_summary(std::uint32_t flags);

} // namespace venom::compiler
