#include "js_rewriting.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
  using namespace venom::compiler;
  std::vector<JsChunk> chunks;

  JsChunk browser;
  browser.route = "/";
  browser.source = "browser/app.tsx";
  browser.code = "import { calculate as quote } from '../protected/pricing';\nconst result=await quote(24.5,4);\n";
  browser.flags = JsChunkBrowser | JsChunkModule;
  browser.order = 0;
  chunks.push_back(browser);

  JsChunk pricing;
  pricing.route = "/";
  pricing.source = "protected/pricing.ts";
  pricing.code = "export function calculate(value,quantity){return value*quantity;}\n";
  pricing.flags = JsChunkModule | JsChunkDependency | JsChunkProtectedModule;
  pricing.typescript_transpiled = true;
  pricing.order = 1;
  chunks.push_back(pricing);

  const auto result = detail::apply_protected_module_rewrites(chunks, "browser-import-lowering", false);
  const auto& app = chunks.at(0).code;
  if (app.find("../protected/pricing") != std::string::npos)
    throw std::runtime_error("protected browser import survived lowering");
  if (app.find("const quote=(...__venomArgs)=>globalThis.__venomInvokeProtectedById") == std::string::npos)
    throw std::runtime_error("inline protected bridge binding was not emitted");
  if (result.lowered_browser_imports.size() != 1u ||
      result.lowered_browser_imports[0].first != "browser/app.tsx" ||
      result.lowered_browser_imports[0].second != "../protected/pricing")
    throw std::runtime_error("lowered browser import ledger is incorrect");
  if ((chunks.at(1).flags & JsChunkBrowser) == 0u || (chunks.at(1).flags & JsChunkDependency) == 0u)
    throw std::runtime_error("protected facade flags are invalid");
  std::cout << "protected browser import lowering: PASS\n";
  return 0;
}
