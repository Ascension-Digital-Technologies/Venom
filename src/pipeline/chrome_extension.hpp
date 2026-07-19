#pragma once

#include "venom/core/site.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler::chrome_extension {

enum class ExecutionContext {
  ExtensionPage,
  ServiceWorker,
  ContentScriptIsolated,
  ContentScriptMain,
  DevToolsPage,
  SandboxPage,
  StaticResource
};

enum class RuntimeHost {
  None,
  ExtensionPage,
  OffscreenDocument,
  ServiceWorker
};

struct ContextPolicy {
  ExecutionContext context = ExecutionContext::StaticResource;
  bool has_dom = false;
  bool has_extension_api = false;
  bool can_host_quickjs_wasm = false;
  bool requires_visible_adapter = false;
  bool stable_path_required = false;
};

struct ResourceReference {
  std::string path;
  ExecutionContext context = ExecutionContext::StaticResource;
  std::string source;
  bool required = true;
};

struct RpcPolicy {
  std::size_t max_payload_bytes = 1024 * 1024;
  std::size_t max_inflight = 32;
  std::size_t default_timeout_ms = 20000;
  std::size_t max_timeout_ms = 60000;
  std::string protocol = "venom-extension-rpc-v1";
};

struct ManifestAnalysis {
  int manifest_version = 0;
  std::string minimum_chrome_version;
  std::vector<ResourceReference> resources;
  std::vector<std::string> permissions;
  std::vector<std::string> host_permissions;
  std::vector<std::string> warnings;
  RuntimeHost runtime_host = RuntimeHost::None;
  std::string background_service_worker;
  bool background_is_module = false;
  bool has_offscreen_permission = false;
  bool has_extension_page = false;
  bool has_main_world_script = false;
  bool has_isolated_content_script = false;
  RpcPolicy rpc_policy;
};

bool is_extension_target(const std::string& target);
ManifestAnalysis analyze_project(const SiteGraph& graph);
void validate_project(const SiteGraph& graph);
void emit_extension_files(const SiteGraph& graph, const std::filesystem::path& output);
const char* execution_context_name(ExecutionContext context);
const char* runtime_host_name(RuntimeHost host);
ContextPolicy context_policy(ExecutionContext context);
std::string compatibility_summary(const ManifestAnalysis& analysis);

} // namespace venom::compiler::chrome_extension
