#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace venom::compiler {

struct CapabilityOccurrence {
  std::filesystem::path file;
  std::size_t line = 0;
  std::size_t column = 0;
  std::string kind;
  std::string name;
  std::string detail;
};

struct CapabilityGraph {
  std::size_t files_scanned = 0;
  std::size_t javascript_files = 0;
  std::size_t html_files = 0;
  std::map<std::string, std::size_t> syntax;
  std::map<std::string, std::size_t> browser_apis;
  std::map<std::string, std::size_t> node_apis;
  std::map<std::string, std::size_t> frameworks;
  std::map<std::string, std::size_t> module_features;
  std::vector<CapabilityOccurrence> occurrences;
};

CapabilityGraph analyze_capabilities(const std::filesystem::path& input);
std::string capability_graph_json(const CapabilityGraph& graph);
void print_capability_graph_text(const CapabilityGraph& graph);

} // namespace venom::compiler
