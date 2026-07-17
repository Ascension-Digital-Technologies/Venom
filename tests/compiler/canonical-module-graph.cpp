#include "compiler/graph/module_graph.hpp"
#include "compiler/pipeline/js.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
  using namespace venom::compiler;
  std::vector<JsChunk> chunks;

  JsChunk entry;
  entry.route = "/";
  entry.source = "browser/app.tsx";
  entry.code = "import { quote } from '../protected/pricing';\nimport runtime from './jsx-runtime';\nconsole.log(runtime, quote);\n";
  entry.order = 0;
  entry.flags = JsChunkBrowser | JsChunkModule;
  chunks.push_back(entry);

  JsChunk pricing;
  pricing.route = "/";
  pricing.source = "protected/pricing.ts";
  pricing.code = "export const quote = (value) => value * 2;\n";
  pricing.order = 1;
  pricing.flags = JsChunkModule | JsChunkDependency;
  chunks.push_back(pricing);

  JsChunk runtime;
  runtime.route = "/";
  runtime.source = "browser/jsx-runtime.js";
  runtime.code = "export default { createElement() {} };\n";
  runtime.order = 2;
  runtime.flags = JsChunkBrowser | JsChunkModule | JsChunkDependency;
  chunks.push_back(runtime);

  const auto module_graph = graph::build_canonical_module_graph(chunks, {}, "canonical-test", true);
  if (module_graph.modules.size() != 3u || module_graph.dependencies.size() != 2u) {
    throw std::runtime_error("canonical graph counts are incorrect");
  }
  const auto* app = module_graph.find_by_source("/", "browser/app.tsx");
  if (app == nullptr || app->runtime_source.rfind("s_", 0) != 0u) {
    throw std::runtime_error("opaque runtime module identity was not generated");
  }
  const auto* protected_module = module_graph.resolve(*app, "../protected/pricing");
  const auto* browser_module = module_graph.resolve(*app, "./jsx-runtime");
  if (protected_module == nullptr || protected_module->source != "protected/pricing.ts") {
    throw std::runtime_error("extensionless protected import did not resolve");
  }
  if (browser_module == nullptr || browser_module->source != "browser/jsx-runtime.js") {
    throw std::runtime_error("extensionless browser import did not resolve");
  }
  const auto prefix = graph::browser_module_map_prefix(module_graph, *app);
  if (prefix.find("/*@venom-module-map-v1") == std::string::npos ||
      prefix.find(graph::module_hex_text("../protected/pricing")) == std::string::npos ||
      prefix.find(graph::module_hex_text("./jsx-runtime")) == std::string::npos) {
    throw std::runtime_error("canonical graph did not emit complete browser module map");
  }
  std::cout << "canonical module graph: PASS\n";
  return 0;
}
