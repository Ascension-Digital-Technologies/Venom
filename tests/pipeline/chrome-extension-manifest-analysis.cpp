#include "venom/core/site.hpp"
#include "venom/internal/pipeline/chrome_extension.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

static void write(const fs::path& path, const std::string& value) {
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) throw std::runtime_error("write failed");
  out << value;
}

static std::string read(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

static bool has(const venom::compiler::chrome_extension::ManifestAnalysis& analysis,
                const std::string& path,
                venom::compiler::chrome_extension::ExecutionContext context) {
  for (const auto& resource : analysis.resources) if (resource.path == path && resource.context == context) return true;
  return false;
}

int main() {
  const auto root = fs::temp_directory_path() / "venom-chrome-extension-manifest-analysis";
  fs::remove_all(root);
  write(root / "manifest.json", R"({
    "manifest_version": 3,
    "name": "fixture",
    "version": "1.0.0",
    "minimum_chrome_version": "109",
    "permissions": ["scripting", "offscreen"],
    "host_permissions": ["https://example.com/*"],
    "action": {"default_popup": "popup.html", "default_icon": {"16": "icons/16.png"}},
    "background": {"service_worker": "background.js", "type": "module"},
    "options_ui": {"page": "options/index.html"},
    "side_panel": {"default_path": "side.html"},
    "devtools_page": "devtools.html",
    "content_scripts": [
      {"matches": ["https://example.com/*"], "js": ["content.js"], "css": ["content.css"]},
      {"matches": ["https://example.com/*"], "js": ["main.js"], "world": "MAIN"}
    ],
    "sandbox": {"pages": ["sandbox.html"]},
    "declarative_net_request": {"rule_resources": [{"id": "rules", "enabled": true, "path": "rules.json"}]},
    "web_accessible_resources": [{"resources": ["bridge.js"], "matches": ["https://example.com/*"]}]
  })");
  write(root / "popup.html", "<script type=\"module\" src=\"popup.js\"></script>");
  write(root / "popup.js", "chrome.runtime.getURL('bridge.js');");
  write(root / "background.js", "export {};");
  write(root / "options/index.html", "<link rel=\"stylesheet\" href=\"options.css\">");
  write(root / "options/options.css", "body{}");
  write(root / "side.html", "side");
  write(root / "devtools.html", "devtools");
  write(root / "content.js", "import \"./content-dep.js\"; globalThis.contentLoaded = true;");
  write(root / "content-dep.js", "globalThis.contentDependency = true;");
  write(root / "content.css", "body{}");
  write(root / "main.js", "import \"./main-dep.js\"; globalThis.mainLoaded = true;");
  write(root / "main-dep.js", "globalThis.mainDependency = true;");
  write(root / "sandbox.html", "sandbox");
  write(root / "rules.json", "[]");
  write(root / "bridge.js", "globalThis.bridgeReady = true;");
  write(root / "icons/16.png", "png");
  write(root / "index.html", "engine");
  write(root / "engine-host.js", "globalThis.engineHostReady = true;");

  const auto graph = venom::compiler::collect_site(root);
  const auto analysis = venom::compiler::chrome_extension::analyze_project(graph);
  using Context = venom::compiler::chrome_extension::ExecutionContext;
  if (analysis.manifest_version != 3 || analysis.minimum_chrome_version != "109") return 1;
  if (!has(analysis, "popup.html", Context::ExtensionPage)) return 2;
  if (!has(analysis, "background.js", Context::ServiceWorker)) return 3;
  if (!has(analysis, "content.js", Context::ContentScriptIsolated)) return 4;
  if (!has(analysis, "main.js", Context::ContentScriptMain)) return 5;
  if (!has(analysis, "content-dep.js", Context::ContentScriptIsolated)) return 6;
  if (!has(analysis, "main-dep.js", Context::ContentScriptMain)) return 7;
  if (!has(analysis, "devtools.html", Context::DevToolsPage)) return 8;
  if (!has(analysis, "sandbox.html", Context::SandboxPage)) return 9;
  if (!has(analysis, "rules.json", Context::StaticResource)) return 10;
  if (!has(analysis, "options/options.css", Context::ExtensionPage)) return 11;
  if (analysis.runtime_host != venom::compiler::chrome_extension::RuntimeHost::OffscreenDocument) return 12;
  const auto isolated = venom::compiler::chrome_extension::context_policy(Context::ContentScriptIsolated);
  const auto main_world = venom::compiler::chrome_extension::context_policy(Context::ContentScriptMain);
  const auto worker = venom::compiler::chrome_extension::context_policy(Context::ServiceWorker);
  if (!isolated.has_dom || !isolated.has_extension_api || !isolated.requires_visible_adapter) return 13;
  if (!main_world.has_dom || main_world.has_extension_api || !main_world.requires_visible_adapter) return 14;
  if (worker.has_dom || !worker.has_extension_api || !worker.can_host_quickjs_wasm) return 15;
  if (venom::compiler::chrome_extension::compatibility_summary(analysis).find("offscreen-document") == std::string::npos) return 16;
  venom::compiler::chrome_extension::validate_project(graph);

  const auto output = root / "dist";
  fs::create_directories(output);
  write(output / "index.html", "<html><body><script src=\"assets/javascript/loader.js\"></script></body></html>");
  venom::compiler::chrome_extension::emit_extension_files(graph, output);
  const auto emitted_manifest = read(output / "manifest.json");
  if (emitted_manifest.find("\"service_worker\": \"assets/extension/venom-background.js\"") == std::string::npos) return 17;
  if (read(output / "assets/extension/venom-background.js").find("./background.js") == std::string::npos) return 18;
  if (read(output / "assets/extension/venom-extension-broker.js").find("HOST_CALL") == std::string::npos) return 19;
  if (read(output / "assets/extension/venom-extension-host.js").find("Protected runtime is busy") == std::string::npos) return 20;
  if (read(output / "assets/extension/venom-extension-rpc.js").find("VenomExtensionRPC") == std::string::npos) return 21;
  const auto engine_page = read(output / "assets/extension/engine.html");
  if (engine_page.find("venom-extension-host.js") == std::string::npos || engine_page.find("engine-host.js") == std::string::npos) return 22;

  fs::remove(root / "content.js");
  bool rejected = false;
  try { venom::compiler::chrome_extension::validate_project(venom::compiler::collect_site(root)); }
  catch (const std::exception& error) { rejected = std::string(error.what()).find("content.js") != std::string::npos; }
  fs::remove_all(root);
  if (!rejected) return 23;
  std::cout << "Chrome extension manifest analysis smoke: PASS\n";
  return 0;
}
