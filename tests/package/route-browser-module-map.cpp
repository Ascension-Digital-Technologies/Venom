#include "build_package_metadata.hpp"
#include "venom/pipeline/js.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t read_u32(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 4u > bytes.size()) throw std::runtime_error("truncated u32");
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::string hex_text(const std::string& value) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string out;
  for (const char raw_c : value) { const auto c = static_cast<unsigned char>(raw_c);
    out.push_back(digits[c >> 4u]);
    out.push_back(digits[c & 15u]);
  }
  return out;
}
}

int main() {
  using namespace venom::compiler;
  std::vector<JsChunk> chunks;
  JsChunk app;
  app.route = "/";
  app.source = "browser/app.ts";
  app.code = "import { calculateQuote } from '../protected/pricing';\nimport formatMoney from './format';\nconsole.log(formatMoney(calculateQuote({ subtotal: 1 })));\n";
  app.order = 0u;
  app.flags = JsChunkBrowser | JsChunkModule;
  chunks.push_back(std::move(app));

  JsChunk pricing;
  pricing.route = "/";
  pricing.source = "protected/pricing.ts";
  pricing.code = "export function calculateQuote(input) { return input.subtotal; }\n";
  pricing.order = 1u;
  pricing.flags = JsChunkBrowser | JsChunkModule | JsChunkDependency;
  chunks.push_back(std::move(pricing));

  JsChunk format;
  format.route = "/";
  format.source = "browser/format.ts";
  format.code = "export default function formatMoney(value) { return String(value); }\n";
  format.order = 2u;
  format.flags = JsChunkBrowser | JsChunkModule | JsChunkDependency;
  chunks.push_back(std::move(format));

  const auto module_graph = graph::build_canonical_module_graph(chunks, {}, "route-test", true);
  const auto bundle = build_package_detail::encode_route_script_bundle(chunks, module_graph, true);
  if (bundle.size() < 24u || std::string(bundle.begin(), bundle.begin() + 8u) != "VJSB0006") {
    throw std::runtime_error("invalid route script bundle header");
  }
  const auto count = read_u32(bundle, 12u);
  const auto text_size = read_u32(bundle, 16u);
  if (count != 3u) throw std::runtime_error("unexpected route script count");
  const std::size_t entries_offset = 24u;
  const std::size_t entry_size = 40u;
  const std::size_t code_offset = entries_offset + count * entry_size + text_size;
  const auto app_code_offset = read_u32(bundle, entries_offset + 16u);
  const auto app_code_size = read_u32(bundle, entries_offset + 20u);
  if (code_offset + app_code_offset + app_code_size > bundle.size()) {
    throw std::runtime_error("app code is outside bundle");
  }
  const std::string app_code(bundle.begin() + static_cast<std::ptrdiff_t>(code_offset + app_code_offset),
                             bundle.begin() + static_cast<std::ptrdiff_t>(code_offset + app_code_offset + app_code_size));
  if (app_code.find("/*@venom-module-map-v1\n") == std::string::npos) {
    throw std::runtime_error("production route serializer omitted browser module map");
  }
  if (app_code.find(hex_text("../protected/pricing")) == std::string::npos ||
      app_code.find(hex_text("./format")) == std::string::npos) {
    throw std::runtime_error("production route serializer omitted relative import mapping");
  }
  if (app_code.find("s_") != std::string::npos) {
    // Targets are hex encoded inside the map, so readable opaque labels should not appear.
    throw std::runtime_error("module map leaked unencoded opaque target");
  }
  std::cout << "route browser module map: PASS\n";
  return 0;
}
