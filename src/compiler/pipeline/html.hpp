#pragma once

#include "compiler/core/site.hpp"
#include "vm/bytecode.hpp"
#include "vm/polymorph.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler {

struct CompiledHtmlRoute {
  std::string route;
  std::string source_path;
  std::uint32_t bytecode_offset = 0;
  std::uint32_t bytecode_size = 0;
  std::uint32_t instruction_count = 0;
  venom::vm::Program program;
};


struct HtmlPreloadHint {
  std::string href;
  std::string relation;
  std::string as_type;
  std::string mime_type;
  bool crossorigin = true;
};

struct CompiledHtmlRoutes {
  std::vector<std::string> strings;
  std::vector<CompiledHtmlRoute> routes;
  std::vector<unsigned char> string_table;
  std::vector<unsigned char> route_table;
  std::vector<unsigned char> bytecode;
  std::string preview;
};

std::string make_bootstrap_html(const SiteGraph& graph,
                                const std::string& loader_public_path,
                                const std::string& style_public_path,
                                const std::string& loader_integrity = {},
                                const std::string& style_integrity = {},
                                const std::vector<HtmlPreloadHint>& preload_hints = {});
std::string html_manifest(const SiteGraph& graph, const CompiledHtmlRoutes* compiled = nullptr, const std::string& asset_manifest = {});
CompiledHtmlRoutes compile_html_routes(const SiteGraph& graph, const venom::vm::PolymorphicPlan& plan);

} // namespace venom::compiler
