#include "venom/base/error.hpp"
#include "venom/internal/pipeline/js_planning.hpp"

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



struct ParsedFunctionDirective {
  std::string runtime;
  bool isolated = false;
  bool module = false;
  bool malformed = false;
};

ParsedFunctionDirective parse_function_runtime_directive(std::string text) {
  text = lower_ascii(std::move(text));
  const auto marker = text.find("@venom:");
  if (marker == std::string::npos) return {};
  text.erase(0, marker + 7u);
  while (!text.empty() && (std::isspace(static_cast<unsigned char>(text.front())) != 0 || text.front() == '*')) text.erase(text.begin());
  std::istringstream tokens(text);
  std::string head;
  tokens >> head;
  if (head != "browser" && head != "protected") {
    if (head.find("browser") != std::string::npos || head.find("protected") != std::string::npos) return {{}, false, false, true};
    return {};
  }
  ParsedFunctionDirective result;
  result.runtime = head;
  std::string modifier;
  while (tokens >> modifier) {
    while (!modifier.empty() && (modifier.back() == ',' || modifier.back() == ';')) modifier.pop_back();
    if (modifier == "isolated") result.isolated = true;
    else if (modifier == "module") result.module = true;
    else { result.malformed = true; break; }
  }
  if (result.runtime == "browser" && (result.isolated || result.module)) result.malformed = true;
  if (result.isolated && result.module) result.malformed = true;
  return result;
}

std::vector<std::string> js_comment_fragments(const std::string& line, bool& in_block_comment) {
  std::vector<std::string> fragments;
  std::string current;
  bool in_single = false, in_double = false, in_template = false, escape = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    const char n = i + 1u < line.size() ? line[i + 1u] : '\0';
    if (in_block_comment) {
      if (c == '*' && n == '/') {
        fragments.push_back(current);
        current.clear();
        in_block_comment = false;
        ++i;
      } else current.push_back(c);
      continue;
    }
    if (escape) { escape = false; continue; }
    if ((in_single || in_double || in_template) && c == '\\') { escape = true; continue; }
    if (!in_double && !in_template && c == '\'') { in_single = !in_single; continue; }
    if (!in_single && !in_template && c == '"') { in_double = !in_double; continue; }
    if (!in_single && !in_double && c == '`') { in_template = !in_template; continue; }
    if (in_single || in_double || in_template) continue;
    if (c == '/' && n == '/') {
      fragments.push_back(line.substr(i + 2u));
      break;
    }
    if (c == '/' && n == '*') {
      in_block_comment = true;
      current.clear();
      ++i;
    }
  }
  if (in_block_comment && !current.empty()) fragments.push_back(current);
  return fragments;
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
          static const char* digits = "0123456789abcdef";
          out << "\\u00" << digits[(c >> 4u) & 0x0fu] << digits[c & 0x0fu];
        } else out << static_cast<char>(c);
    }
  }
  return out.str();
}

} // namespace

std::vector<FunctionRuntimeRecord> scan_function_runtime_directives(const JsChunk& chunk) {
  std::vector<FunctionRuntimeRecord> records;
  const std::regex function_declaration(
      R"(^\s*(?:(?:export\s+)?(?:default\s+)?)?(?:async\s+)?function\s*\*?\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\()");
  const std::regex arrow_declaration_prefix(
      R"(^\s*(?:export\s+)?(?:const|let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*(?:async\s+)?)");
  const std::regex function_expression_prefix(
      R"(^\s*(?:export\s+)?(?:const|let|var)\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*(?:async\s+)?function(?:\s+[A-Za-z_$][A-Za-z0-9_$]*)?\s*\()" );
  std::istringstream input(chunk.code);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(input, line)) lines.push_back(line);

  std::string pending_runtime;
  std::string pending_reason;
  bool pending_isolated = false;
  std::uint32_t pending_line = 0;
  bool in_block_comment = false;
  bool saw_noncomment_code = false;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    line = lines[index];
    const auto line_number = static_cast<std::uint32_t>(index + 1u);
    bool saw_directive_comment = false;
    for (const auto& fragment : js_comment_fragments(line, in_block_comment)) {
      const auto parsed = parse_function_runtime_directive(fragment);
      if (parsed.runtime.empty() && !parsed.malformed) continue;
      saw_directive_comment = true;
      if (parsed.module && !saw_noncomment_code && pending_runtime.empty() &&
          parsed.runtime == "protected" && (chunk.flags & JsChunkBrowser) == 0u) {
        continue;
      }
      if (parsed.module) {
        raise_error("VENOM-E5000", "protected module directive is only valid at file scope in " + chunk.source + ":" + std::to_string(line_number));
      }
      if (parsed.malformed) {
        raise_error("VENOM-E5000", "malformed function-level Venom runtime directive in " + chunk.source + ":" + std::to_string(line_number));
      }
      const bool browser = parsed.runtime == "browser";
      const bool protected_runtime = parsed.runtime == "protected";
      if (browser || protected_runtime) {
        const std::string next_runtime = parsed.runtime;
        const bool matches_file_runtime = (next_runtime == "browser") == ((chunk.flags & JsChunkBrowser) != 0u);
        if (!saw_noncomment_code && pending_runtime.empty() && matches_file_runtime) {
          // The leading runtime annotation belongs to the file, not the next declaration.
          continue;
        }
        if (!pending_runtime.empty()) {
          raise_error("VENOM-E5000", "ambiguous consecutive function-level Venom directives in " + chunk.source + ":" +
                                   std::to_string(pending_line) + " and " + std::to_string(line_number));
        }
        pending_runtime = next_runtime;
        pending_reason = browser ? "explicit function-level browser directive" : "explicit function-level protected directive";
        pending_isolated = protected_runtime && parsed.isolated;
        pending_line = line_number;
      }
    }
    if (saw_directive_comment) continue;
    const auto first_nonspace = line.find_first_not_of(" \t\r");
    if (first_nonspace != std::string::npos && line.compare(first_nonspace, 2, "//") != 0 &&
        line.compare(first_nonspace, 2, "/*") != 0 && line[first_nonspace] != '*') saw_noncomment_code = true;
    if (pending_runtime.empty()) continue;
    if (first_nonspace == std::string::npos || line.compare(first_nonspace, 2, "//") == 0 ||
        line.compare(first_nonspace, 2, "/*") == 0 || line[first_nonspace] == '*') continue;

    // Build a bounded declaration header so multiline arrow parameters are recognized
    // without consuming a function body or a following statement.
    std::string header = line;
    std::size_t last = index;
    for (; last + 1u < lines.size() && last < index + 31u && header.find("=>") == std::string::npos &&
           header.find(';') == std::string::npos; ++last) {
      header.push_back('\n');
      header += lines[last + 1u];
    }
    std::smatch match;
    std::string name;
    if (std::regex_search(header, match, function_declaration) && match.size() > 1 && match[1].matched) {
      name = match[1].str();
    } else if (header.find("=>") != std::string::npos &&
               std::regex_search(header, match, arrow_declaration_prefix) && match.size() > 1 && match[1].matched) {
      name = match[1].str();
    } else if (std::regex_search(header, match, function_expression_prefix) && match.size() > 1 && match[1].matched) {
      name = match[1].str();
    } else {
      raise_error("VENOM-E5000", "orphaned function-level Venom directive in " + chunk.source + ":" +
                               std::to_string(pending_line) +
                               "; directive must bind to the immediately following supported declaration");
    }
    if (!name.empty()) records.push_back({chunk.route, chunk.source, name, pending_runtime, pending_reason, line_number, false, pending_isolated});
    pending_runtime.clear();
    pending_reason.clear();
    pending_isolated = false;
    pending_line = 0;
  }
  if (!pending_runtime.empty()) {
    raise_error("VENOM-E5000", "orphaned function-level Venom directive at end of file in " + chunk.source + ":" +
                             std::to_string(pending_line));
  }
  return records;
}

std::vector<FunctionRuntimeRecord> apply_function_runtime_planning(std::vector<JsChunk>& chunks) {
  std::vector<FunctionRuntimeRecord> records;
  for (auto& chunk : chunks) {
    auto local = scan_function_runtime_directives(chunk);
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
        raise_error("VENOM-E5000", "function-level browser directive conflicts with file-scope protected directive in " + chunk.source);
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


std::vector<FunctionRuntimeRecord> collect_function_runtime_records(const std::vector<JsChunk>& chunks) {
  std::vector<FunctionRuntimeRecord> all;
  for (const auto& chunk : chunks) {
    auto local = scan_function_runtime_directives(chunk);
    const bool promoted = (chunk.flags & JsChunkBrowser) != 0u &&
                          chunk.execution_reason.find("function-level browser directive") != std::string::npos;
    for (auto& record : local) record.promoted_whole_file = promoted;
    all.insert(all.end(), local.begin(), local.end());
  }
  return all;
}

std::string make_function_plan_text(const std::vector<FunctionRuntimeRecord>& records) {
  std::ostringstream out;
  out << "VENOM_FUNCTION_PLAN_V1\n";
  out << "route\tsource\tline\tfunction\texecution\tpromoted_whole_file\treason\n";
  for (const auto& record : records) {
    out << record.route << '\t' << record.source << '\t' << record.line << '\t' << record.name << '\t'
        << record.execution << '\t' << (record.promoted_whole_file ? "true" : "false") << '\t' << record.reason << '\n';
  }
  return out.str();
}

std::string make_function_plan_json(const std::vector<FunctionRuntimeRecord>& records) {
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
                                                                   const std::vector<FunctionRuntimeRecord>& functions) {
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
      rec.blockers.push_back("synchronous cross-runtime result bridge unavailable");
    } else if (fn.execution == "protected" && file_browser) {
      // QuickJS supports ordinary JavaScript semantics such as `this`,
      // `arguments`, nested functions, and `with`. Only syntax that requires the
      // browser runtime or cannot be serialized across the JSON bridge is rejected.
      // Use member-access tokens to avoid false positives in comments such as
      // "documentation".
      static const std::vector<std::pair<std::string, std::string>> unsafe{
        {"eval(", "dynamic eval"}, {"super", "super binding"},
        {"new.target", "new.target binding"}, {"import.meta", "module metadata"}
      };
      for (const auto& item : unsafe) if (contains_token_ci(body, item.first)) rec.blockers.push_back(item.second);
      if (rec.blockers.empty()) {
        rec.disposition = "bridge-candidate";
        rec.bridge_mode = "async-json-v1";
        rec.reason = "function has no immediately unsafe runtime-bound syntax; eligible for generated asynchronous JSON-value bridge";
      } else {
        rec.disposition = "retained-browser";
        rec.reason = "function cannot be extracted safely";
      }
    } else {
      rec.reason = file_browser ? "function already resides in browser runtime" : "function already resides in protected runtime";
    }
    out.push_back(std::move(rec));
  }
  return out;
}

std::string make_extraction_plan_text(const std::vector<FunctionExtractionRecord>& records) {
  std::ostringstream out;
  out << "VENOM_FUNCTION_EXTRACTION_PLAN_V1\n";
  out << "route\tsource\tline\tfunction\trequested_runtime\tdisposition\tbridge_mode\treason\tblockers\n";
  for (const auto& r : records) {
    out << r.route << '\t' << r.source << '\t' << r.line << '\t' << r.name << '\t' << r.requested_runtime
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
        << "\",\"requested_runtime\":\"" << r.requested_runtime << "\",\"disposition\":\"" << r.disposition
        << "\",\"bridge_mode\":\"" << r.bridge_mode << "\",\"reason\":\"" << json_escape_plan(r.reason) << "\",\"blockers\":[";
    for (std::size_t j=0;j<r.blockers.size();++j) { if (j) out << ','; out << '\"' << json_escape_plan(r.blockers[j]) << '\"'; }
    out << "]}" << (i+1==records.size()?"":",") << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

std::string make_runtime_bridge_contract_json(const std::vector<FunctionExtractionRecord>& records) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 2,\n  \"runtime_api_version\": 1,\n  \"transport\": \"binary-capability-v3-leased\",\n  \"value_contract\": \"binary-json-v2\",\n  \"synchronous_calls\": false,\n  \"candidates\": [\n";
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
