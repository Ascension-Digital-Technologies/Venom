#pragma once

#include "venom/core/site.hpp"
#include "venom/pipeline/js.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler {

inline constexpr std::uint32_t kProjectIrVersion = 12;

struct ProjectIrFile {
  std::string path;
  std::string extension;
  std::uint64_t size = 0;
  std::string content_sha256;
};

struct ProjectIrModule {
  std::string route;
  std::string source;
  std::uint32_t order = 0;
  std::uint32_t flags = 0;
  std::uint64_t source_bytes = 0;
  std::uint32_t ast_node_count = 0;
  std::uint32_t ast_function_count = 0;
  std::uint32_t ast_declaration_count = 0;
  std::uint32_t ast_import_count = 0;
  std::uint32_t ast_export_count = 0;
  std::uint32_t ast_top_level_declaration_count = 0;
  std::uint32_t ast_lexical_scope_count = 0;
  std::uint32_t ast_global_reference_count = 0;
  bool typescript_transpiled = false;
  std::uint32_t typescript_erased_declarations = 0;
  std::uint32_t typescript_erased_annotations = 0;
  std::uint32_t typescript_erased_assertions = 0;
};

struct ProjectIr {
  std::uint32_t version = kProjectIrVersion;
  std::filesystem::path root;
  std::string project_fingerprint;
  std::vector<ProjectIrFile> files;
  std::vector<std::string> routes;
  std::vector<std::string> scripts;
  std::vector<std::string> styles;
  std::vector<std::string> assets;
  std::vector<ProjectIrModule> protected_modules;
  std::vector<ModuleGraphEdge> module_edges;
  std::vector<std::pair<std::string, std::string>> protected_exports;
  std::string protected_contracts_json;
  std::string plan_fingerprint;
};

ProjectIr make_project_ir(const SiteGraph& graph);
void enrich_project_ir(ProjectIr& ir, const JsBridge& bridge);
std::string project_ir_summary(const ProjectIr& ir);
std::string serialize_project_ir(const ProjectIr& ir);
void write_project_ir_atomic(const ProjectIr& ir, const std::filesystem::path& path);

} // namespace venom::compiler
