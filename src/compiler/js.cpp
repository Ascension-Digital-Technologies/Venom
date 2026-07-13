#include "compiler/js.hpp"
#include "compiler/function_dependencies.hpp"
#include "compiler/wasm_runtime_blob.hpp"
#include "compiler/quickjs_runtime_wasm_blob.hpp"
#include "quickjs/bytecode.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>


namespace venom::compiler {


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

void append_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

std::string to_string(const std::vector<unsigned char>& bytes) {
  return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
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

bool is_name_char(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) != 0 || c == '-' || c == '_' || c == ':' || c == '.';
}


bool has_file_scope_venom_browser_directive(const std::string& source) {
  std::size_t i = 0;
  if (source.size() >= 3 && static_cast<unsigned char>(source[0]) == 0xefu && static_cast<unsigned char>(source[1]) == 0xbbu && static_cast<unsigned char>(source[2]) == 0xbfu) i = 3;
  while (i < source.size()) {
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
    if (i + 1 >= source.size()) break;
    if (source[i] == '/' && source[i + 1] == '/') {
      const auto end = source.find('\n', i + 2);
      const auto line = lower_ascii(source.substr(i + 2, (end == std::string::npos ? source.size() : end) - (i + 2)));
      if (line.find("@venom:") != std::string::npos && line.find("browser") != std::string::npos) return true;
      i = end == std::string::npos ? source.size() : end + 1; continue;
    }
    if (source[i] == '/' && source[i + 1] == '*') {
      const auto end = source.find("*/", i + 2); if (end == std::string::npos) break;
      const auto block = lower_ascii(source.substr(i + 2, end - (i + 2)));
      if (block.find("@venom:") != std::string::npos && block.find("browser") != std::string::npos) return true;
      i = end + 2; continue;
    }
    break;
  }
  return false;
}


bool has_file_scope_venom_protected_directive(const std::string& source) {
  std::size_t i = 0;
  if (source.size() >= 3 && static_cast<unsigned char>(source[0]) == 0xefu && static_cast<unsigned char>(source[1]) == 0xbbu && static_cast<unsigned char>(source[2]) == 0xbfu) i = 3;
  while (i < source.size()) {
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
    if (i + 1 >= source.size()) break;
    if (source[i] == '/' && source[i + 1] == '/') {
      const auto end = source.find('\n', i + 2);
      const auto line = lower_ascii(source.substr(i + 2, (end == std::string::npos ? source.size() : end) - (i + 2)));
      if (line.find("@venom:") != std::string::npos && line.find("protected") != std::string::npos) return true;
      i = end == std::string::npos ? source.size() : end + 1; continue;
    }
    if (source[i] == '/' && source[i + 1] == '*') {
      const auto end = source.find("*/", i + 2); if (end == std::string::npos) break;
      const auto block = lower_ascii(source.substr(i + 2, end - (i + 2)));
      if (block.find("@venom:") != std::string::npos && block.find("protected") != std::string::npos) return true;
      i = end + 2; continue;
    }
    break;
  }
  return false;
}


std::string json_escape_plan(const std::string& value);

struct FunctionRealmRecord {
  std::string route;
  std::string source;
  std::string name;
  std::string execution;
  std::string reason;
  std::uint32_t line = 0;
  bool promoted_whole_file = false;
  bool isolated = false;
};

std::vector<FunctionRealmRecord> scan_function_realm_directives(const JsChunk& chunk) {
  std::vector<FunctionRealmRecord> records;
  const std::regex declaration(
      R"(^\s*(?:(?:export\s+)?(?:default\s+)?)?(?:async\s+)?function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\(|^\s*(?:const|let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=.*=>)");
  std::istringstream input(chunk.code);
  std::string line;
  std::string pending_realm;
  std::string pending_reason;
  bool pending_isolated = false;
  std::uint32_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    const auto lower = lower_ascii(line);
    const auto directive = lower.find("@venom:");
    if (directive != std::string::npos) {
      const bool browser = lower.find("browser", directive) != std::string::npos;
      const bool protected_realm = lower.find("protected", directive) != std::string::npos;
      if (browser && protected_realm) {
        throw std::runtime_error("conflicting function-level Venom directive in " + chunk.source + ":" + std::to_string(line_number));
      }
      if (browser || protected_realm) {
        pending_realm = browser ? "browser" : "protected";
        pending_reason = browser ? "explicit function-level browser directive" : "explicit function-level protected directive";
        pending_isolated = protected_realm && lower.find("isolated", directive) != std::string::npos;
      }
      continue;
    }
    if (pending_realm.empty()) continue;
    const auto trimmed = lower_ascii(line);
    if (trimmed.find_first_not_of(" \t\r") == std::string::npos || trimmed.rfind("//", 0) == 0 || trimmed.rfind("/*", 0) == 0 || trimmed.rfind("*", 0) == 0) continue;
    std::smatch match;
    if (!std::regex_search(line, match, declaration)) {
      pending_realm.clear();
      pending_reason.clear();
      pending_isolated = false;
      continue;
    }
    std::string name;
    if (match.size() > 1 && match[1].matched) name = match[1].str();
    else if (match.size() > 2 && match[2].matched) name = match[2].str();
    if (!name.empty()) records.push_back({chunk.route, chunk.source, name, pending_realm, pending_reason, line_number, false, pending_isolated});
    pending_realm.clear();
    pending_reason.clear();
    pending_isolated = false;
  }
  return records;
}

std::vector<FunctionRealmRecord> apply_function_realm_planning(std::vector<JsChunk>& chunks) {
  std::vector<FunctionRealmRecord> records;
  for (auto& chunk : chunks) {
    auto local = scan_function_realm_directives(chunk);
    bool has_browser_function = false;
    std::string browser_function;
    for (const auto& record : local) {
      if (record.execution == "browser") {
        has_browser_function = true;
        browser_function = record.name;
        break;
      }
    }
    if (has_browser_function && (chunk.flags & JsChunkBrowser) == 0u) {
      if (has_file_scope_venom_protected_directive(chunk.code)) {
        throw std::runtime_error("function-level browser directive conflicts with file-scope protected directive in " + chunk.source);
      }
      // v1 foundation: preserve semantics by promoting the containing classic file.
      // A later extraction pass can replace this conservative promotion with generated RPC stubs.
      chunk.flags |= JsChunkBrowser;
      chunk.execution_reason = "function-level browser directive on " + browser_function + "; whole-file promotion preserves shared lexical state";
      chunk.execution_confidence = 100u;
      for (auto& record : local) record.promoted_whole_file = true;
    }
    records.insert(records.end(), local.begin(), local.end());
  }
  return records;
}

std::string make_function_plan_text(const std::vector<FunctionRealmRecord>& records) {
  std::ostringstream out;
  out << "VENOM_FUNCTION_PLAN_V1\n";
  out << "route\tsource\tline\tfunction\texecution\tpromoted_whole_file\treason\n";
  for (const auto& record : records) {
    out << record.route << '\t' << record.source << '\t' << record.line << '\t' << record.name << '\t'
        << record.execution << '\t' << (record.promoted_whole_file ? "true" : "false") << '\t' << record.reason << '\n';
  }
  return out.str();
}

std::string make_function_plan_json(const std::vector<FunctionRealmRecord>& records) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 1,\n  \"mode\": \"conservative-whole-file-promotion\",\n  \"functions\": [\n";
  for (std::size_t i = 0; i < records.size(); ++i) {
    const auto& record = records[i];
    out << "    {\"route\":\"" << json_escape_plan(record.route)
        << "\",\"source\":\"" << json_escape_plan(record.source)
        << "\",\"line\":" << record.line
        << ",\"function\":\"" << json_escape_plan(record.name)
        << "\",\"execution\":\"" << record.execution
        << "\",\"promoted_whole_file\":" << (record.promoted_whole_file ? "true" : "false")
        << ",\"reason\":\"" << json_escape_plan(record.reason) << "\"}";
    if (i + 1 != records.size()) out << ',';
    out << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

struct FunctionExtractionRecord {
  std::string route;
  std::string source;
  std::string name;
  std::string requested_realm;
  std::string disposition;
  std::string bridge_mode;
  std::string reason;
  std::vector<std::string> blockers;
  std::uint32_t line = 0;
  bool isolated = false;
};

bool contains_token_ci(const std::string& source, const std::string& token) {
  return lower_ascii(source).find(lower_ascii(token)) != std::string::npos;
}

std::string function_window(const JsChunk& chunk, std::uint32_t line_number) {
  std::istringstream input(chunk.code);
  std::string line;
  std::vector<std::string> lines;
  while (std::getline(input, line)) lines.push_back(line);
  if (line_number == 0 || line_number > lines.size()) return {};
  const std::size_t begin = line_number - 1u;
  std::ostringstream out;
  int depth = 0;
  bool opened = false;
  bool in_single = false, in_double = false, in_template = false, escape = false;
  for (std::size_t i = begin; i < lines.size(); ++i) {
    out << lines[i] << '\n';
    for (char c : lines[i]) {
      if (escape) { escape = false; continue; }
      if ((in_single || in_double || in_template) && c == '\\') { escape = true; continue; }
      if (!in_double && !in_template && c == '\'') { in_single = !in_single; continue; }
      if (!in_single && !in_template && c == '"') { in_double = !in_double; continue; }
      if (!in_single && !in_double && c == '`') { in_template = !in_template; continue; }
      if (in_single || in_double || in_template) continue;
      if (c == '{') { ++depth; opened = true; }
      else if (c == '}') --depth;
    }
    if (opened && depth <= 0) break;
    if (!opened && lines[i].find("=>") != std::string::npos && lines[i].find(';') != std::string::npos) break;
  }
  return out.str();
}

std::vector<FunctionExtractionRecord> analyze_function_extraction(const std::vector<JsChunk>& chunks,
                                                                   const std::vector<FunctionRealmRecord>& functions) {
  std::vector<FunctionExtractionRecord> out;
  for (const auto& fn : functions) {
    const auto it = std::find_if(chunks.begin(), chunks.end(), [&](const JsChunk& c) {
      return c.route == fn.route && c.source == fn.source;
    });
    if (it == chunks.end()) continue;
    const bool file_browser = (it->flags & JsChunkBrowser) != 0u;
    const auto body = function_window(*it, fn.line);
    FunctionExtractionRecord rec{fn.route, fn.source, fn.name, fn.execution, "retained", "none", "", {}, fn.line, fn.isolated};
    if (fn.execution == "browser" && !file_browser) {
      rec.disposition = "whole-file-promotion";
      rec.reason = "browser function requires real DOM semantics; synchronous protected-to-browser invocation is not yet safely representable";
      rec.blockers.push_back("synchronous cross-realm result bridge unavailable");
    } else if (fn.execution == "protected" && file_browser) {
      // QuickJS supports ordinary JavaScript semantics such as `this`,
      // `arguments`, nested functions, and `with`. Only syntax that requires the
      // browser realm or cannot be serialized across the JSON bridge is rejected.
      // Use member-access tokens to avoid false positives in comments such as
      // "documentation".
      static const std::vector<std::pair<std::string, std::string>> unsafe{
        {"document.", "DOM access"}, {"window.", "browser global access"},
        {"globalthis.document", "DOM access"}, {"globalthis.window", "browser global access"},
        {"eval(", "dynamic eval"}, {"super", "super binding"},
        {"new.target", "new.target binding"}, {"yield", "generator semantics"},
        {"import.meta", "module metadata"}
      };
      for (const auto& item : unsafe) if (contains_token_ci(body, item.first)) rec.blockers.push_back(item.second);
      if (rec.blockers.empty()) {
        rec.disposition = "bridge-candidate";
        rec.bridge_mode = "async-json-v1";
        rec.reason = "function has no immediately unsafe realm-bound syntax; eligible for generated asynchronous JSON-value bridge";
      } else {
        rec.disposition = "retained-browser";
        rec.reason = "function cannot be extracted safely";
      }
    } else {
      rec.reason = file_browser ? "function already resides in browser realm" : "function already resides in protected realm";
    }
    out.push_back(std::move(rec));
  }
  return out;
}

std::string make_extraction_plan_text(const std::vector<FunctionExtractionRecord>& records) {
  std::ostringstream out;
  out << "VENOM_FUNCTION_EXTRACTION_PLAN_V1\n";
  out << "route\tsource\tline\tfunction\trequested_realm\tdisposition\tbridge_mode\treason\tblockers\n";
  for (const auto& r : records) {
    out << r.route << '\t' << r.source << '\t' << r.line << '\t' << r.name << '\t' << r.requested_realm
        << '\t' << r.disposition << '\t' << r.bridge_mode << '\t' << r.reason << '\t';
    for (std::size_t i=0;i<r.blockers.size();++i) { if (i) out << ", "; out << r.blockers[i]; }
    out << '\n';
  }
  return out.str();
}

std::string make_extraction_plan_json(const std::vector<FunctionExtractionRecord>& records) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 1,\n  \"policy\": \"fail-closed-analysis\",\n  \"functions\": [\n";
  for (std::size_t i=0;i<records.size();++i) {
    const auto& r=records[i];
    out << "    {\"route\":\"" << json_escape_plan(r.route) << "\",\"source\":\"" << json_escape_plan(r.source)
        << "\",\"line\":" << r.line << ",\"function\":\"" << json_escape_plan(r.name)
        << "\",\"requested_realm\":\"" << r.requested_realm << "\",\"disposition\":\"" << r.disposition
        << "\",\"bridge_mode\":\"" << r.bridge_mode << "\",\"reason\":\"" << json_escape_plan(r.reason) << "\",\"blockers\":[";
    for (std::size_t j=0;j<r.blockers.size();++j) { if (j) out << ','; out << '\"' << json_escape_plan(r.blockers[j]) << '\"'; }
    out << "]}" << (i+1==records.size()?"":",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

std::string make_realm_bridge_contract_json(const std::vector<FunctionExtractionRecord>& records) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 1,\n  \"transport\": \"worker-message\",\n  \"value_contract\": \"json-value-v1\",\n  \"synchronous_calls\": false,\n  \"candidates\": [\n";
  bool first=true;
  for (const auto& r:records) if (r.disposition=="bridge-candidate") {
    if (!first) out << ",\n";
    first = false;
    out << "    {\"id\":\"" << json_escape_plan(r.source + "::" + r.name) << "\",\"function\":\""
        << json_escape_plan(r.name) << "\",\"direction\":\"browser-to-protected\",\"mode\":\"async\",\"arguments\":\"json-array\",\"result\":\"json-value\"}";
  }
  if (!first) out << '\n';
  out << "  ]\n}\n";
  return out.str();
}

struct BridgeRewriteRecord {
  std::string source;
  std::string function;
  std::string candidate;
  std::string status;
  std::string reason;
  std::vector<std::string> lifted_dependencies;
};

struct BridgeRewriteResult {
  std::string registry_source;
  std::vector<BridgeRewriteRecord> records;
};

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

bool extract_function_declaration(const JsChunk& chunk,
                                  const FunctionExtractionRecord& record,
                                  std::size_t& begin,
                                  std::size_t& end,
                                  std::string& declaration,
                                  std::string& params) {
  begin = line_start_offset(chunk.code, record.line);
  if (begin == std::string::npos) return false;
  const auto line_end = chunk.code.find('\n', begin);
  const auto first_line = chunk.code.substr(begin, (line_end == std::string::npos ? chunk.code.size() : line_end) - begin);
  const std::regex header(R"(^\s*(?:export\s+)?(?:default\s+)?(?:async\s+)?function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\(([^)]*)\)\s*\{)");
  std::smatch match;
  if (!std::regex_search(first_line, match, header) || match.size() < 3 || match[1].str() != record.name) return false;
  params = match[2].str();
  const auto brace = chunk.code.find('{', begin + static_cast<std::size_t>(match.position(0)));
  if (brace == std::string::npos) return false;
  int depth = 0;
  bool in_single = false, in_double = false, in_template = false, in_line_comment = false, in_block_comment = false, escape = false;
  for (std::size_t i = brace; i < chunk.code.size(); ++i) {
    const char c = chunk.code[i];
    const char n = i + 1u < chunk.code.size() ? chunk.code[i + 1u] : '\0';
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
    if (c == '{') ++depth;
    else if (c == '}' && --depth == 0) {
      end = i + 1u;
      declaration = chunk.code.substr(begin, end - begin);
      return true;
    }
  }
  return false;
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
    std::size_t begin = 0, end = 0;
    std::string declaration, params;
    if (!extract_function_declaration(*it, record, begin, end, declaration, params)) {
      rewrite.reason = "only named function declarations are currently extractable";
      result.records.push_back(std::move(rewrite));
      continue;
    }
    auto dependency_resolution = resolve_liftable_function_dependencies(
        it->code, declaration, record.name, params);
    // An explicitly isolated function is a complete protected compilation unit:
    // all helpers and constants are declared inside its body, so no browser
    // lexical capture is permitted or required. This avoids conservative false
    // positives from deeply nested third-party parser/engine implementations.
    if (!dependency_resolution.success && (record.isolated || declaration.find("@venom-isolated") != std::string::npos)) {
      dependency_resolution.success = true;
      dependency_resolution.reason = "explicit isolated protected compilation unit";
      dependency_resolution.dependencies.clear();
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
    try { calls_awaited = all_external_calls_are_awaited(it->code, record.name, begin, end, call_reason); }
    catch (const std::regex_error& error) { throw std::runtime_error(std::string("call-site regex: ") + error.what()); }
    if (!calls_awaited) {
      rewrite.reason = call_reason;
      result.records.push_back(std::move(rewrite));
      continue;
    }
    const auto candidate = rewrite.candidate;
    std::ostringstream stub;
    stub << "async function " << record.name << "(" << params << "){return globalThis.__venomInvokeProtectedByName(\""
         << json_escape_plan(record.name) << "\",Array.from(arguments));}";
    it->code.replace(begin, end - begin, stub.str());
    auto registry_declaration = trim_copy(declaration);
    try { registry_declaration = std::regex_replace(registry_declaration, std::regex(R"(^\s*export\s+(?:default\s+)?)"), ""); }
    catch (const std::regex_error& error) { throw std::runtime_error(std::string("registry declaration regex: ") + error.what()); }
    registry << "globalThis.__venomProtectedBridge[\"" << json_escape_plan(candidate) << "\"]=(function(){\n";
    for (const auto& dependency : dependency_resolution.dependencies)
      registry << dependency.declaration << "\n";
    registry << "return (" << registry_declaration << ");\n})();\n";
    rewrite.status = "extracted";
    rewrite.reason = dependency_resolution.dependencies.empty()
        ? "function declaration extracted; all external call sites are explicitly awaited"
        : "function declaration extracted with pure lexical dependencies; all external call sites are explicitly awaited";
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



struct BrowserEvidence {
  bool required = false;
  std::uint32_t confidence = 0;
  std::string reason;
};

BrowserEvidence browser_runtime_evidence(const std::string& source, const std::string& path) {
  const auto lower_path = lower_ascii(path);
  static const std::vector<std::pair<std::string, std::string>> browser_library_paths{
    {"jquery", "known browser library: jQuery"}, {"bootstrap", "known browser library: Bootstrap"},
    {"chessboard", "known browser library: chessboard.js"}, {"react", "known browser library: React"},
    {"vue", "known browser library: Vue"}, {"angular", "known browser library: Angular"},
    {"svelte", "known browser library: Svelte"}, {"three", "known browser library: Three.js"},
    {"pixi", "known browser library: PixiJS"}, {"leaflet", "known browser library: Leaflet"},
    {"mapbox", "known browser library: Mapbox"}, {"chart", "known browser library: Chart.js"}
  };
  for (const auto& marker : browser_library_paths) {
    if (lower_path.find(marker.first) != std::string::npos) return {true, 100u, marker.second};
  }

  const auto lower = lower_ascii(source);
  static const std::vector<std::pair<std::string, std::string>> browser_markers{
    {"document.", "references document"}, {"window.", "references window"},
    {"globalthis.document", "references globalThis.document"}, {"globalthis.window", "references globalThis.window"},
    {"document[", "indexes document"}, {"window[", "indexes window"},
    {"getelementbyid(", "queries DOM by id"}, {"queryselector(", "queries DOM"},
    {"queryselectorall(", "queries DOM collection"}, {"createelement(", "creates DOM elements"},
    {"addeventlistener(", "registers browser events"}, {"mutationobserver", "uses MutationObserver"},
    {"resizeobserver", "uses ResizeObserver"}, {"intersectionobserver", "uses IntersectionObserver"},
    {"htmlcanvaselement", "uses canvas"}, {"canvasrenderingcontext2d", "uses canvas 2D context"},
    {"webglrenderingcontext", "uses WebGL"}, {"requestanimationframe(", "uses animation frames"},
    {"cancelanimationframe(", "uses animation frames"}, {"localstorage", "uses localStorage"},
    {"sessionstorage", "uses sessionStorage"}, {"navigator.", "references navigator"},
    {"location.", "references location"}, {"history.", "references history"},
    {"xmlhttprequest", "uses XMLHttpRequest"}, {"websocket(", "uses WebSocket"},
    {"new worker(", "creates a Worker"}, {"jquery", "references jQuery"}, {"$(", "uses jQuery-style selector"},
    {"chessboard(", "creates a chessboard.js widget"}
  };
  for (const auto& marker : browser_markers) {
    if (lower.find(marker.first) != std::string::npos) return {true, 96u, marker.second};
  }
  return {false, 90u, "no browser-only evidence"};
}

void close_classic_browser_realms(std::vector<JsChunk>& chunks) {
  std::unordered_map<std::string, std::vector<std::size_t>> route_chunks;
  for (std::size_t i = 0; i < chunks.size(); ++i) {
    if ((chunks[i].flags & JsChunkModule) == 0u) route_chunks[chunks[i].route].push_back(i);
  }
  for (const auto& route_entry : route_chunks) {
    bool needs_browser_realm = false;
    std::string trigger;
    for (const auto index : route_entry.second) {
      if ((chunks[index].flags & JsChunkBrowser) != 0u) {
        needs_browser_realm = true;
        trigger = chunks[index].source;
        break;
      }
    }
    if (!needs_browser_realm) continue;
    for (const auto index : route_entry.second) {
      auto& chunk = chunks[index];
      if (has_file_scope_venom_protected_directive(chunk.code)) continue;
      if ((chunk.flags & JsChunkBrowser) == 0u) {
        chunk.flags |= JsChunkBrowser;
        chunk.execution_reason = "classic-script realm closure triggered by " + trigger;
        chunk.execution_confidence = 94u;
      }
    }
  }
}

bool is_remote_url(const std::string& value) {
  const auto lower = lower_ascii(value);
  return lower.rfind("http://", 0) == 0 || lower.rfind("https://", 0) == 0 ||
         lower.rfind("//", 0) == 0 || lower.rfind("data:", 0) == 0 ||
         lower.rfind("blob:", 0) == 0;
}

std::size_t find_case_insensitive(std::string_view haystack, std::string_view needle, std::size_t start) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return std::string::npos;
  }
  for (std::size_t i = start; i + needle.size() <= haystack.size(); ++i) {
    bool match = true;
    for (std::size_t j = 0; j < needle.size(); ++j) {
      const auto a = static_cast<unsigned char>(haystack[i + j]);
      const auto b = static_cast<unsigned char>(needle[j]);
      if (std::tolower(a) != std::tolower(b)) {
        match = false;
        break;
      }
    }
    if (match) {
      return i;
    }
  }
  return std::string::npos;
}

std::string decode_basic_entities(std::string value) {
  const std::pair<const char*, const char*> replacements[] = {
    {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&#39;", "'"},
  };
  for (const auto& item : replacements) {
    std::string from = item.first;
    std::string to = item.second;
    std::size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
      value.replace(pos, from.size(), to);
      pos += to.size();
    }
  }
  return value;
}

std::unordered_map<std::string, std::string> parse_attributes(std::string_view source) {
  std::unordered_map<std::string, std::string> attrs;
  std::size_t i = 0;
  while (i < source.size()) {
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) {
      ++i;
    }
    if (i >= source.size() || source[i] == '/' || source[i] == '>') {
      break;
    }
    const auto name_start = i;
    while (i < source.size() && is_name_char(source[i])) {
      ++i;
    }
    if (i == name_start) {
      ++i;
      continue;
    }
    auto name = lower_ascii(std::string(source.substr(name_start, i - name_start)));
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) {
      ++i;
    }
    std::string value;
    if (i < source.size() && source[i] == '=') {
      ++i;
      while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) {
        ++i;
      }
      if (i < source.size() && (source[i] == '"' || source[i] == '\'')) {
        const char quote = source[i++];
        const auto value_start = i;
        while (i < source.size() && source[i] != quote) {
          ++i;
        }
        value = std::string(source.substr(value_start, i - value_start));
        if (i < source.size()) {
          ++i;
        }
      } else {
        const auto value_start = i;
        while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) == 0 && source[i] != '>' && source[i] != '/') {
          ++i;
        }
        value = std::string(source.substr(value_start, i - value_start));
      }
    }
    attrs[std::move(name)] = decode_basic_entities(std::move(value));
  }
  return attrs;
}

const SiteFile* find_file(const SiteGraph& graph, const std::string& relative) {
  const auto normalized = normalize_slashes(relative);
  for (const auto& file : graph.files) {
    if (normalize_slashes(file.relative) == normalized) {
      return &file;
    }
  }
  return nullptr;
}

std::vector<const SiteFile*> sorted_html_files(const SiteGraph& graph) {
  std::vector<const SiteFile*> files;
  for (const auto& file : graph.files) {
    if (file.extension == ".html" || file.extension == ".htm") {
      files.push_back(&file);
    }
  }
  std::sort(files.begin(), files.end(), [](const SiteFile* a, const SiteFile* b) {
    return a->relative < b->relative;
  });
  return files;
}


struct ImportReference {
  std::string specifier;
  bool dynamic = false;
};

std::vector<ImportReference> scan_module_references(const std::string& source) {
  std::vector<ImportReference> refs;
  std::size_t i = 0;
  auto skip_space_and_comments = [&](std::size_t& p) {
    while (p < source.size()) {
      if (std::isspace(static_cast<unsigned char>(source[p])) != 0) { ++p; continue; }
      if (p + 1 < source.size() && source[p] == '/' && source[p + 1] == '/') {
        p += 2; while (p < source.size() && source[p] != '\n') ++p; continue;
      }
      if (p + 1 < source.size() && source[p] == '/' && source[p + 1] == '*') {
        p += 2; while (p + 1 < source.size() && !(source[p] == '*' && source[p + 1] == '/')) ++p;
        if (p + 1 < source.size()) p += 2;
        continue;
      }
      break;
    }
  };
  auto read_string = [&](std::size_t& p, std::string& out) -> bool {
    if (p >= source.size() || (source[p] != '\'' && source[p] != '"')) return false;
    const char q = source[p++]; out.clear();
    while (p < source.size()) {
      char c = source[p++];
      if (c == q) return true;
      if (c == '\\' && p < source.size()) { out.push_back(source[p++]); continue; }
      out.push_back(c);
    }
    return false;
  };
  while (i < source.size()) {
    skip_space_and_comments(i);
    if (i >= source.size()) break;
    if (source[i] == '\'' || source[i] == '"' || source[i] == '`') {
      const char q = source[i++];
      while (i < source.size()) { char c=source[i++]; if (c=='\\' && i<source.size()) ++i; else if (c==q) break; }
      continue;
    }
    if (!is_js_ident_start(source[i])) { ++i; continue; }
    const auto start=i++; while (i<source.size() && is_js_ident_continue(source[i])) ++i;
    const auto word=source.substr(start,i-start);
    if (word != "import" && word != "export") continue;
    std::size_t p=i; skip_space_and_comments(p);
    bool dynamic=false;
    if (word=="import" && p<source.size() && source[p]=='(') { dynamic=true; ++p; skip_space_and_comments(p); }
    std::string spec;
    if (read_string(p,spec)) { refs.push_back({spec,dynamic}); i=p; continue; }
    // static import/export ... from "specifier"
    while (p<source.size() && source[p]!=';' && source[p]!='\n') {
      skip_space_and_comments(p);
      if (p>=source.size()) break;
      if (source[p]=='\'' || source[p]=='"') {
        if (read_string(p,spec)) refs.push_back({spec,false});
        break;
      }
      ++p;
    }
    i=p;
  }
  return refs;
}

bool is_local_module_specifier(const std::string& specifier) {
  return specifier.rfind("./",0)==0 || specifier.rfind("../",0)==0 || (!specifier.empty() && specifier[0]=='/');
}

std::vector<JsChunk> collect_script_chunks(const SiteGraph& graph, const RemoteVendorOptions& remote_options, std::vector<RemoteVendorRecord>& remote_vendors, std::vector<ModuleGraphEdge>& module_edges) {
  std::vector<JsChunk> chunks;
  std::uint32_t order = 0;

  for (const auto* file : sorted_html_files(graph)) {
    const auto html = to_string(file->bytes);
    const auto route = route_from_html_path(file->relative);
    std::size_t cursor = 0;
    std::uint32_t inline_index = 0;

    while (true) {
      const auto start = find_case_insensitive(html, "<script", cursor);
      if (start == std::string::npos) {
        break;
      }
      const auto after_name = start + 7;
      if (after_name < html.size() && is_name_char(html[after_name])) {
        cursor = after_name;
        continue;
      }
      const auto open_end = html.find('>', after_name);
      if (open_end == std::string::npos) {
        break;
      }
      const auto attr_source = std::string_view(html).substr(after_name, open_end - after_name);
      const auto attrs = parse_attributes(attr_source);
      const auto close_start = find_case_insensitive(html, "</script", open_end + 1);
      const std::size_t content_end = close_start == std::string::npos ? open_end + 1 : close_start;
      const auto close_end = close_start == std::string::npos ? open_end : html.find('>', close_start);

      JsChunk chunk;
      chunk.route = route;
      chunk.order = order++;
      chunk.kind = 1;
      chunk.flags = 0;

      const auto type_it = attrs.find("type");
      if (type_it != attrs.end() && lower_ascii(type_it->second).find("module") != std::string::npos) {
        chunk.flags |= JsChunkModule;
      }
      if (attrs.find("defer") != attrs.end()) {
        chunk.flags |= JsChunkDefer;
      }
      if (attrs.find("async") != attrs.end()) {
        chunk.flags |= JsChunkAsync;
      }

      const auto src_it = attrs.find("src");
      if (src_it != attrs.end() && !src_it->second.empty()) {
        chunk.flags |= JsChunkExternalFile;
        chunk.kind = 2;
        if (is_remote_url(src_it->second)) {
          const auto integrity_it = attrs.find("integrity");
          const auto declared_integrity = integrity_it == attrs.end() ? std::string{} : integrity_it->second;
          const auto vendored = vendor_remote_script(src_it->second, declared_integrity, remote_options);
          chunk.flags |= JsChunkVendoredRemote;
          chunk.source = vendored.record.normalized_url;
          chunk.code.assign(reinterpret_cast<const char*>(vendored.bytes.data()), vendored.bytes.size());
          remote_vendors.push_back(vendored.record);
        } else {
          const auto resolved = normalize_slashes(dirname_of(file->relative) + src_it->second);
          const auto* script_file = find_file(graph, resolved);
          if (script_file == nullptr) {
            throw std::runtime_error("script source not found: " + src_it->second + " from " + file->relative);
          }
          chunk.source = script_file->relative;
          chunk.code = to_string(script_file->bytes);
        }
      } else {
        chunk.flags |= JsChunkInline;
        chunk.kind = 1;
        chunk.source = file->relative + "#inline-script-" + std::to_string(inline_index++);
        if (content_end > open_end + 1) {
          chunk.code = html.substr(open_end + 1, content_end - open_end - 1);
        }
      }

      const auto venom_it = attrs.find("data-venom");
      const auto venom_mode = venom_it == attrs.end() ? std::string{} : lower_ascii(venom_it->second);
      const bool browser_attribute = venom_mode == "browser";
      const bool protected_attribute = venom_mode == "protected";
      const bool explicit_browser = browser_attribute || has_file_scope_venom_browser_directive(chunk.code);
      const bool explicit_protected = protected_attribute || has_file_scope_venom_protected_directive(chunk.code);
      const auto evidence = browser_runtime_evidence(chunk.code, chunk.source);
      if (explicit_browser) {
        chunk.flags |= JsChunkBrowser;
        chunk.execution_reason = "explicit browser directive";
        chunk.execution_confidence = 100u;
      } else if (explicit_protected) {
        chunk.execution_reason = "explicit protected directive";
        chunk.execution_confidence = 100u;
      } else if (evidence.required) {
        chunk.flags |= JsChunkBrowser;
        chunk.execution_reason = evidence.reason;
        chunk.execution_confidence = evidence.confidence;
      } else {
        chunk.execution_reason = evidence.reason;
        chunk.execution_confidence = evidence.confidence;
      }
      chunks.push_back(std::move(chunk));
      cursor = close_end == std::string::npos ? html.size() : close_end + 1;
    }
  }

  // Collect HTML modulepreload hints as dependency-only module records. They are
  // package inputs, never network-fetched source at runtime.
  for (const auto* file : sorted_html_files(graph)) {
    const auto html = to_string(file->bytes);
    const auto route = route_from_html_path(file->relative);
    std::size_t cursor = 0;
    while (true) {
      const auto start = find_case_insensitive(html, "<link", cursor);
      if (start == std::string::npos) break;
      const auto after_name = start + 5;
      const auto open_end = html.find('>', after_name);
      if (open_end == std::string::npos) break;
      const auto attrs = parse_attributes(std::string_view(html).substr(after_name, open_end - after_name));
      const auto rel = attrs.find("rel");
      const auto href = attrs.find("href");
      if (rel != attrs.end() && href != attrs.end() && lower_ascii(rel->second).find("modulepreload") != std::string::npos && !is_remote_url(href->second)) {
        const auto resolved = normalize_slashes(dirname_of(file->relative) + href->second);
        if (const auto* dependency = find_file(graph, resolved)) {
          const bool exists = std::any_of(chunks.begin(), chunks.end(), [&](const JsChunk& chunk) { return chunk.route == route && normalize_slashes(chunk.source) == resolved; });
          if (!exists) {
            JsChunk child;
            child.route = route; child.source = dependency->relative; child.code = to_string(dependency->bytes);
            child.order = order++; child.kind = 3;
            child.flags = JsChunkModule | JsChunkExternalFile | JsChunkDependency | JsChunkModulePreload;
            child.execution_reason = "modulepreload dependency";
            child.execution_confidence = 100u;
            chunks.push_back(std::move(child));
          }
          module_edges.push_back({route, file->relative, href->second, resolved, false, true});
        } else {
          throw std::runtime_error("modulepreload source not found: " + href->second + " from " + file->relative);
        }
      }
      cursor = open_end + 1;
    }
  }

  // Recursively collect package-local module dependencies. Dependency records are
  // available to the QuickJS module compiler but are not executed as route entry scripts.
  std::unordered_set<std::string> known;
  for (const auto& chunk : chunks) if ((chunk.flags & JsChunkModule) != 0u) known.insert(chunk.route + "\n" + normalize_slashes(chunk.source));
  for (std::size_t index = 0; index < chunks.size(); ++index) {
    const auto parent = chunks[index];
    if ((parent.flags & JsChunkModule) == 0u) continue;
    for (const auto& ref : scan_module_references(parent.code)) {
      if (!is_local_module_specifier(ref.specifier)) continue;
      const auto resolved = normalize_slashes(ref.specifier[0] == '/' ? ref.specifier.substr(1) : dirname_of(parent.source) + ref.specifier);
      module_edges.push_back({parent.route, parent.source, ref.specifier, resolved, ref.dynamic, false});
      const auto key = parent.route + "\n" + resolved;
      if (known.count(key)) continue;
      const auto* dependency = find_file(graph, resolved);
      if (!dependency) throw std::runtime_error("module dependency not found: " + ref.specifier + " from " + parent.source);
      JsChunk child;
      child.route = parent.route;
      child.source = dependency->relative;
      child.code = to_string(dependency->bytes);
      child.order = order++;
      child.kind = 3;
      child.flags = JsChunkModule | JsChunkExternalFile | JsChunkDependency;
      if (ref.dynamic) child.flags |= JsChunkDynamicImport;
      child.execution_reason = ref.dynamic ? "dynamic module dependency" : "static module dependency";
      child.execution_confidence = 100u;
      chunks.push_back(std::move(child));
      known.insert(key);
    }
  }

  const auto function_realm_records = apply_function_realm_planning(chunks);
  (void)function_realm_records;

  // Classic scripts on one route share a single browser global realm. Once any
  // classic script requires the real DOM/browser environment, keep the complete
  // classic chain together unless a file explicitly opts back into protection.
  close_classic_browser_realms(chunks);
  return chunks;
}

std::vector<unsigned char> encode_js_bundle(const std::vector<JsChunk>& chunks) {
  struct Entry {
    std::uint32_t route_offset = 0;
    std::uint32_t route_size = 0;
    std::uint32_t source_offset = 0;
    std::uint32_t source_size = 0;
    std::uint32_t code_offset = 0;
    std::uint32_t code_size = 0;
    std::uint32_t order = 0;
    std::uint32_t flags = 0;
    std::uint32_t kind = 0;
    std::uint32_t reserved = 0;
  };

  std::vector<Entry> entries;
  std::vector<unsigned char> text_blob;
  std::vector<unsigned char> code_blob;
  std::vector<venom::quickjs::ModuleSourceRecord> module_sources;
  entries.reserve(chunks.size());
  module_sources.reserve(chunks.size());

  for (const auto& chunk : chunks) {
    if ((chunk.flags & JsChunkModule) == 0u) continue;
    module_sources.push_back({chunk.source, chunk.source, chunk.code});
  }

  for (const auto& chunk : chunks) {
    Entry entry;
    entry.route_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.route_size = static_cast<std::uint32_t>(chunk.route.size());
    text_blob.insert(text_blob.end(), chunk.route.begin(), chunk.route.end());

    entry.source_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.source_size = static_cast<std::uint32_t>(chunk.source.size());
    text_blob.insert(text_blob.end(), chunk.source.begin(), chunk.source.end());

    const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    const bool is_dependency = (chunk.flags & JsChunkDependency) != 0u;
    const bool has_module_requests = is_module && !scan_module_references(chunk.code).empty();
    std::vector<unsigned char> payload;
    if (browser_side) payload.assign(chunk.code.begin(), chunk.code.end());
    else payload = (is_module && !is_dependency && has_module_requests)
        ? venom::quickjs::compile_native_quickjs_module_bundle(chunk.source, module_sources, false)
        : venom::quickjs::compile_native_quickjs_bytecode(chunk.code, chunk.source, is_module, false, is_module ? &module_sources : nullptr);
    entry.code_offset = static_cast<std::uint32_t>(code_blob.size());
    entry.code_size = static_cast<std::uint32_t>(payload.size());
    code_blob.insert(code_blob.end(), payload.begin(), payload.end());
    entry.order = chunk.order;
    entry.flags = browser_side ? chunk.flags : (chunk.flags | JsChunkBytecodeEncoded);
    entry.kind = chunk.kind;
    entries.push_back(entry);
  }

  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'J', 'S', 'B', '0', '0', '0', '6'});
  append_u32(out, 1);
  append_u32(out, static_cast<std::uint32_t>(entries.size()));
  append_u32(out, static_cast<std::uint32_t>(text_blob.size()));
  append_u32(out, 0);

  for (const auto& entry : entries) {
    append_u32(out, entry.route_offset);
    append_u32(out, entry.route_size);
    append_u32(out, entry.source_offset);
    append_u32(out, entry.source_size);
    append_u32(out, entry.code_offset);
    append_u32(out, entry.code_size);
    append_u32(out, entry.order);
    append_u32(out, entry.flags);
    append_u32(out, entry.kind);
    append_u32(out, entry.reserved);
  }
  out.insert(out.end(), text_blob.begin(), text_blob.end());
  out.insert(out.end(), code_blob.begin(), code_blob.end());
  return out;
}

std::string make_js_diagnostics(const std::vector<JsChunk>& chunks) {
  std::ostringstream out;
  out << "VJSD0006\n";
  out << "version\t1\n";
  out << "count\t" << chunks.size() << "\n";
  out << "order\troute\tkind\tflags\tsource\tbytes\n";
  for (const auto& chunk : chunks) {
    out << chunk.order << '\t'
        << chunk.route << '\t'
        << chunk.kind << '\t'
        << js_flags_summary(chunk.flags) << '\t'
        << chunk.source << '\t'
        << chunk.code.size() << '\n';
  }
  return out.str();
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
        if (c < 0x20u) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned int>(c) << std::dec;
        else out << static_cast<char>(c);
    }
  }
  return out.str();
}

std::string make_execution_plan_text(const std::vector<JsChunk>& chunks) {
  std::ostringstream out;
  out << "VENOM_EXECUTION_PLAN_V1\n";
  out << "order\troute\texecution\tconfidence\tsource\treason\n";
  for (const auto& chunk : chunks) {
    out << chunk.order << '\t' << chunk.route << '\t'
        << (((chunk.flags & JsChunkBrowser) != 0u) ? "browser" : "protected") << '\t'
        << chunk.execution_confidence << '\t' << chunk.source << '\t' << chunk.execution_reason << '\n';
  }
  return out.str();
}

std::string make_execution_plan_json(const std::vector<JsChunk>& chunks) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 1,\n  \"scripts\": [\n";
  for (std::size_t i = 0; i < chunks.size(); ++i) {
    const auto& chunk = chunks[i];
    out << "    {\"order\":" << chunk.order
        << ",\"route\":\"" << json_escape_plan(chunk.route)
        << "\",\"source\":\"" << json_escape_plan(chunk.source)
        << "\",\"execution\":\"" << (((chunk.flags & JsChunkBrowser) != 0u) ? "browser" : "protected")
        << "\",\"confidence\":" << chunk.execution_confidence
        << ",\"reason\":\"" << json_escape_plan(chunk.execution_reason)
        << "\",\"flags\":\"" << json_escape_plan(js_flags_summary(chunk.flags)) << "\"}";
    if (i + 1 != chunks.size()) out << ',';
    out << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

} // namespace

std::string js_flags_summary(std::uint32_t flags) {
  std::vector<std::string> names;
  if ((flags & JsChunkInline) != 0) names.push_back("inline");
  if ((flags & JsChunkModule) != 0) names.push_back("module");
  if ((flags & JsChunkDefer) != 0) names.push_back("defer");
  if ((flags & JsChunkAsync) != 0) names.push_back("async");
  if ((flags & JsChunkExternalFile) != 0) names.push_back("external-file");
  if ((flags & JsChunkRemoteUrl) != 0) names.push_back("remote-url");
  if ((flags & JsChunkBytecodeEncoded) != 0) names.push_back("bytecode");
  if ((flags & JsChunkVendoredRemote) != 0) names.push_back("vendored-remote");
  if ((flags & JsChunkDependency) != 0) names.push_back("dependency");
  if ((flags & JsChunkDynamicImport) != 0) names.push_back("dynamic-import");
  if ((flags & JsChunkModulePreload) != 0) names.push_back("modulepreload");
  if ((flags & JsChunkBrowser) != 0) names.push_back("browser");
  if (names.empty()) return "none";
  std::ostringstream out;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0) out << ',';
    out << names[i];
  }
  return out.str();
}

JsBridge compile_js_bridge(const SiteGraph& graph, const RemoteVendorOptions& remote_options) {
  JsBridge bridge;
  bridge.chunks = collect_script_chunks(graph, remote_options, bridge.remote_vendors, bridge.module_edges);
  const auto function_records = [] (const std::vector<JsChunk>& chunks) {
    std::vector<FunctionRealmRecord> all;
    for (const auto& chunk : chunks) {
      auto local = scan_function_realm_directives(chunk);
      const bool promoted = (chunk.flags & JsChunkBrowser) != 0u && chunk.execution_reason.find("function-level browser directive") != std::string::npos;
      for (auto& record : local) record.promoted_whole_file = promoted;
      all.insert(all.end(), local.begin(), local.end());
    }
    return all;
  }(bridge.chunks);
  const auto extraction_records = analyze_function_extraction(bridge.chunks, function_records);
  const auto rewrite_result = apply_bridge_rewrites(bridge.chunks, extraction_records, remote_options.bridge_id_salt);
  bridge.diagnostics = make_js_diagnostics(bridge.chunks);
  bridge.execution_plan_text = make_execution_plan_text(bridge.chunks);
  bridge.execution_plan_json = make_execution_plan_json(bridge.chunks);
  bridge.function_plan_text = make_function_plan_text(function_records);
  bridge.function_plan_json = make_function_plan_json(function_records);
  bridge.extraction_plan_text = make_extraction_plan_text(extraction_records);
  bridge.extraction_plan_json = make_extraction_plan_json(extraction_records);
  bridge.realm_bridge_contract_json = make_realm_bridge_contract_json(extraction_records);
  bridge.bridge_rewrite_report_json = make_bridge_rewrite_report_json(rewrite_result.records);
  for (const auto& record : rewrite_result.records) {
    if (record.status == "extracted") {
      bridge.bridge_candidate_ids.push_back(record.candidate);
      bridge.protected_exports.emplace_back(record.function, record.candidate);
    }
  }
  if (!rewrite_result.registry_source.empty()) {
    bridge.bridge_registry_bytecode = venom::quickjs::compile_native_quickjs_bytecode(
        rewrite_result.registry_source, "venom://protected-bridge-registry", false, false, nullptr);
  }
  bridge.bundle = encode_js_bundle(bridge.chunks);

  std::ostringstream preview;
  for (const auto& chunk : bridge.chunks) {
    preview << "\n// route: " << chunk.route << " source: " << chunk.source
            << " flags: " << js_flags_summary(chunk.flags) << "\n";
    preview << chunk.code << "\n";
  }
  bridge.preview = preview.str();
  return bridge;
}


std::string js_hash64_literal(std::uint64_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

std::string make_loader_js(const std::string& runtime_asset_name, const std::string& package_asset_name, std::uint64_t expected_package_hash, const std::string& expected_package_sha256, bool hardened, const std::string& quickjs_engine_asset_name, const std::string& quickjs_runtime_wasm_asset_name, const std::string& runtime_wasm_asset_name, const std::string& style_asset_name, const std::string& package_binding_token, const std::string& worker_asset_name, const std::string& quickjs_runtime_wasm_sha256, const std::vector<std::pair<std::string, std::string>>& protected_exports,
                           std::uint32_t bridge_invoke_opcode,
                           std::uint32_t bridge_cancel_opcode,
                           std::uint32_t bridge_result_opcode,
                           std::uint32_t bridge_error_opcode) {
  std::ostringstream out;
  out << "import { bootVenom, renderVenomFailure } from './" << runtime_asset_name << "';\n\n";
  out << "const bootOptions = {\n"
      << "  root: document.getElementById('venom-root'),\n"
      << "  packageUrl: new URL('" << package_asset_name << "', import.meta.url).href,\n"
      << "  assetBaseUrl: new URL('" << (hardened ? "../" : "./") << "', import.meta.url).href,\n"
      << "  runtimeUrl: new URL('" << runtime_asset_name << "', import.meta.url).href,\n"
      << "  loaderUrl: import.meta.url,\n"
      << "  expectedPackageHash: '" << js_hash64_literal(expected_package_hash) << "',\n"
      << "  expectedPackageSha256: '" << expected_package_sha256 << "',\n"
      << "  hardened: " << (hardened ? "true" : "false") << ",\n";
  if (!style_asset_name.empty()) out << "  styleUrl: new URL('" << style_asset_name << "', import.meta.url).href,\n";
  if (!runtime_wasm_asset_name.empty()) out << "  runtimeWasmUrl: new URL('" << runtime_wasm_asset_name << "', import.meta.url).href,\n";
  if (!package_binding_token.empty()) out << "  bindingToken: '" << package_binding_token << "',\n";
  if (!quickjs_engine_asset_name.empty()) out << "  quickJsEngineUrl: new URL('" << quickjs_engine_asset_name << "', import.meta.url).href,\n";
  if (!quickjs_runtime_wasm_asset_name.empty()) out << "  quickJsWasmUrl: new URL('" << quickjs_runtime_wasm_asset_name << "', import.meta.url).href,\n";
  if (!quickjs_runtime_wasm_sha256.empty()) out << "  expectedQuickJsWasmSha256: '" << quickjs_runtime_wasm_sha256 << "',\n";
  if (!worker_asset_name.empty()) out << "  workerUrl: new URL('" << worker_asset_name << "', import.meta.url).href,\n";
  out << "};\n";
  if (!worker_asset_name.empty()) {
    out << "const __venomInvokeOp=" << bridge_invoke_opcode << ",__venomCancelOp=" << bridge_cancel_opcode << ",__venomResultOp=" << bridge_result_opcode << ",__venomErrorOp=" << bridge_error_opcode << ";\n";
    out << "let __venomReadyResolve,__venomReadyReject; const __venomReadyPromise=new Promise((resolve,reject)=>{__venomReadyResolve=resolve;__venomReadyReject=reject;});\n"
        << "async function prepareWorkerOwnedRuntime() {\n"
        << "  if (typeof Worker !== 'function') throw new Error('protected release requires Web Worker support');\n"
        << "  const worker = new Worker(bootOptions.workerUrl, { type: 'module', name: 'venom-engine' });\n"
        << "  const token = crypto.getRandomValues(new Uint32Array(4));\n"
        << "  const nonce = Array.from(token, (v) => v.toString(16).padStart(8, '0')).join('');\n"
        << "  const prepared = await new Promise((resolve, reject) => {\n"
        << "    const timer = setTimeout(() => reject(new Error('worker runtime preparation timed out')), 15000);\n"
        << "    worker.onmessage = (event) => { const m = event.data || {}; if (m.nonce !== nonce) return; clearTimeout(timer); if (m.protocol !== 1) return reject(new Error('worker protocol version mismatch')); if (m.type !== 'ready') return reject(new Error(m.message || 'worker runtime preparation failed')); if (m.packageHash !== bootOptions.expectedPackageHash) return reject(new Error('worker package hash attestation mismatch')); if (m.packageSha256 !== bootOptions.expectedPackageSha256) return reject(new Error('worker package SHA-256 attestation mismatch')); if (m.quickJsWasmSha256 !== bootOptions.expectedQuickJsWasmSha256) return reject(new Error('worker QuickJS WASM attestation mismatch')); if (m.quickJsRuntimeReady !== true) return reject(new Error('worker QuickJS WASM readiness attestation missing')); resolve(m); };\n"
        << "    worker.onerror = (event) => { clearTimeout(timer); reject(new Error(event.message || 'worker startup failed')); };\n"
        << "    worker.postMessage({ protocol: 1, type: 'prepare', nonce, packageUrl: bootOptions.packageUrl, quickJsWasmUrl: bootOptions.quickJsWasmUrl, expectedPackageHash: bootOptions.expectedPackageHash, expectedPackageSha256: bootOptions.expectedPackageSha256, expectedQuickJsWasmSha256: bootOptions.expectedQuickJsWasmSha256, maxPackageBytes: 67108864, maxWasmBytes: 33554432 });\n"
        << "  });\n"
        << "  bootOptions.packageBytes = prepared.packageBytes;\n"
        << "  if (prepared.quickJsModule) globalThis.__venomWorkerQuickJsModule = prepared.quickJsModule;\n"
        << "  const pendingBridge = new Map(); const protectedExports = new Map(); let bridgeSequence = 0; let bridgeCounter = 0; const bridgeSession = String(prepared.bridgeSession || ''); if (!/^[0-9a-f]{32}$/i.test(bridgeSession)) throw new Error('worker bridge session attestation missing');\n"
        << "  const MAX_PUBLIC_PAYLOAD_BYTES=1048576,MAX_PUBLIC_TIMEOUT_MS=30000,DEFAULT_PUBLIC_TIMEOUT_MS=5000;\n"
        << "  class VenomBridgeError extends Error{constructor(code,message){super(message);this.name='VenomBridgeError';this.code=code||'BRIDGE_ERROR';}}\n"
        << "  class VenomTimeoutError extends VenomBridgeError{constructor(){super('TIMEOUT','Protected operation timed out');this.name='VenomTimeoutError';}}\n"
        << "  function publicBridgeError(code,message){return new VenomBridgeError(String(code||'BRIDGE_ERROR').toUpperCase().replace(/-/g,'_'),String(message||'Protected operation failed'));}\n"
        << "  worker.onmessage=(event)=>{const m=event.data;if(!Array.isArray(m)||m[0]!==1||String(m[2]||'')!==bridgeSession)return;const op=Number(m[1])>>>0,counter=Number(m[3])>>>0,id=String(m[4]||''),p=pendingBridge.get(id);if(!p||counter!==p.counter||(op!==__venomResultOp&&op!==__venomErrorOp))return;pendingBridge.delete(id);clearTimeout(p.timer);if(p.abortCleanup)p.abortCleanup();if(op===__venomResultOp)p.resolve(m[5]);else p.reject(publicBridgeError(m[5],m[6]));};\n"
        << "  function encodeJson(value,label){let encoded;try{encoded=JSON.stringify(value);}catch(error){throw new TypeError(label+' must contain only JSON-serializable values');}if(typeof encoded!=='string')throw new TypeError(label+' must be a JSON value');if(new TextEncoder().encode(encoded).byteLength>MAX_PUBLIC_PAYLOAD_BYTES)throw new RangeError(label+' exceeds the 1 MiB bridge limit');return {encoded,value:JSON.parse(encoded)};}\n"
        << "  function invokeCandidate(candidateSlot,args,options){options=options||{};let normalized;try{normalized=encodeJson(Array.isArray(args)?args:[],'Protected arguments').value;}catch(error){error.code='INVALID_ARGUMENTS';return Promise.reject(error);}candidateSlot=Number(candidateSlot);if(!Number.isInteger(candidateSlot)||candidateSlot<0)return Promise.reject(publicBridgeError('UNKNOWN_EXPORT','Unknown protected export'));const timeout=Math.max(1,Math.min(MAX_PUBLIC_TIMEOUT_MS,Number(options.timeout)||DEFAULT_PUBLIC_TIMEOUT_MS));const signal=options.signal;if(signal&&signal.aborted)return Promise.reject(new DOMException('The protected operation was aborted','AbortError'));return new Promise((resolve,reject)=>{const requestId='v'+Date.now().toString(36)+(++bridgeSequence).toString(36)+crypto.getRandomValues(new Uint32Array(1))[0].toString(36);const counter=++bridgeCounter;let settled=false;const finish=(fn,value)=>{if(settled)return;settled=true;pendingBridge.delete(requestId);clearTimeout(timer);if(abortCleanup)abortCleanup();fn(value);};const timer=setTimeout(()=>{worker.postMessage([1,__venomCancelOp,bridgeSession,++bridgeCounter,requestId]);finish(reject,new VenomTimeoutError());},timeout);let abortCleanup=null;if(signal){const onAbort=()=>{worker.postMessage([1,__venomCancelOp,bridgeSession,++bridgeCounter,requestId]);finish(reject,new DOMException('The protected operation was aborted','AbortError'));};signal.addEventListener('abort',onAbort,{once:true});abortCleanup=()=>signal.removeEventListener('abort',onAbort);}pendingBridge.set(requestId,{resolve:(v)=>finish(resolve,v),reject:(e)=>finish(reject,e),timer,abortCleanup,counter});worker.postMessage([1,__venomInvokeOp,bridgeSession,counter,requestId,candidateSlot,normalized]);});}\n"
        << "  globalThis.__venomInvokeProtected=(candidateSlot,args,options)=>invokeCandidate(candidateSlot,args,options);\n"
        << "  globalThis.__venomInvokeProtectedByName=(name,args,options)=>{const candidateSlot=protectedExports.get(String(name||''));if(candidateSlot===undefined)return Promise.reject(publicBridgeError('UNKNOWN_EXPORT','Unknown protected export'));return invokeCandidate(candidateSlot,args,options);};\n"
        << "  globalThis.__venomRegisterProtectedExport=(name,candidateSlot)=>{name=String(name||'');candidateSlot=Number(candidateSlot);if(!/^[A-Za-z_$][A-Za-z0-9_$]*$/.test(name)||!Number.isInteger(candidateSlot)||candidateSlot<0)throw new Error('invalid protected export registration');const prior=protectedExports.get(name);if(prior!==undefined&&prior!==candidateSlot)throw new Error('duplicate protected export: '+name);protectedExports.set(name,candidateSlot);};\n";
    for (std::size_t export_index = 0; export_index < protected_exports.size(); ++export_index) {
      const auto& entry = protected_exports[export_index];
      out << "  globalThis.__venomRegisterProtectedExport(\"" << json_escape_plan(entry.first) << "\"," << export_index << ");\n";
    }
    out        << "  const venomApi=Object.freeze({ready:()=>__venomReadyPromise,call:(name,input,options)=>{const candidateSlot=protectedExports.get(String(name||''));if(candidateSlot===undefined)return Promise.reject(publicBridgeError('UNKNOWN_EXPORT','Unknown protected export'));return invokeCandidate(candidateSlot,[input],options);},info:()=>Object.freeze({protocol:1,transport:'worker-message-v1',valueContract:'json-value-v1',maxPayloadBytes:MAX_PUBLIC_PAYLOAD_BYTES,maxTimeoutMs:MAX_PUBLIC_TIMEOUT_MS,exports:Array.from(protectedExports.keys()).sort()}),exports:new Proxy(Object.create(null),{get:(_,name)=>typeof name==='string'&&protectedExports.has(name)?((input,options)=>venomApi.call(name,input,options)):undefined,has:(_,name)=>typeof name==='string'&&protectedExports.has(name),ownKeys:()=>Array.from(protectedExports.keys()).sort(),getOwnPropertyDescriptor:(_,name)=>typeof name==='string'&&protectedExports.has(name)?{enumerable:true,configurable:true}:undefined,set:()=>false})});\n"
        << "  Object.defineProperty(globalThis,'venom',{value:venomApi,writable:false,configurable:false,enumerable:true});\n"
        << "  __venomReadyResolve(venomApi);\n"
        << "}\n"
        << "prepareWorkerOwnedRuntime().then(() => bootVenom(bootOptions)).catch((error) => { __venomReadyReject(error); console.error('[venom] boot failed', error); renderVenomFailure(bootOptions.root, error); });\n";
  } else {
    out << "bootVenom(bootOptions).catch((error) => { console.error('[venom] boot failed', error); renderVenomFailure(bootOptions.root, error); });\n";
  }
  return out.str();
}

std::string bundle_js_preview(const SiteGraph& graph) {
  std::ostringstream out;
  for (const auto& file : graph.files) {
    if (file.extension == ".js" || file.extension == ".mjs") {
      out << "\n// source: " << file.relative << "\n";
      out.write(reinterpret_cast<const char*>(file.bytes.data()), static_cast<std::streamsize>(file.bytes.size()));
      out << "\n";
    }
  }
  return out.str();
}

} // namespace venom::compiler
