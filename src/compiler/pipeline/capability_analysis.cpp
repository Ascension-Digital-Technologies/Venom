#include "compiler/pipeline/capability_analysis.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace venom::compiler {
namespace {

enum class TokenKind { Identifier, Keyword, Number, String, Template, Punct };
struct Token {
  TokenKind kind;
  std::string text;
  std::size_t line;
  std::size_t column;
};

bool is_ident_start(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalpha(u) || c == '_' || c == '$';
}
bool is_ident_continue(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) || c == '_' || c == '$';
}

const std::unordered_set<std::string> kKeywords = {
  "async", "await", "class", "const", "export", "extends", "function", "import",
  "let", "new", "return", "static", "typeof", "var", "yield"
};

std::string read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("unable to read compatibility input: " + path.string());
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

std::vector<Token> lex_javascript(const std::string& source, std::size_t base_line = 1) {
  std::vector<Token> out;
  std::size_t i = 0;
  std::size_t line = base_line;
  std::size_t column = 1;
  auto advance = [&](char c) {
    ++i;
    if (c == '\n') { ++line; column = 1; }
    else { ++column; }
  };
  while (i < source.size()) {
    const char c = source[i];
    if (std::isspace(static_cast<unsigned char>(c))) { advance(c); continue; }
    if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
      while (i < source.size() && source[i] != '\n') advance(source[i]);
      continue;
    }
    if (c == '/' && i + 1 < source.size() && source[i + 1] == '*') {
      advance('/'); advance('*');
      while (i < source.size()) {
        if (source[i] == '*' && i + 1 < source.size() && source[i + 1] == '/') {
          advance('*'); advance('/'); break;
        }
        advance(source[i]);
      }
      continue;
    }
    if (c == '\'' || c == '"' || c == '`') {
      const char quote = c;
      const auto start_line = line;
      const auto start_col = column;
      std::string value;
      advance(c);
      bool escaped = false;
      while (i < source.size()) {
        const char d = source[i];
        if (!escaped && d == quote) { advance(d); break; }
        if (!escaped && d == '\\') { escaped = true; advance(d); continue; }
        escaped = false;
        value.push_back(d);
        advance(d);
      }
      out.push_back({quote == '`' ? TokenKind::Template : TokenKind::String, value, start_line, start_col});
      continue;
    }
    if (is_ident_start(c)) {
      const auto start = i;
      const auto start_line = line;
      const auto start_col = column;
      while (i < source.size() && is_ident_continue(source[i])) advance(source[i]);
      auto text = source.substr(start, i - start);
      out.push_back({kKeywords.count(text) ? TokenKind::Keyword : TokenKind::Identifier, std::move(text), start_line, start_col});
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      const auto start = i;
      const auto start_line = line;
      const auto start_col = column;
      while (i < source.size() && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '.' || source[i] == '_')) advance(source[i]);
      out.push_back({TokenKind::Number, source.substr(start, i - start), start_line, start_col});
      continue;
    }
    const auto start_line = line;
    const auto start_col = column;
    std::string punct(1, c);
    if (i + 2 < source.size() && source.substr(i, 3) == "...") {
      punct = "..."; advance('.'); advance('.'); advance('.');
    } else if (i + 1 < source.size()) {
      const auto two = source.substr(i, 2);
      static const std::unordered_set<std::string> doubles = {
        "=>", "?.", "??", "&&", "||", "==", "!=", "<=", ">=", "++", "--", "**", "::"
      };
      if (doubles.count(two)) { punct = two; advance(source[i]); advance(source[i]); }
      else advance(c);
    } else advance(c);
    out.push_back({TokenKind::Punct, punct, start_line, start_col});
  }
  return out;
}

bool is_js_file(const std::filesystem::path& p) {
  auto e = p.extension().string();
  std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return e == ".js" || e == ".mjs" || e == ".cjs" || e == ".jsx" || e == ".ts" || e == ".tsx";
}
bool is_html_file(const std::filesystem::path& p) {
  auto e = p.extension().string();
  std::transform(e.begin(), e.end(), e.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  return e == ".html" || e == ".htm";
}

std::vector<std::pair<std::string, std::size_t>> extract_inline_scripts(const std::string& html) {
  std::vector<std::pair<std::string, std::size_t>> scripts;
  std::string lower = html;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
  std::size_t pos = 0;
  while ((pos = lower.find("<script", pos)) != std::string::npos) {
    const auto open_end = lower.find('>', pos);
    if (open_end == std::string::npos) break;
    const auto close = lower.find("</script", open_end + 1);
    if (close == std::string::npos) break;
    const auto prefix = html.substr(0, open_end + 1);
    const auto line = 1 + static_cast<std::size_t>(std::count(prefix.begin(), prefix.end(), '\n'));
    scripts.push_back({html.substr(open_end + 1, close - open_end - 1), line});
    pos = close + 8;
  }
  return scripts;
}

void bump(std::map<std::string, std::size_t>& map, const std::string& key) { ++map[key]; }
void add_occurrence(CapabilityGraph& g, const std::filesystem::path& file, const Token& t,
                    std::string kind, std::string name, std::string detail = {}) {
  g.occurrences.push_back({file, t.line, t.column, std::move(kind), std::move(name), std::move(detail)});
}

bool token_is(const std::vector<Token>& t, std::size_t i, const char* value) {
  return i < t.size() && t[i].text == value;
}
bool member_chain(const std::vector<Token>& t, std::size_t i, const char* object, const char* property) {
  return token_is(t, i, object) && token_is(t, i + 1, ".") && token_is(t, i + 2, property);
}
bool guarded_umd_require(const std::vector<Token>& t, std::size_t i) {
  const auto begin = i > 32 ? i - 32 : 0;
  const auto end = std::min(t.size(), i + 8);
  bool typeof_module = false, typeof_exports = false, amd = false;
  for (std::size_t j = begin; j < end; ++j) {
    if (token_is(t, j, "typeof") && token_is(t, j + 1, "module")) typeof_module = true;
    if (token_is(t, j, "typeof") && token_is(t, j + 1, "exports")) typeof_exports = true;
    if (member_chain(t, j, "define", "amd")) amd = true;
  }
  return typeof_module || typeof_exports || amd;
}

void analyze_tokens(CapabilityGraph& g, const std::filesystem::path& file, const std::vector<Token>& t) {
  static const std::unordered_set<std::string> browser_globals = {
    "AbortController", "Blob", "BroadcastChannel", "CustomEvent", "Event", "EventSource", "File", "FileReader", "FormData",
    "Headers", "IntersectionObserver", "MutationObserver", "Notification", "ResizeObserver", "Request",
    "Response", "RTCPeerConnection", "SharedArrayBuffer", "SubmitEvent", "URL", "URLSearchParams", "WebSocket", "Worker",
    "XMLHttpRequest", "indexedDB", "localStorage", "sessionStorage", "history", "location",
    "setTimeout", "setInterval", "requestAnimationFrame", "cancelAnimationFrame", "addEventListener", "removeEventListener", "dispatchEvent"
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    const auto& token = t[i];
    if (token.kind == TokenKind::Template) {
      bump(g.syntax, "template_literals");
      add_occurrence(g, file, token, "syntax", "template_literals");
      continue;
    }
    if (token.kind != TokenKind::Identifier && token.kind != TokenKind::Keyword) continue;

    if (token.text == "import") {
      if (token_is(t, i + 1, "(")) { bump(g.module_features, "dynamic_import"); add_occurrence(g, file, token, "module", "dynamic_import"); }
      else { bump(g.module_features, "static_import"); add_occurrence(g, file, token, "module", "static_import"); }
    } else if (token.text == "export") {
      bump(g.module_features, "exports"); add_occurrence(g, file, token, "module", "exports");
    } else if (token.text == "await") {
      bump(g.syntax, "await"); add_occurrence(g, file, token, "syntax", "await");
    } else if (token.text == "class") {
      bump(g.syntax, "classes"); add_occurrence(g, file, token, "syntax", "classes");
    } else if (token.text == "async") {
      bump(g.syntax, "async_functions"); add_occurrence(g, file, token, "syntax", "async_functions");
    } else if (token.text == "eval" && token_is(t, i + 1, "(")) {
      bump(g.syntax, "dynamic_eval"); add_occurrence(g, file, token, "syntax", "dynamic_eval");
    } else if (token.text == "Function" && i > 0 && token_is(t, i - 1, "new")) {
      bump(g.syntax, "function_constructor"); add_occurrence(g, file, token, "syntax", "function_constructor");
    } else if (token.text == "require" && token_is(t, i + 1, "(")) {
      const auto name = guarded_umd_require(t, i) ? "umd_commonjs_branch" : "require";
      bump(g.node_apis, name); add_occurrence(g, file, token, "node", name);
    } else if (member_chain(t, i, "process", "env") || token.text == "process") {
      bump(g.node_apis, "process"); add_occurrence(g, file, token, "node", "process");
    } else if (member_chain(t, i, "navigator", "gpu")) {
      bump(g.browser_apis, "WebGPU"); add_occurrence(g, file, token, "browser", "WebGPU");
    } else if (member_chain(t, i, "navigator", "mediaDevices")) {
      bump(g.browser_apis, "MediaDevices"); add_occurrence(g, file, token, "browser", "MediaDevices");
    } else if (member_chain(t, i, "import", "meta")) {
      bump(g.module_features, "import_meta"); add_occurrence(g, file, token, "module", "import_meta");
    } else if (token_is(t, i + 1, ".") && i + 2 < t.size()) {
      const auto& property = t[i + 2].text;
      static const std::unordered_set<std::string> dom_methods = {
        "querySelector", "querySelectorAll", "getElementById", "createElement", "createElementNS",
        "createTextNode", "createDocumentFragment", "appendChild", "insertBefore", "replaceChildren",
        "removeChild", "remove", "closest", "matches", "cloneNode", "focus", "blur"
      };
      static const std::unordered_set<std::string> dom_properties = {
        "textContent", "innerHTML", "outerHTML", "className", "classList", "dataset", "style",
        "value", "checked", "selected", "selectedIndex", "disabled", "defaultValue", "defaultChecked"
      };
      if (dom_methods.count(property)) {
        const auto name = std::string("dom.") + property;
        bump(g.browser_apis, name); add_occurrence(g, file, t[i + 2], "browser", name);
      } else if (dom_properties.count(property)) {
        std::string name = std::string("dom.") + property;
        if (property == "value") name = "input.value";
        else if (property == "checked" || property == "defaultChecked") name = "input.checked";
        else if (property == "selected" || property == "selectedIndex") name = "select.value";
        bump(g.browser_apis, name); add_occurrence(g, file, t[i + 2], "browser", name);
      } else if (property == "submit" || property == "requestSubmit" || property == "reset") {
        const auto name = std::string("form.") + property;
        bump(g.browser_apis, name); add_occurrence(g, file, t[i + 2], "browser", name);
      } else if (property == "preventDefault" || property == "stopPropagation" || property == "stopImmediatePropagation") {
        const auto name = std::string("event.") + property;
        bump(g.browser_apis, name); add_occurrence(g, file, t[i + 2], "browser", name);
      }
    } else if (browser_globals.count(token.text)) {
      bump(g.browser_apis, token.text); add_occurrence(g, file, token, "browser", token.text);
    } else if (token.text == "fetch" && token_is(t, i + 1, "(")) {
      bump(g.browser_apis, "fetch"); add_occurrence(g, file, token, "browser", "fetch");
    } else if (token.text == "React" || token.text == "ReactDOM") {
      bump(g.frameworks, "React"); add_occurrence(g, file, token, "framework", "React");
    } else if (token.text == "Vue" || token.text == "createApp") {
      bump(g.frameworks, "Vue"); add_occurrence(g, file, token, "framework", "Vue");
    } else if (token.text == "Svelte") {
      bump(g.frameworks, "Svelte"); add_occurrence(g, file, token, "framework", "Svelte");
    } else if (token.text == "Alpine") {
      bump(g.frameworks, "Alpine"); add_occurrence(g, file, token, "framework", "Alpine");
    } else if (token.text == "htmx") {
      bump(g.frameworks, "HTMX"); add_occurrence(g, file, token, "framework", "HTMX");
    }
  }
}

std::string escape_json(const std::string& s) {
  std::string out;
  for (unsigned char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          static const char hex[] = "0123456789abcdef";
          out += "\\u00"; out += hex[(c >> 4) & 0xf]; out += hex[c & 0xf];
        } else out += static_cast<char>(c);
    }
  }
  return out;
}

void write_map_json(std::ostringstream& out, const std::map<std::string, std::size_t>& values) {
  out << '{';
  bool first = true;
  for (const auto& [key, value] : values) {
    if (!first) out << ',';
    first = false;
    out << '"' << escape_json(key) << "\":" << value;
  }
  out << '}';
}

void print_map(const char* title, const std::map<std::string, std::size_t>& values) {
  if (values.empty()) return;
  std::cout << title << ":\n";
  for (const auto& [key, count] : values) std::cout << "  " << key << ": " << count << "\n";
}

} // namespace

CapabilityGraph analyze_capabilities(const std::filesystem::path& input) {
  if (!std::filesystem::exists(input)) throw std::runtime_error("compatibility input does not exist: " + input.string());
  CapabilityGraph graph;
  auto scan = [&](const std::filesystem::path& path) {
    if (!is_js_file(path) && !is_html_file(path)) return;
    ++graph.files_scanned;
    const auto source = read_file(path);
    if (is_js_file(path)) {
      ++graph.javascript_files;
      analyze_tokens(graph, path, lex_javascript(source));
    } else {
      ++graph.html_files;
      for (const auto& [script, line] : extract_inline_scripts(source)) analyze_tokens(graph, path, lex_javascript(script, line));
      std::size_t pos = 0;
      while ((pos = source.find("modulepreload", pos)) != std::string::npos) {
        std::size_t line = 1, column = 1;
        for (std::size_t i = 0; i < pos; ++i) { if (source[i] == '\n') { ++line; column = 1; } else ++column; }
        ++graph.module_features["modulepreload"];
        graph.occurrences.push_back({path, line, column, "module", "modulepreload", {}});
        pos += 13;
      }
    }
  };
  if (std::filesystem::is_regular_file(input)) scan(input);
  else {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(input)) if (entry.is_regular_file()) scan(entry.path());
  }
  return graph;
}

std::string capability_graph_json(const CapabilityGraph& graph) {
  std::ostringstream out;
  out << "{\"schema_version\":1,\"files_scanned\":" << graph.files_scanned
      << ",\"javascript_files\":" << graph.javascript_files
      << ",\"html_files\":" << graph.html_files << ",\"syntax\":";
  write_map_json(out, graph.syntax);
  out << ",\"browser_apis\":"; write_map_json(out, graph.browser_apis);
  out << ",\"node_apis\":"; write_map_json(out, graph.node_apis);
  out << ",\"frameworks\":"; write_map_json(out, graph.frameworks);
  out << ",\"module_features\":"; write_map_json(out, graph.module_features);
  out << ",\"occurrences\":[";
  for (std::size_t i = 0; i < graph.occurrences.size(); ++i) {
    if (i) out << ',';
    const auto& x = graph.occurrences[i];
    out << "{\"file\":\"" << escape_json(x.file.generic_string()) << "\",\"line\":" << x.line
        << ",\"column\":" << x.column << ",\"kind\":\"" << escape_json(x.kind)
        << "\",\"name\":\"" << escape_json(x.name) << "\"";
    if (!x.detail.empty()) out << ",\"detail\":\"" << escape_json(x.detail) << "\"";
    out << '}';
  }
  out << "]}";
  return out.str();
}

void print_capability_graph_text(const CapabilityGraph& graph) {
  std::cout << "Venom application capability graph\n\n"
            << "Files scanned: " << graph.files_scanned << "\n"
            << "JavaScript files: " << graph.javascript_files << "\n"
            << "HTML files: " << graph.html_files << "\n\n";
  print_map("Syntax", graph.syntax);
  print_map("Modules", graph.module_features);
  print_map("Browser APIs", graph.browser_apis);
  print_map("Node APIs", graph.node_apis);
  print_map("Framework signatures", graph.frameworks);
}

} // namespace venom::compiler
