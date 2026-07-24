#pragma once

#include "graph/module_types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler::graph {

enum class ModuleRuntime : std::uint8_t { Protected, Browser, Review };
enum class ModuleKind : std::uint8_t { ClassicScript, EsModule, TypeScript, Tsx, ProtectedFacade, External };

struct ModuleRecord {
  std::string id;
  std::string route;
  std::string source;
  std::string runtime_source;
  ModuleRuntime runtime = ModuleRuntime::Protected;
  ModuleKind kind = ModuleKind::ClassicScript;
  std::uint32_t flags = 0;
  std::uint32_t order = 0;
};

struct ModuleDependency {
  std::string importer_id;
  std::string dependency_id;
  std::string specifier;
  std::string resolved_source;
  bool dynamic = false;
  bool preload = false;
  bool external = false;
};

struct CanonicalModuleGraph {
  std::vector<ModuleRecord> modules;
  std::vector<ModuleDependency> dependencies;
  bool opaque_runtime_ids = false;

  const ModuleRecord* find_by_id(const std::string& id) const;
  const ModuleRecord* find_by_source(const std::string& route, const std::string& source) const;
  const ModuleRecord* resolve(const ModuleRecord& importer, const std::string& specifier) const;
};

std::string normalize_module_path(std::string value);
std::vector<std::string> module_resolution_candidates(const std::string& referrer,
                                                      const std::string& specifier);
std::string module_hex_text(const std::string& value);
std::string canonical_module_id(const std::string& route, const std::string& source);
std::string runtime_module_id(const std::string& salt,
                              const std::string& route,
                              const std::string& source,
                              std::uint32_t order,
                              bool opaque);

CanonicalModuleGraph build_canonical_module_graph(const std::vector<JsChunk>& chunks,
                                                  const std::vector<ModuleGraphEdge>& discovered_edges,
                                                  const std::string& diversification_salt,
                                                  bool opaque_runtime_ids);

std::string browser_module_map_prefix(const CanonicalModuleGraph& graph,
                                      const ModuleRecord& module);
std::string browser_module_identity_prefix(const ModuleRecord& module);

} // namespace venom::compiler::graph
