#include "core/site.hpp"
#include "chrome_extension.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using venom::compiler::chrome_extension::ExecutionContext;
using venom::compiler::chrome_extension::RuntimeHost;

struct FixtureExpectation {
  const char* name;
  RuntimeHost host;
  const char* required_path;
  ExecutionContext required_context;
  bool expect_warning;
};

static bool has(const venom::compiler::chrome_extension::ManifestAnalysis& analysis,
                const std::string& path, ExecutionContext context) {
  for (const auto& resource : analysis.resources) {
    if (resource.path == path && resource.context == context) return true;
  }
  return false;
}

int main() {
#ifdef VENOM_SOURCE_ROOT
  const fs::path root = fs::path(VENOM_SOURCE_ROOT) / "tests/fixtures/chrome-extensions";
#else
  const fs::path root = fs::current_path() / "tests/fixtures/chrome-extensions";
#endif
  const std::vector<FixtureExpectation> fixtures = {
    {"popup-page", RuntimeHost::ExtensionPage, "popup.js", ExecutionContext::ExtensionPage, false},
    {"service-worker-module", RuntimeHost::ServiceWorker, "worker-dep.js", ExecutionContext::ServiceWorker, false},
    {"content-isolated", RuntimeHost::ServiceWorker, "content-dep.js", ExecutionContext::ContentScriptIsolated, false},
    {"content-main", RuntimeHost::ServiceWorker, "main.js", ExecutionContext::ContentScriptMain, false},
    {"offscreen-runtime", RuntimeHost::OffscreenDocument, "offscreen.js", ExecutionContext::ExtensionPage, false},
    {"options-sidepanel", RuntimeHost::ExtensionPage, "side.js", ExecutionContext::ExtensionPage, false},
    {"devtools-sandbox", RuntimeHost::ExtensionPage, "sandbox.js", ExecutionContext::SandboxPage, false},
    {"dnr-web-accessible", RuntimeHost::None, "rules.json", ExecutionContext::StaticResource, false},
    {"localized-icons", RuntimeHost::None, "_locales/en/messages.json", ExecutionContext::StaticResource, false},
    {"dynamic-registration", RuntimeHost::ServiceWorker, "registered.js", ExecutionContext::ContentScriptIsolated, false},
    {"multi-route", RuntimeHost::ExtensionPage, "newtab.js", ExecutionContext::ExtensionPage, false},
    {"sandbox-only", RuntimeHost::None, "sandbox.js", ExecutionContext::SandboxPage, false},
  };

  for (const auto& fixture : fixtures) {
    const auto fixture_root = root / fixture.name;
    if (!fs::is_directory(fixture_root)) throw std::runtime_error("fixture missing: " + fixture_root.string());
    const auto graph = venom::compiler::collect_site(fixture_root);
    const auto analysis = venom::compiler::chrome_extension::analyze_project(graph);
    if (analysis.manifest_version != 3) return 10;
    if (analysis.runtime_host != fixture.host) {
      std::cerr << fixture.name << ": wrong runtime host\n";
      return 11;
    }
    if (!has(analysis, fixture.required_path, fixture.required_context)) {
      std::cerr << fixture.name << ": missing resource/context " << fixture.required_path << "\n";
      return 12;
    }
    if (fixture.expect_warning != !analysis.warnings.empty()) {
      std::cerr << fixture.name << ": unexpected warning state\n";
      return 13;
    }
    // Validation is required only when the fixture has a compatible protected
    // runtime host. Adapter-only fixtures intentionally demonstrate contexts
    // that cannot host TurboJS/WASM directly.
    if (analysis.runtime_host != RuntimeHost::None) {
      venom::compiler::chrome_extension::validate_project(graph);
    }
  }

  std::cout << "Chrome extension compatibility matrix: PASS (" << fixtures.size() << " fixtures)\n";
  return 0;
}
