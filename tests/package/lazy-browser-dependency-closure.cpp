#include "compiler/package/section_plan.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

int main() {
  using namespace venom::compiler;
  std::vector<JsChunk> chunks;
  JsChunk app;
  app.route = "/";
  app.source = "browser/app.tsx";
  app.order = 10u;
  app.flags = JsChunkBrowser | JsChunkModule;
  chunks.push_back(app);

  JsChunk pricing;
  pricing.route = "/generated/protected-facade";
  pricing.source = "protected/pricing.ts";
  pricing.order = 20u;
  pricing.flags = JsChunkBrowser | JsChunkModule | JsChunkDependency;
  chunks.push_back(pricing);

  JsChunk unrelated;
  unrelated.route = "/other";
  unrelated.source = "browser/other.js";
  unrelated.order = 30u;
  unrelated.flags = JsChunkBrowser | JsChunkModule;
  chunks.push_back(unrelated);

  const auto selected = package_sections::select_lazy_route_scripts(chunks, "/");
  if (selected.size() != 2u) throw std::runtime_error("lazy route did not include exact dependency closure");
  if (selected[0].source != "browser/app.tsx") throw std::runtime_error("route entry missing or out of order");
  if (selected[1].source != "protected/pricing.ts") throw std::runtime_error("cross-route protected facade missing");
  std::cout << "lazy browser dependency closure: PASS\n";
  return 0;
}
