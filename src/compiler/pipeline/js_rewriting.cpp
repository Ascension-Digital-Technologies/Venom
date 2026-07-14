#include "compiler/pipeline/js_rewriting.hpp"
#include "compiler/pipeline/function_dependencies.hpp"

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
  for (const unsigned char c : value) {
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


bool is_js_ident_start(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalpha(u) != 0 || c == '_' || c == '$';
}

bool is_js_ident_continue(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) != 0 || c == '_' || c == '$';
}

std::string json_escape_plan(const std::string& value) {
  std::ostringstream out;
  for (const unsigned char c : value) {
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

struct JsLexToken {
  enum class Kind { Identifier, String, Punct, End } kind = Kind::End;
  std::string text;
  std::size_t begin = 0;
  std::size_t end = 0;
};

class JsModuleLexer {
 public:
  explicit JsModuleLexer(const std::string& source) : source_(source) {}

  JsLexToken next() {
    skip_trivia();
    if (cursor_ >= source_.size()) return {JsLexToken::Kind::End, {}, cursor_, cursor_};
    const auto begin = cursor_;
    const char c = source_[cursor_];
    if (is_js_ident_start(c)) {
      ++cursor_;
      while (cursor_ < source_.size() && is_js_ident_continue(source_[cursor_])) ++cursor_;
      return {JsLexToken::Kind::Identifier, source_.substr(begin, cursor_ - begin), begin, cursor_};
    }
    if (c == '\'' || c == '"') {
      const char quote = c;
      ++cursor_;
      std::string value;
      bool escape = false;
      while (cursor_ < source_.size()) {
        const char ch = source_[cursor_++];
        if (escape) {
          switch (ch) {
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: value.push_back(ch); break;
          }
          escape = false;
          continue;
        }
        if (ch == '\\') { escape = true; continue; }
        if (ch == quote) break;
        value.push_back(ch);
      }
      return {JsLexToken::Kind::String, value, begin, cursor_};
    }
    if (c == '`') {
      ++cursor_;
      bool escape = false;
      while (cursor_ < source_.size()) {
        const char ch = source_[cursor_++];
        if (escape) { escape = false; continue; }
        if (ch == '\\') { escape = true; continue; }
        if (ch == '`') break;
      }
      return {JsLexToken::Kind::String, source_.substr(begin, cursor_ - begin), begin, cursor_};
    }
    ++cursor_;
    return {JsLexToken::Kind::Punct, std::string(1, c), begin, cursor_};
  }

 private:
  void skip_trivia() {
    while (cursor_ < source_.size()) {
      if (std::isspace(static_cast<unsigned char>(source_[cursor_])) != 0) { ++cursor_; continue; }
      if (cursor_ + 1 < source_.size() && source_[cursor_] == '/' && source_[cursor_ + 1] == '/') {
        cursor_ += 2;
        while (cursor_ < source_.size() && source_[cursor_] != '\n') ++cursor_;
        continue;
      }
      if (cursor_ + 1 < source_.size() && source_[cursor_] == '/' && source_[cursor_ + 1] == '*') {
        const auto end = source_.find("*/", cursor_ + 2);
        cursor_ = end == std::string::npos ? source_.size() : end + 2;
        continue;
      }
      break;
    }
  }

  const std::string& source_;
  std::size_t cursor_ = 0;
};

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
      throw std::runtime_error("VENOM-E2301 malformed " + direction + " contract in " + source + ": " + item);
    auto name = trim_ascii(item.substr(0, colon));
    auto type = trim_ascii(item.substr(colon + 1));
    bool optional = false;
    if (!name.empty() && name.back() == '?') { optional = true; name.pop_back(); }
    static const std::regex ident(R"(^[A-Za-z_$][A-Za-z0-9_$]*$)");
    static const std::unordered_set<std::string> valid = {
      "string", "number", "integer", "boolean", "object", "array", "any",
      "string[]", "number[]", "integer[]", "boolean[]"
    };
    if (!std::regex_match(name, ident) || valid.find(type) == valid.end())
      throw std::runtime_error("VENOM-E2302 unsupported " + direction + " contract field in " + source + ": " + item);
    if (!names.insert(name).second)
      throw std::runtime_error("VENOM-E2303 duplicate " + direction + " contract field in " + source + ": " + name);
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

ProtectedModuleSyntax parse_protected_module_syntax(const JsChunk& chunk) {
  {
    JsModuleLexer dynamic_scan(chunk.code);
    while (true) {
      const auto token = dynamic_scan.next();
      if (token.kind == JsLexToken::Kind::End) break;
      if (token.kind != JsLexToken::Kind::Identifier || token.text != "import") continue;
      const auto next = dynamic_scan.next();
      if (next.kind == JsLexToken::Kind::Punct && next.text == "(")
        throw std::runtime_error("VENOM-E2205 dynamic import is not supported in protected module: " + chunk.source);
    }
  }
  ProtectedModuleSyntax syntax;
  JsModuleLexer lexer(chunk.code);
  int brace_depth = 0;
  while (true) {
    const auto token = lexer.next();
    if (token.kind == JsLexToken::Kind::End) break;
    if (token.kind == JsLexToken::Kind::Punct) {
      if (token.text == "{") ++brace_depth;
      else if (token.text == "}") --brace_depth;
      continue;
    }
    if (brace_depth != 0 || token.kind != JsLexToken::Kind::Identifier) continue;

    if (token.text == "import") {
      const auto next = lexer.next();
      if (next.kind == JsLexToken::Kind::Punct && next.text == "(") {
        throw std::runtime_error("VENOM-E2205 dynamic import is not supported in protected module: " + chunk.source);
      }
      std::size_t statement_end = next.end;
      std::string clause;
      std::string specifier;
      if (next.kind == JsLexToken::Kind::String) {
        throw std::runtime_error("VENOM-E2206 side-effect-only imports are not supported in protected module: " + chunk.source);
      }
      clause = chunk.code.substr(next.begin, next.end - next.begin);
      bool saw_from = false;
      int nested = (next.kind == JsLexToken::Kind::Punct && (next.text == "{" || next.text == "(" || next.text == "[")) ? 1 : 0;
      while (true) {
        const auto part = lexer.next();
        if (part.kind == JsLexToken::Kind::End) break;
        statement_end = part.end;
        if (part.kind == JsLexToken::Kind::Punct) {
          if (part.text == "{" || part.text == "(" || part.text == "[") ++nested;
          else if (part.text == "}" || part.text == ")" || part.text == "]") --nested;
          else if (part.text == ";" && nested == 0) break;
        }
        if (!saw_from) {
          if (part.kind == JsLexToken::Kind::Identifier && part.text == "from" && nested == 0) {
            saw_from = true;
            clause = trim_ascii(chunk.code.substr(next.begin, part.begin - next.begin));
          }
        } else if (part.kind == JsLexToken::Kind::String) {
          specifier = part.text;
        }
      }
      if (!saw_from || specifier.empty())
        throw std::runtime_error("VENOM-E2207 malformed protected module import in " + chunk.source);
      syntax.imports.push_back({specifier, clause, token.begin, statement_end});
      continue;
    }

    if (token.text == "export") {
      const auto first = lexer.next();
      bool is_async = false;
      auto current = first;
      if (current.kind == JsLexToken::Kind::Identifier && current.text == "async") {
        is_async = true;
        current = lexer.next();
      }
      if (current.kind == JsLexToken::Kind::Identifier && current.text == "function") {
        const auto name = lexer.next();
        if (name.kind != JsLexToken::Kind::Identifier)
          throw std::runtime_error("VENOM-E2208 protected module export function requires a name: " + chunk.source);
        const auto open = lexer.next();
        if (open.kind != JsLexToken::Kind::Punct || open.text != "(")
          throw std::runtime_error("VENOM-E2209 malformed protected module export function: " + chunk.source);
        int depth = 1;
        std::size_t close_begin = open.end;
        while (depth > 0) {
          const auto part = lexer.next();
          if (part.kind == JsLexToken::Kind::End)
            throw std::runtime_error("VENOM-E2210 unterminated protected export parameters: " + chunk.source);
          if (part.kind == JsLexToken::Kind::Punct && part.text == "(") ++depth;
          else if (part.kind == JsLexToken::Kind::Punct && part.text == ")") {
            --depth;
            if (depth == 0) close_begin = part.begin;
          }
        }
        ProtectedModuleExport exported;
        exported.name = name.text;
        exported.params = chunk.code.substr(open.end, close_begin - open.end);
        exported.async = is_async;
        exported.input_contract = contract_annotation_before(chunk.code, token.begin, "input", chunk.source);
        exported.output_contract = contract_annotation_before(chunk.code, token.begin, "output", chunk.source);
        exported.export_begin = token.begin;
        exported.export_end = current.begin;
        syntax.exports.push_back(std::move(exported));
      } else {
        syntax.unsupported_exports.push_back(current.text.empty() ? "unknown" : current.text);
      }
    }
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
  if (resolved.size() < 3 || resolved.substr(resolved.size() - 3) != ".js") {
    const auto with_js = resolved + ".js";
    if (by_source.find(with_js) != by_source.end()) return with_js;
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
  for (std::size_t i = 0; i < chunks.size(); ++i) {
    auto& chunk = chunks[i];
    if (!has_file_scope_venom_protected_module_directive(chunk.code)) continue;
    if ((chunk.flags & JsChunkBrowser) != 0u)
      throw std::runtime_error("protected module cannot be browser-side: " + chunk.source);
    const auto source = normalize_slashes(chunk.source);
    by_source.emplace(source, modules.size());
    modules.push_back({i, source, protected_module_id(source, bridge_id_salt), parse_protected_module_syntax(chunk), {}});
  }
  if (modules.empty()) return result;

  for (auto& module : modules) {
    if (!module.syntax.unsupported_exports.empty())
      throw std::runtime_error("VENOM-E2202 protected modules currently export named function declarations only: " + module.source);
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
  dts << "// Generated by Venom. Do not edit.\n"
      << "export interface VenomCallOptions { timeout?: number; signal?: AbortSignal; }\n";
  registry << "globalThis.__venomProtectedBridge=globalThis.__venomProtectedBridge||Object.create(null);\n"
           << "globalThis.__venomProtectedModules=globalThis.__venomProtectedModules||Object.create(null);\n";

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
      const auto wrapped = "__venomContract_" + entry.name;
      registry << "const " << wrapped << "=function(){\n"
               << contract_validator_js(entry.input_contract, "arguments[0]", "input")
               << "const __r=" << entry.name << ".apply(this,arguments);\n";
      if (entry.output_contract.empty()) {
        registry << "return __r;\n";
      } else {
        registry << "if(__r&&typeof __r.then==='function')return __r.then(function(__o){\n"
                 << contract_validator_js(entry.output_contract, "__o", "output")
                 << "return __o;});\n"
                 << contract_validator_js(entry.output_contract, "__r", "output")
                 << "return __r;\n";
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
      result.records.push_back({module.source, entry.name, candidate, "extracted",
                                "AST-scanned protected module graph export", {}});
    }
    if (!public_entry_module) facade << "export {};\n";
    registry << "})();\n";
    chunk.code = facade.str();
    chunk.flags |= JsChunkBrowser | JsChunkModule;
    chunk.flags &= ~JsChunkBytecodeEncoded;
    chunk.execution_reason = "generated browser facade for isolated protected module graph";
    chunk.execution_confidence = 100u;
  }
  result.registry_source = registry.str();
  result.typescript = dts.str();
  return result;
}

std::size_t line_start_offset(const std::string& source, std::uint32_t line) {
  if (line <= 1u) return 0u;
  std::size_t offset = 0;
  for (std::uint32_t current = 1; current < line; ++current) {
    const auto next = source.find('\n', offset);
    if (next == std::string::npos) return std::string::npos;
    offset = next + 1u;
  }
  return offset;
}

std::string trim_copy(std::string value);

struct ExtractedProtectedFunction {
  std::size_t begin = 0;
  std::size_t end = 0;
  std::string declaration;
  std::string callable_expression;
  std::string params;
  std::string syntax_kind;
  bool exported = false;
};

std::size_t skip_js_space(const std::string& source, std::size_t i) {
  while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
  return i;
}

std::size_t find_balanced_js_end(const std::string& source, std::size_t open, char left, char right) {
  int depth = 0;
  bool in_single = false, in_double = false, in_template = false;
  bool in_line_comment = false, in_block_comment = false, escape = false;
  for (std::size_t i = open; i < source.size(); ++i) {
    const char c = source[i];
    const char n = i + 1u < source.size() ? source[i + 1u] : '\0';
    if (in_line_comment) { if (c == '\n') in_line_comment = false; continue; }
    if (in_block_comment) { if (c == '*' && n == '/') { in_block_comment = false; ++i; } continue; }
    if (escape) { escape = false; continue; }
    if ((in_single || in_double || in_template) && c == '\\') { escape = true; continue; }
    if (!in_single && !in_double && !in_template && c == '/' && n == '/') { in_line_comment = true; ++i; continue; }
    if (!in_single && !in_double && !in_template && c == '/' && n == '*') { in_block_comment = true; ++i; continue; }
    if (!in_double && !in_template && c == '\'') { in_single = !in_single; continue; }
    if (!in_single && !in_template && c == '"') { in_double = !in_double; continue; }
    if (!in_single && !in_double && c == '`') { in_template = !in_template; continue; }
    if (in_single || in_double || in_template) continue;
    if (c == left) ++depth;
    else if (c == right && --depth == 0) return i + 1u;
  }
  return std::string::npos;
}

std::size_t find_arrow_expression_end(const std::string& source, std::size_t begin) {
  int paren = 0, bracket = 0, brace = 0;
  bool in_single = false, in_double = false, in_template = false, escape = false;
  for (std::size_t i = begin; i < source.size(); ++i) {
    const char c = source[i];
    if (escape) { escape = false; continue; }
    if ((in_single || in_double || in_template) && c == '\\') { escape = true; continue; }
    if (!in_double && !in_template && c == '\'') { in_single = !in_single; continue; }
    if (!in_single && !in_template && c == '"') { in_double = !in_double; continue; }
    if (!in_single && !in_double && c == '`') { in_template = !in_template; continue; }
    if (in_single || in_double || in_template) continue;
    if (c == '(') ++paren; else if (c == ')') --paren;
    else if (c == '[') ++bracket; else if (c == ']') --bracket;
    else if (c == '{') ++brace; else if (c == '}') --brace;
    else if ((c == ';' || c == '\n') && paren == 0 && bracket == 0 && brace == 0) return c == ';' ? i + 1u : i;
  }
  return source.size();
}

bool extract_protected_function(const JsChunk& chunk,
                                const detail::FunctionExtractionRecord& record,
                                ExtractedProtectedFunction& out) {
  out = {};
  out.begin = line_start_offset(chunk.code, record.line);
  if (out.begin == std::string::npos) return false;

  // Named declaration: export/default/async function name(...){...}. Parse the
  // parameter range structurally so nested defaults and destructuring are safe.
  const auto header_limit = std::min(chunk.code.size(), out.begin + 8192u);
  const auto first_line = chunk.code.substr(out.begin, header_limit - out.begin);
  const std::regex declaration_prefix(
      R"(^\s*(?:export\s+)?(?:default\s+)?(?:async\s+)?function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*)");
  std::smatch declaration_match;
  if (std::regex_search(first_line, declaration_match, declaration_prefix) && declaration_match.size() >= 2 &&
      declaration_match[1].str() == record.name) {
    std::size_t cursor = out.begin + static_cast<std::size_t>(declaration_match.position(0) + declaration_match.length(0));
    cursor = skip_js_space(chunk.code, cursor);
    if (cursor >= chunk.code.size() || chunk.code[cursor] != '(') return false;
    const auto params_end = find_balanced_js_end(chunk.code, cursor, '(', ')');
    if (params_end == std::string::npos) return false;
    out.params = chunk.code.substr(cursor + 1u, params_end - cursor - 2u);
    const auto brace = skip_js_space(chunk.code, params_end);
    if (brace >= chunk.code.size() || chunk.code[brace] != '{') return false;
    out.end = find_balanced_js_end(chunk.code, brace, '{', '}');
    if (out.end == std::string::npos) return false;
    out.declaration = chunk.code.substr(out.begin, out.end - out.begin);
    out.callable_expression = out.declaration;
    out.syntax_kind = "function-declaration";
    out.exported = std::regex_search(out.declaration, std::regex(R"(^\s*export\s+)"));
    return true;
  }

  // Variable-bound arrow: export? const|let|var name = async? params => body.
  // Parse the parameter range structurally so nested defaults and destructuring
  // do not terminate at the first ')' or '{'.
  const std::regex arrow_prefix(R"(^\s*(?:export\s+)?(?:const|let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*(async\s+)?)");
  std::smatch arrow_match;
  if (!std::regex_search(first_line, arrow_match, arrow_prefix) || arrow_match.size() < 3 || arrow_match[1].str() != record.name)
    return false;
  std::size_t cursor = out.begin + static_cast<std::size_t>(arrow_match.position(0) + arrow_match.length(0));
  cursor = skip_js_space(chunk.code, cursor);
  std::string param_text;
  if (cursor < chunk.code.size() && chunk.code[cursor] == '(') {
    const auto params_end = find_balanced_js_end(chunk.code, cursor, '(', ')');
    if (params_end == std::string::npos) return false;
    param_text = chunk.code.substr(cursor, params_end - cursor);
    cursor = params_end;
  } else {
    const auto start = cursor;
    if (start >= chunk.code.size() ||
        !(std::isalpha(static_cast<unsigned char>(chunk.code[start])) != 0 || chunk.code[start] == '_' || chunk.code[start] == '$'))
      return false;
    ++cursor;
    while (cursor < chunk.code.size() &&
           (std::isalnum(static_cast<unsigned char>(chunk.code[cursor])) != 0 || chunk.code[cursor] == '_' || chunk.code[cursor] == '$'))
      ++cursor;
    param_text = chunk.code.substr(start, cursor - start);
  }
  cursor = skip_js_space(chunk.code, cursor);
  if (cursor + 1u >= chunk.code.size() || chunk.code.compare(cursor, 2u, "=>") != 0) return false;
  const auto arrow = cursor;
  if (param_text.size() >= 2u && param_text.front() == '(' && param_text.back() == ')')
    out.params = param_text.substr(1u, param_text.size() - 2u);
  else
    out.params = param_text;
  const auto body = skip_js_space(chunk.code, arrow + 2u);
  if (body >= chunk.code.size()) return false;
  if (chunk.code[body] == '{') {
    out.end = find_balanced_js_end(chunk.code, body, '{', '}');
    if (out.end == std::string::npos) return false;
    auto semi = skip_js_space(chunk.code, out.end);
    if (semi < chunk.code.size() && chunk.code[semi] == ';') out.end = semi + 1u;
  } else {
    out.end = find_arrow_expression_end(chunk.code, body);
  }
  out.declaration = chunk.code.substr(out.begin, out.end - out.begin);
  const auto equals = out.declaration.find('=');
  if (equals == std::string::npos) return false;
  out.callable_expression = trim_copy(out.declaration.substr(equals + 1u));
  if (!out.callable_expression.empty() && out.callable_expression.back() == ';') out.callable_expression.pop_back();
  out.callable_expression = trim_copy(out.callable_expression);
  out.syntax_kind = arrow_match[2].matched ? "async-arrow-function" : "arrow-function";
  out.exported = std::regex_search(out.declaration, std::regex(R"(^\s*export\s+)"));
  return !out.callable_expression.empty();
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
  registry << "globalThis.__venomProtectedBridge=globalThis.__venomProtectedBridge||Object.create(null);\n";
  bool any = false;
  for (const auto& record : extraction_records) {
    if (record.disposition != "bridge-candidate") continue;
    const auto it = std::find_if(chunks.begin(), chunks.end(), [&](const JsChunk& chunk) {
      return chunk.route == record.route && chunk.source == record.source;
    });
    BridgeRewriteRecord rewrite{record.source, record.name, opaque_bridge_id(record.source, record.name, bridge_id_salt), "retained", "", {}};
    if (it == chunks.end()) { rewrite.reason = "source chunk not found"; result.records.push_back(std::move(rewrite)); continue; }
    ExtractedProtectedFunction extracted;
    if (!extract_protected_function(*it, record, extracted)) {
      rewrite.reason = "supported protected forms are named function declarations and variable-bound arrow functions";
      result.records.push_back(std::move(rewrite));
      continue;
    }
    auto dependency_declaration = extracted.declaration;
    try { dependency_declaration = std::regex_replace(dependency_declaration, std::regex(R"(^\s*export\s+(?:default\s+)?)"), ""); }
    catch (const std::regex_error& error) { throw std::runtime_error(std::string("dependency declaration regex: ") + error.what()); }
    auto dependency_resolution = resolve_liftable_function_dependencies(
        it->code, dependency_declaration, record.name, extracted.params);
    // Large explicitly isolated compilation units may contain deeply nested
    // scopes that the lightweight analyzer cannot fully resolve. They are
    // accepted only after a separate realm-safety verification rejects browser
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
    registry << "return (" << registry_declaration << ");\n})();\n";
    rewrite.status = "extracted";
    rewrite.reason = dependency_resolution.dependencies.empty()
        ? extracted.syntax_kind + " extracted; all external call sites are explicitly awaited"
        : extracted.syntax_kind + " extracted with pure lexical dependencies; all external call sites are explicitly awaited";
    result.records.push_back(std::move(rewrite));
    any = true;
  }
  if (any) result.registry_source = registry.str();
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




}  // namespace venom::compiler::detail
