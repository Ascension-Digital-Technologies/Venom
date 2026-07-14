#include "compiler/pipeline/js_planning.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace venom::compiler::detail {
namespace {

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

bool has_file_scope_venom_protected_directive(const std::string& source) {
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
    if (lower.find("@venom:") != std::string::npos && lower.find("protected") != std::string::npos) return true;
  }
  return false;
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
          static const char* digits = "0123456789abcdef";
          out << "\\u00" << digits[(c >> 4u) & 0x0fu] << digits[c & 0x0fu];
        } else out << static_cast<char>(c);
    }
  }
  return out.str();
}

} // namespace

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


std::vector<FunctionRealmRecord> collect_function_realm_records(const std::vector<JsChunk>& chunks) {
  std::vector<FunctionRealmRecord> all;
  for (const auto& chunk : chunks) {
    auto local = scan_function_realm_directives(chunk);
    const bool promoted = (chunk.flags & JsChunkBrowser) != 0u &&
                          chunk.execution_reason.find("function-level browser directive") != std::string::npos;
    for (auto& record : local) record.promoted_whole_file = promoted;
    all.insert(all.end(), local.begin(), local.end());
  }
  return all;
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
  out << "{\n  \"schema_version\": 2,\n  \"transport\": \"binary-capability-v3-leased\",\n  \"value_contract\": \"json-value-v1\",\n  \"synchronous_calls\": false,\n  \"candidates\": [\n";
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

} // namespace venom::compiler::detail
