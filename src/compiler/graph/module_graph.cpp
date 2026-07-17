#include "compiler/graph/module_graph.hpp"

#include "compiler/pipeline/js.hpp"
#include "compiler/pipeline/js_frontend.hpp"
#include "package/hash.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace venom::compiler::graph {
namespace {

std::string module_directory(const std::string& source) {
  const auto normalized = normalize_module_path(source);
  const auto slash = normalized.rfind('/');
  return slash == std::string::npos ? std::string{} : normalized.substr(0, slash + 1);
}

std::string short_sha256(const std::string& material, std::size_t size) {
  const std::vector<unsigned char> bytes(material.begin(), material.end());
  return venom::package::sha256_hex(bytes).substr(0, size);
}

ModuleKind module_kind_for(const JsChunk& chunk) {
  if ((chunk.flags & JsChunkModule) == 0u) return ModuleKind::ClassicScript;
  const auto source = normalize_module_path(chunk.source);
  if (source.size() >= 4u && source.compare(source.size() - 4u, 4u, ".tsx") == 0) return ModuleKind::Tsx;
  if ((source.size() >= 3u && source.compare(source.size() - 3u, 3u, ".ts") == 0) ||
      (source.size() >= 4u && source.compare(source.size() - 4u, 4u, ".mts") == 0) ||
      (source.size() >= 4u && source.compare(source.size() - 4u, 4u, ".cts") == 0)) return ModuleKind::TypeScript;
  return ModuleKind::EsModule;
}

const ModuleRecord* find_source_any_route(const CanonicalModuleGraph& graph,
                                          const std::string& source) {
  const auto normalized = normalize_module_path(source);
  const ModuleRecord* match = nullptr;
  for (const auto& module : graph.modules) {
    if (normalize_module_path(module.source) != normalized) continue;
    if (match != nullptr) return nullptr; // ambiguous across routes
    match = &module;
  }
  return match;
}

void append_dependency(CanonicalModuleGraph& graph,
                       const ModuleRecord& importer,
                       const std::string& specifier,
                       bool dynamic,
                       bool preload) {
  ModuleDependency edge;
  edge.importer_id = importer.id;
  edge.specifier = specifier;
  edge.dynamic = dynamic;
  edge.preload = preload;
  if (specifier.empty() || (specifier.front() != '.' && specifier.front() != '/')) {
    edge.external = true;
    graph.dependencies.push_back(std::move(edge));
    return;
  }
  if (const auto* dependency = graph.resolve(importer, specifier)) {
    edge.dependency_id = dependency->id;
    edge.resolved_source = dependency->source;
  }
  graph.dependencies.push_back(std::move(edge));
}

} // namespace

const ModuleRecord* CanonicalModuleGraph::find_by_id(const std::string& id) const {
  const auto it = std::find_if(modules.begin(), modules.end(), [&](const auto& module) { return module.id == id; });
  return it == modules.end() ? nullptr : &*it;
}

const ModuleRecord* CanonicalModuleGraph::find_by_source(const std::string& route,
                                                         const std::string& source) const {
  const auto normalized = normalize_module_path(source);
  const auto it = std::find_if(modules.begin(), modules.end(), [&](const auto& module) {
    return module.route == route && normalize_module_path(module.source) == normalized;
  });
  return it == modules.end() ? nullptr : &*it;
}

const ModuleRecord* CanonicalModuleGraph::resolve(const ModuleRecord& importer,
                                                  const std::string& specifier) const {
  for (const auto& candidate : module_resolution_candidates(importer.source, specifier)) {
    if (const auto* local = find_by_source(importer.route, candidate)) return local;
    if (const auto* unique = find_source_any_route(*this, candidate)) return unique;
  }
  return nullptr;
}

std::string normalize_module_path(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  std::vector<std::string> parts;
  std::size_t begin = 0;
  while (begin <= value.size()) {
    const auto end = value.find('/', begin);
    const auto part = value.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
    if (!part.empty() && part != ".") {
      if (part == "..") { if (!parts.empty()) parts.pop_back(); }
      else parts.push_back(part);
    }
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < parts.size(); ++i) { if (i) out << '/'; out << parts[i]; }
  return out.str();
}

std::vector<std::string> module_resolution_candidates(const std::string& referrer,
                                                      const std::string& specifier) {
  if (specifier.empty() || (specifier.front() != '.' && specifier.front() != '/')) return {};
  const auto base = normalize_module_path(specifier.front() == '/'
      ? specifier.substr(1)
      : module_directory(referrer) + specifier);
  std::vector<std::string> candidates{base};
  const auto slash = base.rfind('/');
  const auto dot = base.rfind('.');
  const bool has_extension = dot != std::string::npos && (slash == std::string::npos || dot > slash);
  if (!has_extension) {
    for (const auto* ext : {".js", ".mjs", ".cjs", ".jsx", ".ts", ".tsx", ".mts", ".cts"}) candidates.push_back(base + ext);
    for (const auto* ext : {".js", ".mjs", ".jsx", ".ts", ".tsx", ".mts", ".cts"}) candidates.push_back(base + "/index" + ext);
  } else if (base.size() > 3u && base.compare(base.size() - 3u, 3u, ".js") == 0) {
    const auto stem = base.substr(0, base.size() - 3u);
    for (const auto* ext : {".ts", ".tsx", ".mts"}) candidates.push_back(stem + ext);
  }
  return candidates;
}

std::string module_hex_text(const std::string& value) {
  static constexpr char digits[] = "0123456789abcdef";
  std::string out;
  out.reserve(value.size() * 2u);
  for (const char raw_c : value) { const auto c = static_cast<unsigned char>(raw_c); out.push_back(digits[c >> 4u]); out.push_back(digits[c & 15u]); }
  return out;
}

std::string canonical_module_id(const std::string& route, const std::string& source) {
  return "m_" + short_sha256("venom-module-v1|" + route + "|" + normalize_module_path(source), 16u);
}

std::string runtime_module_id(const std::string& salt,
                              const std::string& route,
                              const std::string& source,
                              std::uint32_t order,
                              bool opaque) {
  if (!opaque) return source;
  return "s_" + short_sha256("venom-runtime-module-v1|" + salt + "|" + route + "|" + normalize_module_path(source) + "|" + std::to_string(order), 16u);
}

CanonicalModuleGraph build_canonical_module_graph(const std::vector<JsChunk>& chunks,
                                                  const std::vector<ModuleGraphEdge>& discovered_edges,
                                                  const std::string& diversification_salt,
                                                  bool opaque_runtime_ids) {
  CanonicalModuleGraph graph;
  graph.opaque_runtime_ids = opaque_runtime_ids;
  graph.modules.reserve(chunks.size());
  for (const auto& chunk : chunks) {
    ModuleRecord module;
    module.id = canonical_module_id(chunk.route, chunk.source);
    module.route = chunk.route;
    module.source = normalize_module_path(chunk.source);
    module.runtime_source = runtime_module_id(diversification_salt, chunk.route, chunk.source, chunk.order, opaque_runtime_ids);
    module.runtime = (chunk.flags & JsChunkBrowser) != 0u ? ModuleRuntime::Browser : ModuleRuntime::Protected;
    module.kind = module_kind_for(chunk);
    module.flags = chunk.flags;
    module.order = chunk.order;
    graph.modules.push_back(std::move(module));
  }

  std::unordered_set<std::string> seen;
  for (const auto& chunk : chunks) {
    if ((chunk.flags & JsChunkModule) == 0u) continue;
    const auto* importer = graph.find_by_source(chunk.route, chunk.source);
    if (importer == nullptr) continue;
    const auto parsed = frontend::parse(chunk.code, chunk.source, true);
    for (const auto& reference : parsed.module_references) {
      const auto key = importer->id + "\n" + reference.specifier + "\n" + (reference.dynamic ? "1" : "0");
      if (seen.insert(key).second) append_dependency(graph, *importer, reference.specifier, reference.dynamic, false);
    }
  }
  for (const auto& discovered : discovered_edges) {
    const auto* importer = graph.find_by_source(discovered.route, discovered.referrer);
    if (importer == nullptr) continue;
    const auto key = importer->id + "\n" + discovered.specifier + "\n" + (discovered.dynamic ? "1" : "0");
    if (seen.insert(key).second) append_dependency(graph, *importer, discovered.specifier, discovered.dynamic, discovered.preload);
  }
  return graph;
}

std::string browser_module_identity_prefix(const ModuleRecord& module) {
  if (module.runtime != ModuleRuntime::Browser || module.kind == ModuleKind::ClassicScript) return {};
  return "/*@venom-module-source-v1\n" + module_hex_text(normalize_module_path(module.source)) + "\n*/\n";
}

std::string browser_module_map_prefix(const CanonicalModuleGraph& graph,
                                      const ModuleRecord& module) {
  if (module.runtime != ModuleRuntime::Browser || module.kind == ModuleKind::ClassicScript) return {};
  std::ostringstream map;
  bool any = false;
  map << "/*@venom-module-map-v1\n";
  for (const auto& edge : graph.dependencies) {
    if (edge.importer_id != module.id || edge.dynamic || edge.dependency_id.empty()) continue;
    const auto* dependency = graph.find_by_id(edge.dependency_id);
    if (dependency == nullptr) continue;
    map << module_hex_text(edge.specifier) << '\t' << module_hex_text(dependency->runtime_source) << '\n';
    any = true;
  }
  map << "*/\n";
  return any ? map.str() : std::string{};
}

} // namespace venom::compiler::graph
