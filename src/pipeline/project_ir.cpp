#include "base/error.hpp"
#include "pipeline/project_ir.hpp"
#include "package/hash.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <tuple>
#include <stdexcept>

namespace venom::compiler {
namespace {
std::string escape_json(const std::string& value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (const char raw_c : value) {
    const auto c = static_cast<unsigned char>(raw_c);
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          static constexpr char hex[] = "0123456789abcdef";
          out += "\\u00";
          out += hex[(c >> 4) & 0xf];
          out += hex[c & 0xf];
        } else out.push_back(static_cast<char>(c));
    }
  }
  return out;
}
void string_array(std::ostringstream& out, const std::vector<std::string>& values) {
  out << '[';
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i) out << ',';
    out << '"' << escape_json(values[i]) << '"';
  }
  out << ']';
}
} // namespace

ProjectIr make_project_ir(const SiteGraph& graph) {
  ProjectIr ir;
  ir.root = graph.root;
  ir.routes = graph.routes;
  ir.scripts = graph.scripts;
  ir.styles = graph.styles;
  ir.assets = graph.assets;
  std::ostringstream fingerprint_material;
  fingerprint_material << "venom-project-ir:" << kProjectIrVersion << '\n';
  for (const auto& file : graph.files) {
    const auto digest = venom::package::sha256_hex(file.bytes);
    ir.files.push_back(ProjectIrFile{file.relative, file.extension,
                                     static_cast<std::uint64_t>(file.bytes.size()), digest});
    fingerprint_material << file.relative << '\0' << file.extension << '\0'
                         << file.bytes.size() << '\0' << digest << '\n';
  }
  const auto material = fingerprint_material.str();
  ir.project_fingerprint = venom::package::sha256_hex(
      std::vector<unsigned char>(material.begin(), material.end()));
  return ir;
}

void enrich_project_ir(ProjectIr& ir, const JsBridge& bridge) {
  ir.protected_modules.clear();
  ir.protected_modules.reserve(bridge.chunks.size());
  for (const auto& chunk : bridge.chunks) {
    ir.protected_modules.push_back(ProjectIrModule{chunk.route, chunk.source, chunk.order, chunk.flags,
                                                   static_cast<std::uint64_t>(chunk.code.size()),
                                                   chunk.ast_node_count, chunk.ast_function_count,
                                                   chunk.ast_declaration_count, chunk.ast_import_count,
                                                   chunk.ast_export_count, chunk.ast_top_level_declaration_count,
                                                   chunk.ast_lexical_scope_count, chunk.ast_global_reference_count,
                                                   chunk.typescript_transpiled, chunk.typescript_erased_declarations,
                                                   chunk.typescript_erased_annotations, chunk.typescript_erased_assertions});
  }
  ir.module_edges = bridge.module_edges;
  ir.protected_exports = bridge.protected_exports;
  ir.protected_contracts_json = bridge.protected_contracts_json;
  std::sort(ir.protected_modules.begin(), ir.protected_modules.end(), [](const auto& a, const auto& b) {
    return std::tie(a.route, a.order, a.source) < std::tie(b.route, b.order, b.source);
  });
  std::sort(ir.module_edges.begin(), ir.module_edges.end(), [](const auto& a, const auto& b) {
    return std::tie(a.route, a.referrer, a.specifier, a.resolved, a.dynamic, a.preload) <
           std::tie(b.route, b.referrer, b.specifier, b.resolved, b.dynamic, b.preload);
  });
  std::sort(ir.protected_exports.begin(), ir.protected_exports.end());
  std::ostringstream material;
  material << "venom-project-plan:" << kProjectIrVersion << '\n' << ir.project_fingerprint << '\n';
  for (const auto& module : ir.protected_modules) {
    material << module.route << '\t' << module.order << '\t' << module.flags << '\t'
             << module.source_bytes << '\t' << module.ast_node_count << '\t'
             << module.ast_function_count << '\t' << module.ast_declaration_count << '\t'
             << module.ast_import_count << '\t' << module.ast_export_count << '\t'
             << module.ast_top_level_declaration_count << '\t'
             << module.ast_lexical_scope_count << '\t' << module.ast_global_reference_count << '\t'
             << module.typescript_transpiled << '\t' << module.typescript_erased_declarations << '\t'
             << module.typescript_erased_annotations << '\t' << module.typescript_erased_assertions << '\t'
             << module.source << '\n';
  }
  for (const auto& edge : ir.module_edges) {
    material << edge.route << '\t' << edge.referrer << '\t' << edge.specifier << '\t'
             << edge.resolved << '\t' << edge.dynamic << '\t' << edge.preload << '\n';
  }
  for (const auto& item : ir.protected_exports) material << item.first << '\t' << item.second << '\n';
  material << ir.protected_contracts_json << '\n';
  const auto text = material.str();
  ir.plan_fingerprint = venom::package::sha256_hex(std::vector<unsigned char>(text.begin(), text.end()));
}

std::string project_ir_summary(const ProjectIr& ir) {
  std::ostringstream out;
  const auto typed = std::count_if(ir.protected_modules.begin(), ir.protected_modules.end(), [](const auto& module) {
    return module.typescript_transpiled;
  });
  out << "IR v" << ir.version << ": files=" << ir.files.size() << " routes=" << ir.routes.size()
      << " protected_modules=" << ir.protected_modules.size() << " typescript=" << typed
      << " edges=" << ir.module_edges.size() << " exports=" << ir.protected_exports.size()
      << " contracts=" << (ir.protected_contracts_json.empty() ? 0 : ir.protected_exports.size());
  if (!ir.plan_fingerprint.empty()) out << " digest=" << ir.plan_fingerprint.substr(0, 12);
  return out.str();
}

std::string serialize_project_ir(const ProjectIr& ir) {
  std::ostringstream out;
  out << "{\n  \"schema\":\"venom-project-ir\",\n"
      << "  \"version\":" << ir.version << ",\n"
      << "  \"root\":\"" << escape_json(ir.root.generic_string()) << "\",\n"
      << "  \"project_fingerprint\":\"" << ir.project_fingerprint << "\",\n"
      << "  \"files\":[";
  for (std::size_t i = 0; i < ir.files.size(); ++i) {
    if (i) out << ',';
    const auto& f = ir.files[i];
    out << "{\"path\":\"" << escape_json(f.path) << "\",\"extension\":\""
        << escape_json(f.extension) << "\",\"size\":" << f.size
        << ",\"sha256\":\"" << f.content_sha256 << "\"}";
  }
  out << "],\n  \"routes\":"; string_array(out, ir.routes);
  out << ",\n  \"scripts\":"; string_array(out, ir.scripts);
  out << ",\n  \"styles\":"; string_array(out, ir.styles);
  out << ",\n  \"assets\":"; string_array(out, ir.assets);
  out << ",\n  \"protected_modules\":[";
  for (std::size_t i = 0; i < ir.protected_modules.size(); ++i) {
    if (i) out << ',';
    const auto& module = ir.protected_modules[i];
    out << "{\"route\":\"" << escape_json(module.route) << "\",\"source\":\""
        << escape_json(module.source) << "\",\"order\":" << module.order
        << ",\"flags\":" << module.flags << ",\"source_bytes\":" << module.source_bytes
        << ",\"ast_nodes\":" << module.ast_node_count
        << ",\"ast_functions\":" << module.ast_function_count
        << ",\"ast_declarations\":" << module.ast_declaration_count
        << ",\"ast_imports\":" << module.ast_import_count
        << ",\"ast_exports\":" << module.ast_export_count
        << ",\"ast_top_level_declarations\":" << module.ast_top_level_declaration_count
        << ",\"ast_lexical_scopes\":" << module.ast_lexical_scope_count
        << ",\"ast_global_references\":" << module.ast_global_reference_count
        << ",\"typescript_transpiled\":" << (module.typescript_transpiled ? "true" : "false")
        << ",\"typescript_erased_declarations\":" << module.typescript_erased_declarations
        << ",\"typescript_erased_annotations\":" << module.typescript_erased_annotations
        << ",\"typescript_erased_assertions\":" << module.typescript_erased_assertions << '}';
  }
  out << "],\n  \"module_edge_count\":" << ir.module_edges.size()
      << ",\n  \"protected_export_count\":" << ir.protected_exports.size()
      << ",\n  \"protected_contracts\":" << (ir.protected_contracts_json.empty() ? "{\"schema\":\"venom-protected-contracts\",\"version\":1,\"exports\":[]}" : ir.protected_contracts_json)
      << ",\n  \"plan_fingerprint\":\"" << ir.plan_fingerprint << "\"\n}\n";
  return out.str();
}

void write_project_ir_atomic(const ProjectIr& ir, const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());
  const auto temporary = path.string() + ".tmp";
  {
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) raise_error("VENOM-E5000", "failed to write project IR: " + temporary);
    out << serialize_project_ir(ir);
    if (!out) raise_error("VENOM-E5000", "failed to flush project IR: " + temporary);
  }
  std::error_code ec;
  std::filesystem::rename(temporary, path, ec);
  if (ec) {
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(temporary, path, ec);
    if (ec) raise_error("VENOM-E5000", "failed to commit project IR: " + path.string());
  }
}

} // namespace venom::compiler
