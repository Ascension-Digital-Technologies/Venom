#include "compiler/pipeline/js_rewriting.hpp"
#include "compiler/core/diagnostic.hpp"
#include "compiler/pipeline/function_dependencies.hpp"
#include "compiler/pipeline/js_frontend.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace venom::compiler::detail {
namespace {

std::uint64_t bridge_id_hash(std::string_view value) {
  std::uint64_t hash = 1469598103934665603ull;
  for (const char raw_c : value) {
    const auto c = static_cast<unsigned char>(raw_c);
    hash ^= static_cast<std::uint64_t>(c);
    hash *= 1099511628211ull;
  }
  return hash;
}

std::string opaque_bridge_id(std::string_view source, std::string_view name, std::string_view salt) {
  const auto h1 = bridge_id_hash(std::string(source) + "|" + std::string(name) + "|" + std::string(salt));
  const auto h2 = bridge_id_hash(std::string(salt) + "|" + std::string(name) + "|" + std::string(source));
  std::ostringstream out;
  out << "x" << std::hex << std::setfill('0') << std::setw(16) << h1 << std::setw(16) << h2;
  return out.str();
}

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

std::string normalize_slashes(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  while (!value.empty() && value.front() == '/') {
    value.erase(value.begin());
  }
  while (value.rfind("./", 0) == 0) {
    value.erase(0, 2);
  }
  std::vector<std::string> parts;
  std::string current;
  std::stringstream input(value);
  while (std::getline(input, current, '/')) {
    if (current.empty() || current == ".") {
      continue;
    }
    if (current == "..") {
      if (!parts.empty()) {
        parts.pop_back();
      }
      continue;
    }
    parts.push_back(current);
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) out << '/';
    out << parts[i];
  }
  return out.str();
}

std::string dirname_of(std::string path) {
  path = normalize_slashes(std::move(path));
  const auto pos = path.find_last_of('/');
  return pos == std::string::npos ? std::string{} : path.substr(0, pos + 1);
}


std::string json_escape_plan(const std::string& value) {
  std::ostringstream out;
  for (const char raw_c : value) {
    const auto c = static_cast<unsigned char>(raw_c);
    switch (c) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20u) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned>(c) << std::dec;
        } else {
          out << static_cast<char>(c);
        }
    }
  }
  return out.str();
}

}  // namespace




struct ProtectedContractField {
  std::string name;
  std::string type;
  bool optional = false;
};

struct ProtectedModuleExport {
  std::string name;
  std::string params;
  bool async = false;
  std::vector<ProtectedContractField> input_contract;
  std::vector<ProtectedContractField> output_contract;
  std::size_t export_begin = 0;
  std::size_t export_end = 0;
};

struct ProtectedModuleImport {
  std::string specifier;
  std::string clause;
  std::size_t begin = 0;
  std::size_t end = 0;
};

struct ProtectedModuleSyntax {
  std::vector<ProtectedModuleExport> exports;
  std::vector<ProtectedModuleImport> imports;
  std::vector<std::string> unsupported_exports;
};


bool has_file_scope_venom_protected_module_directive(const std::string& source) {
  std::size_t i = 0;
  if (source.size() >= 3 && static_cast<unsigned char>(source[0]) == 0xefu &&
      static_cast<unsigned char>(source[1]) == 0xbbu && static_cast<unsigned char>(source[2]) == 0xbfu) i = 3;
  while (i < source.size()) {
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
    if (i + 1 >= source.size()) break;
    std::string comment;
    if (source[i] == '/' && source[i + 1] == '/') {
      const auto end = source.find('\n', i + 2);
      comment = source.substr(i + 2, (end == std::string::npos ? source.size() : end) - (i + 2));
      i = end == std::string::npos ? source.size() : end + 1;
    } else if (source[i] == '/' && source[i + 1] == '*') {
      const auto end = source.find("*/", i + 2);
      if (end == std::string::npos) break;
      comment = source.substr(i + 2, end - (i + 2));
      i = end + 2;
    } else {
      break;
    }
    const auto lower = lower_ascii(comment);
    if (lower.find("@venom:") != std::string::npos &&
        lower.find("protected") != std::string::npos &&
        lower.find("module") != std::string::npos) return true;
  }
  return false;
}

std::string trim_ascii(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}


std::vector<ProtectedContractField> parse_contract_fields(const std::string& text,
                                                          const std::string& source,
                                                          const std::string& direction) {
  std::vector<ProtectedContractField> fields;
  std::stringstream stream(text);
  std::string item;
  std::unordered_set<std::string> names;
  while (std::getline(stream, item, ',')) {
    item = trim_ascii(item);
    if (item.empty()) continue;
    const auto colon = item.find(':');
    if (colon == std::string::npos)
      throw std::runtime_error("VENOM-E2401 malformed " + direction + " contract in " + source + ": " + item);
    auto name = trim_ascii(item.substr(0, colon));
    auto type = trim_ascii(item.substr(colon + 1));
    bool optional = false;
    if (!name.empty() && name.back() == '?') { optional = true; name.pop_back(); }
    static const std::regex ident(R"(^[A-Za-z_$][A-Za-z0-9_$]*$)");
    static const std::unordered_set<std::string> valid = {
      "string", "number", "integer", "boolean", "object", "array", "any",
      "string[]", "number[]", "integer[]", "boolean[]",
      "arraybuffer", "uint8array", "uint8clampedarray", "int8array",
      "uint16array", "int16array", "uint32array", "int32array",
      "float32array", "float64array"
    };
    if (!std::regex_match(name, ident) || valid.find(type) == valid.end())
      throw std::runtime_error("VENOM-E2402 unsupported " + direction + " contract field in " + source + ": " + item);
    if (!names.insert(name).second)
      throw std::runtime_error("VENOM-E2403 duplicate " + direction + " contract field in " + source + ": " + name);
    fields.push_back({name, type, optional});
  }
  return fields;
}

std::vector<ProtectedContractField> contract_annotation_before(const std::string& source,
                                                               std::size_t offset,
                                                               const std::string& kind,
                                                               const std::string& filename) {
  const auto window_begin = offset > 2048 ? offset - 2048 : 0;
  auto prefix = source.substr(window_begin, offset - window_begin);
  const auto boundary = prefix.find_last_of(";}");
  if (boundary != std::string::npos) prefix = prefix.substr(boundary + 1);
  const std::regex annotation("@venom\\s*:\\s*" + kind + "\\s+([^\\r\\n*]+)", std::regex::icase);
  std::sregex_iterator it(prefix.begin(), prefix.end(), annotation), end;
  if (it == end) return {};
  std::string value;
  for (; it != end; ++it) value = trim_ascii((*it)[1].str());
  return parse_contract_fields(value, filename, kind);
}

std::string contract_type_check_js(const std::string& expression, const std::string& type) {
  if (type == "any") return "true";
  if (type == "string") return "typeof " + expression + "==='string'";
  if (type == "number") return "typeof " + expression + "==='number'&&Number.isFinite(" + expression + ")";
  if (type == "integer") return "Number.isInteger(" + expression + ")";
  if (type == "boolean") return "typeof " + expression + "==='boolean'";
  if (type == "object") return expression + "!==null&&typeof " + expression + "==='object'&&!Array.isArray(" + expression + ")";
  if (type == "array") return "Array.isArray(" + expression + ")";
  if (type == "arraybuffer") return expression + " instanceof ArrayBuffer";
  if (type == "uint8array") return expression + " instanceof Uint8Array";
  if (type == "uint8clampedarray") return expression + " instanceof Uint8ClampedArray";
  if (type == "int8array") return expression + " instanceof Int8Array";
  if (type == "uint16array") return expression + " instanceof Uint16Array";
  if (type == "int16array") return expression + " instanceof Int16Array";
  if (type == "uint32array") return expression + " instanceof Uint32Array";
  if (type == "int32array") return expression + " instanceof Int32Array";
  if (type == "float32array") return expression + " instanceof Float32Array";
  if (type == "float64array") return expression + " instanceof Float64Array";
  const auto bracket = type.find("[]");
  if (bracket != std::string::npos) {
    const auto inner = type.substr(0, bracket);
    return "Array.isArray(" + expression + ")&&" + expression + ".every(function(__v){return " + contract_type_check_js("__v", inner) + ";})";
  }
  return "false";
}

std::string contract_validator_js(const std::vector<ProtectedContractField>& fields,
                                  const std::string& expression,
                                  const std::string& label) {
  if (fields.empty()) return {};
  std::ostringstream out;
  out << "if(" << expression << "===null||typeof " << expression << "!=='object'||Array.isArray(" << expression
      << "))throw new TypeError('" << label << " must be an object');\n";
  for (const auto& field : fields) {
    const auto access = expression + "[\"" + json_escape_plan(field.name) + "\"]";
    if (field.optional) {
      out << "if(" << access << "!==undefined&&!(" << contract_type_check_js(access, field.type)
          << "))throw new TypeError('" << label << "." << json_escape_plan(field.name) << " must be " << field.type << "');\n";
    } else {
      out << "if(!Object.prototype.hasOwnProperty.call(" << expression << ",\"" << json_escape_plan(field.name)
          << "\")||!(" << contract_type_check_js(access, field.type) << "))throw new TypeError('" << label << "."
          << json_escape_plan(field.name) << " must be " << field.type << "');\n";
    }
  }
  return out.str();
}

std::string typescript_type(const std::string& type) {
  if (type == "integer") return "number";
  if (type == "object") return "Record<string, unknown>";
  if (type == "array") return "unknown[]";
  if (type == "any") return "unknown";
  if (type == "integer[]") return "number[]";
  if (type == "arraybuffer") return "ArrayBuffer";
  if (type == "uint8array") return "Uint8Array";
  if (type == "uint8clampedarray") return "Uint8ClampedArray";
  if (type == "int8array") return "Int8Array";
  if (type == "uint16array") return "Uint16Array";
  if (type == "int16array") return "Int16Array";
  if (type == "uint32array") return "Uint32Array";
  if (type == "int32array") return "Int32Array";
  if (type == "float32array") return "Float32Array";
  if (type == "float64array") return "Float64Array";
  return type;
}

std::string typescript_object_type(const std::vector<ProtectedContractField>& fields) {
  if (fields.empty()) return "unknown";
  std::ostringstream out;
  out << "{ ";
  for (std::size_t i = 0; i < fields.size(); ++i) {
    if (i) out << ' ';
    out << fields[i].name << (fields[i].optional ? "?" : "") << ": " << typescript_type(fields[i].type) << ";";
  }
  out << " }";
  return out.str();
}

std::string contract_fields_json(const std::vector<ProtectedContractField>& fields) {
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < fields.size(); ++i) {
    if (i) out << ',';
    out << "{\"name\":\"" << json_escape_plan(fields[i].name)
        << "\",\"type\":\"" << json_escape_plan(fields[i].type)
        << "\",\"optional\":" << (fields[i].optional ? "true" : "false") << '}';
  }
  out << ']';
  return out.str();
}

ProtectedModuleSyntax parse_protected_module_syntax(const JsChunk& chunk) {
  const auto parsed = frontend::parse(chunk.code, chunk.source, true);
  for (const auto& reference : parsed.module_references) {
    if (reference.dynamic) {
      throw std::runtime_error("VENOM-E2205 dynamic import is not supported in protected module: " +
                               chunk.source + ":" + std::to_string(reference.line) + ":" +
                               std::to_string(reference.column + 1));
    }
  }

  ProtectedModuleSyntax syntax;
  syntax.imports.reserve(parsed.module_imports.size());
  for (const auto& imported : parsed.module_imports) {
    if (imported.side_effect_only) {
      throw std::runtime_error("VENOM-E2206 side-effect-only imports are not supported in protected module: " +
                               chunk.source + ":" + std::to_string(imported.line) + ":" +
                               std::to_string(imported.column + 1));
    }
    if (imported.has_default) {
      throw std::runtime_error("VENOM-E2215 default imports are not supported in protected modules: " +
                               chunk.source + ":" + std::to_string(imported.line) + ":" +
                               std::to_string(imported.column + 1));
    }
    syntax.imports.push_back({imported.specifier, imported.clause, imported.begin, imported.end});
  }

  syntax.exports.reserve(parsed.exported_functions.size());
  for (const auto& exported : parsed.exported_functions) {
    ProtectedModuleExport item;
    item.name = exported.name;
    item.params = exported.parameters;
    item.async = exported.async;
    item.input_contract = contract_annotation_before(chunk.code, exported.export_begin, "input", chunk.source);
    item.output_contract = contract_annotation_before(chunk.code, exported.export_begin, "output", chunk.source);
    item.export_begin = exported.export_begin;
    item.export_end = exported.declaration_begin;
    syntax.exports.push_back(std::move(item));
  }
  for (const auto& unsupported : parsed.unsupported_exports) {
    syntax.unsupported_exports.push_back(unsupported.kind + "@" + std::to_string(unsupported.line) + ":" +
                                         std::to_string(unsupported.column + 1));
  }
  return syntax;
}

std::string protected_module_id(const std::string& source, const std::string& salt) {
  return opaque_bridge_id("module:" + normalize_slashes(source), "namespace", salt);
}

std::string resolve_protected_module_specifier(const std::string& referrer, const std::string& specifier,
                                               const std::unordered_map<std::string, std::size_t>& by_source) {
  if (specifier.empty() || specifier.front() != '.')
    throw std::runtime_error("VENOM-E2211 protected module imports must be relative: " + referrer + " -> " + specifier);
  auto resolved = normalize_slashes(dirname_of(referrer) + specifier);
  if (by_source.find(resolved) != by_source.end()) return resolved;
  const auto slash = resolved.find_last_of('/');
  const auto dot = resolved.find_last_of('.');
  const bool has_extension = dot != std::string::npos && (slash == std::string::npos || dot > slash);
  if (!has_extension) {
    for (const auto* ext : {".ts", ".tsx", ".mts", ".js", ".mjs"}) {
      const auto candidate = resolved + ext;
      if (by_source.find(candidate) != by_source.end()) return candidate;
    }
    for (const auto* ext : {".ts", ".tsx", ".mts", ".js", ".mjs"}) {
      const auto candidate = resolved + "/index" + ext;
      if (by_source.find(candidate) != by_source.end()) return candidate;
    }
  } else if (resolved.size() >= 3 && resolved.substr(resolved.size() - 3) == ".js") {
    const auto stem = resolved.substr(0, resolved.size() - 3);
    for (const auto* ext : {".ts", ".tsx", ".mts"}) {
      const auto candidate = stem + ext;
      if (by_source.find(candidate) != by_source.end()) return candidate;
    }
  }
  throw std::runtime_error("VENOM-E2212 protected module dependency not found: " + referrer + " -> " + specifier);
}

std::string import_clause_to_bindings(const ProtectedModuleImport& import, const std::string& dependency_id,
                                      const std::string& source) {
  const auto clause = trim_ascii(import.clause);
  std::ostringstream out;
  const std::string module_ref = "globalThis.__venomProtectedModules[\"" + json_escape_plan(dependency_id) + "\"]";
  if (clause.rfind("*", 0) == 0) {
    const std::regex ns(R"(^\*\s+as\s+([A-Za-z_$][A-Za-z0-9_$]*)$)");
    std::smatch match;
    if (!std::regex_match(clause, match, ns))
      throw std::runtime_error("VENOM-E2213 malformed namespace import in " + source);
    out << "const " << match[1].str() << "=" << module_ref << ";\n";
    return out.str();
  }
  if (!clause.empty() && clause.front() == '{' && clause.back() == '}') {
    auto inner = clause.substr(1, clause.size() - 2);
    std::stringstream items(inner);
    std::string item;
    while (std::getline(items, item, ',')) {
      item = trim_ascii(item);
      if (item.empty()) continue;
      const std::regex named(R"(^([A-Za-z_$][A-Za-z0-9_$]*)(?:\s+as\s+([A-Za-z_$][A-Za-z0-9_$]*))?$)");
      std::smatch match;
      if (!std::regex_match(item, match, named))
        throw std::runtime_error("VENOM-E2214 malformed named import in " + source + ": " + item);
      const auto imported = match[1].str();
      const auto local = match[2].matched ? match[2].str() : imported;
      out << "const " << local << "=" << module_ref << "[\"" << json_escape_plan(imported) << "\"];\n";
    }
    return out.str();
  }
  throw std::runtime_error("VENOM-E2215 default imports are not supported in protected modules: " + source);
}

std::string apply_source_edits(std::string source,
                               std::vector<std::tuple<std::size_t, std::size_t, std::string>> edits) {
  std::sort(edits.begin(), edits.end(), [](const auto& a, const auto& b) { return std::get<0>(a) > std::get<0>(b); });
  for (const auto& edit : edits) {
    source.replace(std::get<0>(edit), std::get<1>(edit) - std::get<0>(edit), std::get<2>(edit));
  }
  return source;
}

std::string protected_registry_bootstrap_js(bool include_modules) {
  std::ostringstream out;
  out << "globalThis.__venomProtectedBridge=globalThis.__venomProtectedBridge||Object.create(null);\n";
  if (include_modules)
    out << "globalThis.__venomProtectedModules=globalThis.__venomProtectedModules||Object.create(null);\n";
  out << "globalThis.__venomBridgeRevive=function(v){if(v&&typeof v==='object'&&!Array.isArray(v)&&typeof v.__venomBinaryValue==='string'&&Array.isArray(v.bytes)){const b=Uint8Array.from(v.bytes),t=v.__venomBinaryValue;if(t==='ArrayBuffer')return b.buffer;const C=globalThis[t];if(typeof C!=='function'||!['Uint8Array','Uint8ClampedArray','Int8Array','Uint16Array','Int16Array','Uint32Array','Int32Array','Float32Array','Float64Array'].includes(t)||b.byteLength%C.BYTES_PER_ELEMENT)throw new TypeError('invalid protected binary value');return new C(b.buffer);}if(Array.isArray(v))return v.map(globalThis.__venomBridgeRevive);if(v&&typeof v==='object')for(const k of Object.keys(v))v[k]=globalThis.__venomBridgeRevive(v[k]);return v;};\n"
      << "globalThis.__venomBridgeEncode=function(v){let t='';if(v instanceof ArrayBuffer)t='ArrayBuffer';else if(ArrayBuffer.isView(v)&&!(v instanceof DataView))t=v.constructor&&v.constructor.name||'';if(t){if(!['ArrayBuffer','Uint8Array','Uint8ClampedArray','Int8Array','Uint16Array','Int16Array','Uint32Array','Int32Array','Float32Array','Float64Array'].includes(t))throw new TypeError('unsupported protected binary result');const b=v instanceof ArrayBuffer?new Uint8Array(v):new Uint8Array(v.buffer,v.byteOffset,v.byteLength);return {__venomBinaryValue:t,bytes:Array.from(b)};}if(Array.isArray(v))return v.map(globalThis.__venomBridgeEncode);if(v&&typeof v==='object'){const o=Object.create(null);for(const k of Object.keys(v))o[k]=globalThis.__venomBridgeEncode(v[k]);return o;}return v;};\n";
  return out.str();
}

ProtectedModuleRewriteResult apply_protected_module_rewrites(std::vector<JsChunk>& chunks,
                                                              const std::string& bridge_id_salt,
                                                              bool development) {
  ProtectedModuleRewriteResult result;
  struct ModuleUnit {
    std::size_t chunk_index = 0;
    std::string source;
    std::string id;
    ProtectedModuleSyntax syntax;
    std::vector<std::string> dependencies;
  };
  std::vector<ModuleUnit> modules;
  std::unordered_map<std::string, std::size_t> by_source;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> public_export_candidates;
  for (std::size_t i = 0; i < chunks.size(); ++i) {
    auto& chunk = chunks[i];
    if ((chunk.flags & JsChunkProtectedModule) == 0u &&
        !has_file_scope_venom_protected_module_directive(chunk.code)) continue;
    if ((chunk.flags & JsChunkBrowser) != 0u)
      throw std::runtime_error("protected module cannot be browser-side: " + chunk.source);
    const auto source = normalize_slashes(chunk.source);
    by_source.emplace(source, modules.size());
    modules.push_back({i, source, protected_module_id(source, bridge_id_salt), parse_protected_module_syntax(chunk), {}});
  }
  if (modules.empty()) return result;

  for (auto& module : modules) {
    if (!module.syntax.unsupported_exports.empty()) {
      std::ostringstream details;
      for (std::size_t i = 0; i < module.syntax.unsupported_exports.size(); ++i) {
        if (i) details << ", ";
        details << module.syntax.unsupported_exports[i];
      }
      throw std::runtime_error("VENOM-E2202 protected modules currently export named function declarations only: " +
                               module.source + " (" + details.str() + ")");
    }
    if (module.syntax.exports.empty())
      throw std::runtime_error("VENOM-E2203 protected module has no named function exports: " + module.source);
    for (const auto& import : module.syntax.imports) {
      module.dependencies.push_back(resolve_protected_module_specifier(module.source, import.specifier, by_source));
    }
  }

  std::unordered_set<std::string> internal_module_sources;
  for (const auto& module : modules) {
    for (const auto& dependency : module.dependencies) internal_module_sources.insert(dependency);
  }

  std::vector<int> state(modules.size(), 0);
  std::vector<std::size_t> order;
  std::function<void(std::size_t)> visit = [&](std::size_t index) {
    if (state[index] == 2) return;
    if (state[index] == 1)
      throw std::runtime_error("VENOM-E2216 protected module import cycle detected at " + modules[index].source);
    state[index] = 1;
    for (const auto& dependency : modules[index].dependencies) visit(by_source.at(dependency));
    state[index] = 2;
    order.push_back(index);
  };
  for (std::size_t i = 0; i < modules.size(); ++i) visit(i);

  std::unordered_set<std::string> public_names;
  std::ostringstream registry;
  std::ostringstream dts;
  registry << protected_registry_bootstrap_js(true);

  for (const auto index : order) {
    auto& module = modules[index];
    auto& chunk = chunks[module.chunk_index];
    std::vector<std::tuple<std::size_t, std::size_t, std::string>> edits;
    std::ostringstream bindings;
    for (std::size_t i = 0; i < module.syntax.imports.size(); ++i) {
      const auto& import = module.syntax.imports[i];
      const auto dependency = resolve_protected_module_specifier(module.source, import.specifier, by_source);
      bindings << import_clause_to_bindings(import, modules[by_source.at(dependency)].id, module.source);
      edits.emplace_back(import.begin, import.end, std::string{});
    }
    for (const auto& entry : module.syntax.exports) edits.emplace_back(entry.export_begin, entry.export_end, std::string{});
    const auto protected_source = apply_source_edits(chunk.code, std::move(edits));

    registry << "\n(()=>{\nconst __venomModule=Object.create(null);\n"
             << "globalThis.__venomProtectedModules[\"" << json_escape_plan(module.id) << "\"]=__venomModule;\n"
             << bindings.str() << protected_source << "\n";

    const bool public_entry_module = internal_module_sources.find(module.source) == internal_module_sources.end();
    std::ostringstream facade;
    facade << "// generated Venom protected-module facade\n";
    for (const auto& entry : module.syntax.exports) {
      registry << "__venomModule[\"" << json_escape_plan(entry.name) << "\"]=" << entry.name << ";\n";
      if (!public_entry_module) continue;
      if (!public_names.insert(entry.name).second)
        throw std::runtime_error("VENOM-E2204 duplicate public protected export name: " + entry.name +
                                 "; rename the browser-facing export or import it through one protected entry module");
      const auto candidate = opaque_bridge_id(module.source, entry.name, bridge_id_salt);
      public_export_candidates[module.source].emplace(entry.name, candidate);
      const auto wrapped = "__venomContract_" + entry.name;
      registry << "const " << wrapped << "=function(){\nconst __a=Array.from(arguments,globalThis.__venomBridgeRevive);\n"
               << contract_validator_js(entry.input_contract, "__a[0]", "input")
               << "const __r=" << entry.name << ".apply(this,__a);\n";
      if (entry.output_contract.empty()) {
        registry << "return globalThis.__venomBridgeEncode(__r);\n";
      } else {
        registry << "if(__r&&typeof __r.then==='function')return __r.then(function(__o){\n"
                 << contract_validator_js(entry.output_contract, "__o", "output")
                 << "return globalThis.__venomBridgeEncode(__o);});\n"
                 << contract_validator_js(entry.output_contract, "__r", "output")
                 << "return globalThis.__venomBridgeEncode(__r);\n";
      }
      registry << "};\n"
               << "globalThis.__venomProtectedBridge[\"" << json_escape_plan(candidate) << "\"]=" << wrapped << ";\n";
      facade << "export async function " << entry.name << "(" << entry.params << "){";
      if (development) facade << contract_validator_js(entry.input_contract, "arguments[0]", "input");
      facade << "const __r=await globalThis.__venomInvokeProtectedById(\""
             << json_escape_plan(candidate) << "\",Array.from(arguments));";
      if (development) facade << contract_validator_js(entry.output_contract, "__r", "output");
      facade << "return __r;}\n";
      dts << "export function " << entry.name << "(input: " << typescript_object_type(entry.input_contract)
          << ", options?: VenomCallOptions): Promise<" << typescript_object_type(entry.output_contract) << ">;\n";
      BridgeRewriteRecord module_record;
      module_record.source = module.source;
      module_record.function = entry.name;
      module_record.candidate = candidate;
      module_record.status = "extracted";
      module_record.reason = "AST-scanned protected module graph export";
      module_record.input_contract_json = contract_fields_json(entry.input_contract);
      module_record.output_contract_json = contract_fields_json(entry.output_contract);
      module_record.input_typescript = typescript_object_type(entry.input_contract);
      module_record.output_typescript = typescript_object_type(entry.output_contract);
      result.records.push_back(std::move(module_record));
    }
    if (!public_entry_module) facade << "export {};\n";
    registry << "})();\n";
    chunk.code = facade.str();
    chunk.flags |= JsChunkBrowser | JsChunkModule | JsChunkDependency;
    chunk.flags &= ~JsChunkBytecodeEncoded;
    chunk.execution_reason = "generated browser facade for isolated protected module graph";
    chunk.execution_confidence = 100u;
  }
  // Lower browser imports of public protected modules directly to bridge
  // bindings. These imports must not remain browser-module dependencies: doing
  // so ties correctness to lazy-section membership and polymorphic module IDs.
  // The protected implementation still executes exclusively in QuickJS/WASM.
  auto resolve_public_protected_target = [&](const std::string& referrer,
                                              const std::string& specifier) -> std::string {
    if (specifier.empty() || specifier.front() != '.') return {};
    auto resolved = normalize_slashes(dirname_of(referrer) + specifier);
    auto has_public = [&](const std::string& candidate) {
      const auto it = public_export_candidates.find(candidate);
      return it != public_export_candidates.end() && !it->second.empty();
    };
    if (has_public(resolved)) return resolved;
    const auto slash = resolved.find_last_of('/');
    const auto dot = resolved.find_last_of('.');
    const bool has_extension = dot != std::string::npos && (slash == std::string::npos || dot > slash);
    if (!has_extension) {
      for (const auto* ext : {".ts", ".tsx", ".mts", ".js", ".mjs"}) {
        const auto candidate = resolved + ext;
        if (has_public(candidate)) return candidate;
      }
      for (const auto* ext : {".ts", ".tsx", ".mts", ".js", ".mjs"}) {
        const auto candidate = resolved + "/index" + ext;
        if (has_public(candidate)) return candidate;
      }
    } else if (resolved.size() >= 3 && resolved.substr(resolved.size() - 3) == ".js") {
      const auto stem = resolved.substr(0, resolved.size() - 3);
      for (const auto* ext : {".ts", ".tsx", ".mts"}) {
        const auto candidate = stem + ext;
        if (has_public(candidate)) return candidate;
      }
    }
    return {};
  };

  for (auto& importer_chunk : chunks) {
    if ((importer_chunk.flags & JsChunkBrowser) == 0u ||
        (importer_chunk.flags & JsChunkModule) == 0u) continue;
    const auto parsed = frontend::parse(importer_chunk.code, importer_chunk.source, true);
    std::vector<std::tuple<std::size_t, std::size_t, std::string>> import_edits;
    for (const auto& imported : parsed.module_imports) {
      const auto target = resolve_public_protected_target(importer_chunk.source, imported.specifier);
      if (target.empty()) continue;
      if (imported.side_effect_only || imported.has_default) {
        throw std::runtime_error("VENOM-E2217 browser imports from protected modules must use named imports: " +
                                 importer_chunk.source + " -> " + imported.specifier);
      }
      const auto clause = trim_ascii(imported.clause);
      if (clause.empty() || clause.front() != '{' || clause.back() != '}') {
        throw std::runtime_error("VENOM-E2218 browser imports from protected modules must use named imports: " +
                                 importer_chunk.source + " -> " + imported.specifier);
      }
      std::ostringstream replacement;
      auto inner = clause.substr(1, clause.size() - 2);
      std::stringstream items(inner);
      std::string item;
      while (std::getline(items, item, ',')) {
        item = trim_ascii(item);
        if (item.empty()) continue;
        const std::regex named(R"(^([A-Za-z_$][A-Za-z0-9_$]*)(?:\s+as\s+([A-Za-z_$][A-Za-z0-9_$]*))?$)");
        std::smatch match;
        if (!std::regex_match(item, match, named)) {
          throw std::runtime_error("VENOM-E2219 malformed protected browser import in " +
                                   importer_chunk.source + ": " + item);
        }
        const auto exported = match[1].str();
        const auto local = match[2].matched ? match[2].str() : exported;
        const auto target_it = public_export_candidates.find(target);
        const auto candidate_it = target_it == public_export_candidates.end()
            ? std::unordered_map<std::string, std::string>::const_iterator{}
            : target_it->second.find(exported);
        if (target_it == public_export_candidates.end() || candidate_it == target_it->second.end()) {
          throw std::runtime_error("VENOM-E2220 protected module does not export " + exported + ": " + target);
        }
        replacement << "const " << local
                    << "=(...__venomArgs)=>globalThis.__venomInvokeProtectedById(\""
                    << json_escape_plan(candidate_it->second)
                    << "\",__venomArgs);\n";
      }
      import_edits.emplace_back(imported.begin, imported.end, replacement.str());
      result.lowered_browser_imports.emplace_back(normalize_slashes(importer_chunk.source), imported.specifier);
    }
    if (!import_edits.empty()) importer_chunk.code = apply_source_edits(importer_chunk.code, std::move(import_edits));
  }

  result.registry_source = registry.str();
  detail::RegistrySourceChunk module_chunk;
  module_chunk.id = "modules";
  module_chunk.source = result.registry_source;
  for (const auto& record : result.records) if (record.status == "extracted") module_chunk.candidates.push_back(record.candidate);
  if (!module_chunk.candidates.empty()) result.registry_chunks.push_back(std::move(module_chunk));
  result.typescript = dts.str();
  return result;
}

std::string trim_copy(std::string value);

std::string source_line_text(const std::string& source, std::size_t line) {
  if (line == 0) return {};
  std::size_t begin = 0;
  for (std::size_t current = 1; current < line; ++current) {
    begin = source.find('\n', begin);
    if (begin == std::string::npos) return {};
    ++begin;
  }
  const auto end = source.find('\n', begin);
  return source.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}


struct ExtractedProtectedFunction {
  std::size_t begin = 0;
  std::size_t end = 0;
  std::string declaration;
  std::string callable_expression;
  std::string params;
  std::string syntax_kind;
  bool exported = false;
};

bool extract_protected_function(const JsChunk& chunk,
                                const detail::FunctionExtractionRecord& record,
                                ExtractedProtectedFunction& out) {
  out = {};
  const auto unit = frontend::lower_protected_unit(
      chunk.code, chunk.source, record.name, (chunk.flags & JsChunkModule) != 0u);
  if (!unit.found) return false;
  if (unit.generator) {
    raise_diagnostic("VENOM-E2303", "Generator protected functions are not supported",
                     {chunk.source, unit.line, unit.column + 1u, source_line_text(chunk.code, unit.line)},
                     "The protected unit uses generator semantics.",
                     "Convert the generator to a normal or async function before protecting it.",
                     "docs/reference/diagnostics.md#venom-e2303");
  }
  out.begin = unit.begin;
  out.end = unit.end;
  out.declaration = unit.declaration;
  out.callable_expression = unit.callable_expression;
  out.params = unit.parameters;
  out.syntax_kind = unit.syntax_kind;
  out.exported = unit.exported;
  return true;
}

bool all_external_calls_are_awaited(const std::string& code,
                                    const std::string& name,
                                    std::size_t declaration_begin,
                                    std::size_t declaration_end,
                                    std::string& reason) {
  const std::regex call("(^|[^A-Za-z0-9_$\\.])" + name + R"(\s*\()" );
  auto begin = std::sregex_iterator(code.begin(), code.end(), call);
  const auto finish = std::sregex_iterator();
  for (auto it = begin; it != finish; ++it) {
    const auto offset = static_cast<std::size_t>(it->position(0));
    if (offset >= declaration_begin && offset < declaration_end) continue;
    const auto prefix_begin = offset > 32u ? offset - 32u : 0u;
    const auto prefix = code.substr(prefix_begin, offset - prefix_begin + static_cast<std::size_t>((*it)[1].length()));
    if (!std::regex_search(prefix, std::regex(R"(await\s*$)"))) {
      reason = "call site is not explicitly awaited";
      return false;
    }
  }
  return true;
}

std::string trim_copy(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1u);
}

BridgeRewriteResult apply_bridge_rewrites(std::vector<JsChunk>& chunks,
                                          const std::vector<FunctionExtractionRecord>& extraction_records,
                                          const std::string& bridge_id_salt) {
  BridgeRewriteResult result;
  std::ostringstream registry;
  std::ostringstream dts;
  // Each independently executable function chunk needs the value codec bootstrap.
  registry << protected_registry_bootstrap_js(false);
  bool any = false;
  for (const auto& record : extraction_records) {
    if (record.disposition != "bridge-candidate") continue;
    const auto it = std::find_if(chunks.begin(), chunks.end(), [&](const JsChunk& chunk) {
      return chunk.route == record.route && chunk.source == record.source;
    });
    BridgeRewriteRecord rewrite;
    rewrite.source = record.source;
    rewrite.function = record.name;
    rewrite.candidate = opaque_bridge_id(record.source, record.name, bridge_id_salt);
    rewrite.status = "retained";
    if (it == chunks.end()) { rewrite.reason = "source chunk not found"; result.records.push_back(std::move(rewrite)); continue; }
    ExtractedProtectedFunction extracted;
    if (!extract_protected_function(*it, record, extracted)) {
      rewrite.reason = "supported protected forms are named function declarations and variable-bound arrow functions";
      result.records.push_back(std::move(rewrite));
      continue;
    }
    const auto input_contract = contract_annotation_before(it->code, extracted.begin, "input", it->source);
    const auto output_contract = contract_annotation_before(it->code, extracted.begin, "output", it->source);
    rewrite.input_contract_json = contract_fields_json(input_contract);
    rewrite.output_contract_json = contract_fields_json(output_contract);
    rewrite.input_typescript = typescript_object_type(input_contract);
    rewrite.output_typescript = typescript_object_type(output_contract);
    auto dependency_declaration = extracted.declaration;
    try { dependency_declaration = std::regex_replace(dependency_declaration, std::regex(R"(^\s*export\s+(?:default\s+)?)"), ""); }
    catch (const std::regex_error& error) { throw std::runtime_error(std::string("dependency declaration regex: ") + error.what()); }
    auto dependency_resolution = resolve_liftable_function_dependencies(
        it->code, dependency_declaration, record.name, extracted.params, it->source,
        (it->flags & JsChunkModule) != 0u);
    // Large explicitly isolated compilation units may contain deeply nested
    // scopes that the lightweight analyzer cannot fully resolve. They are
    // accepted only after a separate runtime-safety verification rejects browser
    // globals and unsupported dynamic semantics.
    if (!dependency_resolution.success && record.isolated) {
      std::string isolated_reason;
      if (verify_isolated_protected_unit(dependency_declaration, isolated_reason)) {
        dependency_resolution.success = true;
        dependency_resolution.reason = isolated_reason;
        dependency_resolution.dependencies.clear();
      } else {
        dependency_resolution.reason = "isolated verification failed: " + isolated_reason;
      }
    }
    if (!dependency_resolution.success) {
      rewrite.reason = dependency_resolution.reason;
      result.records.push_back(std::move(rewrite));
      continue;
    }
    for (const auto& dependency : dependency_resolution.dependencies)
      rewrite.lifted_dependencies.push_back(dependency.name);
    std::string call_reason;
    bool calls_awaited = false;
    try { calls_awaited = all_external_calls_are_awaited(it->code, record.name, extracted.begin, extracted.end, call_reason); }
    catch (const std::regex_error& error) { throw std::runtime_error(std::string("call-site regex: ") + error.what()); }
    if (!calls_awaited) {
      rewrite.reason = call_reason;
      result.records.push_back(std::move(rewrite));
      continue;
    }
    const auto candidate = rewrite.candidate;
    std::ostringstream stub;
    if (extracted.exported) stub << "export ";
    stub << "async function " << record.name << "(" << extracted.params << "){return globalThis.__venomInvokeProtectedById(\""
         << json_escape_plan(candidate) << "\",Array.from(arguments));}";
    it->code.replace(extracted.begin, extracted.end - extracted.begin, stub.str());
    if (it->code.find(extracted.declaration) != std::string::npos) {
      throw std::runtime_error("protected function source remained in browser chunk after extraction: " + record.source + "::" + record.name);
    }
    auto registry_declaration = trim_copy(extracted.callable_expression);
    try { registry_declaration = std::regex_replace(registry_declaration, std::regex(R"(^\s*export\s+(?:default\s+)?)"), ""); }
    catch (const std::regex_error& error) { throw std::runtime_error(std::string("registry declaration regex: ") + error.what()); }
    registry << "globalThis.__venomProtectedBridge[\"" << json_escape_plan(candidate) << "\"]=(function(){\n";
    for (const auto& dependency : dependency_resolution.dependencies)
      registry << dependency.declaration << "\n";
    registry << "const __venomFn=(" << registry_declaration << ");\n"
             << "return function(){\nconst __a=Array.from(arguments,globalThis.__venomBridgeRevive);\n"
             << contract_validator_js(input_contract, "__a[0]", "input")
             << "const __r=__venomFn.apply(this,__a);\n";
    if (output_contract.empty()) {
      registry << "return globalThis.__venomBridgeEncode(__r);\n";
    } else {
      registry << "if(__r&&typeof __r.then==='function')return __r.then(function(__o){\n"
               << contract_validator_js(output_contract, "__o", "output")
               << "return globalThis.__venomBridgeEncode(__o);});\n"
               << contract_validator_js(output_contract, "__r", "output")
               << "return globalThis.__venomBridgeEncode(__r);\n";
    }
    registry << "};\n})();\n";
    dts << "export function " << record.name << "(input: " << rewrite.input_typescript
        << ", options?: VenomCallOptions): Promise<" << rewrite.output_typescript << ">;\n";
    rewrite.status = "extracted";
    rewrite.reason = dependency_resolution.dependencies.empty()
        ? extracted.syntax_kind + " extracted; all external call sites are explicitly awaited"
        : extracted.syntax_kind + " extracted with pure lexical dependencies; all external call sites are explicitly awaited";
    result.records.push_back(std::move(rewrite));
    any = true;
  }
  if (any) {
    result.registry_source = registry.str();
    detail::RegistrySourceChunk function_chunk;
    function_chunk.id = "functions";
    function_chunk.source = result.registry_source;
    for (const auto& record : result.records) if (record.status == "extracted") function_chunk.candidates.push_back(record.candidate);
    if (!function_chunk.candidates.empty()) result.registry_chunks.push_back(std::move(function_chunk));
  }
  result.typescript = dts.str();
  return result;
}

std::string make_bridge_rewrite_report_json(const std::vector<BridgeRewriteRecord>& records) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 2,\n  \"policy\": \"awaited-calls-plus-pure-dependency-lifting\",\n  \"functions\": [\n";
  for (std::size_t i = 0; i < records.size(); ++i) {
    const auto& r = records[i];
    out << "    {\"source\":\"" << json_escape_plan(r.source) << "\",\"function\":\""
        << json_escape_plan(r.function) << "\",\"candidate\":\"" << json_escape_plan(r.candidate)
        << "\",\"status\":\"" << r.status << "\",\"reason\":\"" << json_escape_plan(r.reason) << "\",\"lifted_dependencies\":[";
    for (std::size_t j = 0; j < r.lifted_dependencies.size(); ++j) {
      if (j) out << ',';
      out << '\"' << json_escape_plan(r.lifted_dependencies[j]) << '\"';
    }
    out << "]}" << (i + 1u == records.size() ? "" : ",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

std::string make_protected_contracts_json(const std::vector<BridgeRewriteRecord>& records) {
  std::ostringstream out;
  out << "{\n  \"schema\":\"venom-protected-contracts\",\n"
      << "  \"version\":1,\n  \"exports\":[";
  bool first = true;
  for (const auto& record : records) {
    if (record.status != "extracted") continue;
    if (!first) out << ',';
    first = false;
    out << "\n    {\"name\":\"" << json_escape_plan(record.function)
        << "\",\"source\":\"" << json_escape_plan(record.source)
        << "\",\"candidate\":\"" << json_escape_plan(record.candidate)
        << "\",\"input\":" << (record.input_contract_json.empty() ? "[]" : record.input_contract_json)
        << ",\"output\":" << (record.output_contract_json.empty() ? "[]" : record.output_contract_json)
        << '}';
  }
  if (!first) out << '\n';
  out << "  ]\n}\n";
  return out.str();
}


}  // namespace venom::compiler::detail
