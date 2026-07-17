#include "compiler/services/runtime_modules.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace venom::compiler {
namespace {

bool has(const std::map<std::string, std::size_t>& values, const char* key) {
  const auto it = values.find(key);
  return it != values.end() && it->second != 0;
}

bool contains_inline_event_attribute(const SiteGraph& graph) {
  for (const auto& file : graph.files) {
    if (file.extension != ".html" && file.extension != ".htm") continue;
    std::string text(file.bytes.begin(), file.bytes.end());
    for (std::size_t i = 0; i + 3 < text.size(); ++i) {
      if (text[i] != 'o' && text[i] != 'O') continue;
      if (text[i + 1] != 'n' && text[i + 1] != 'N') continue;
      std::size_t j = i + 2;
      if (!std::isalpha(static_cast<unsigned char>(text[j]))) continue;
      while (j < text.size() && (std::isalnum(static_cast<unsigned char>(text[j])) || text[j] == '_' || text[j] == '-' || text[j] == ':')) ++j;
      while (j < text.size() && std::isspace(static_cast<unsigned char>(text[j]))) ++j;
      if (j < text.size() && text[j] == '=') return true;
    }
  }
  return false;
}

std::size_t find_function_open(const std::string& source, const std::string& name, std::size_t& start) {
  const std::string async_needle = "async function " + name + "(";
  const std::string needle = "function " + name + "(";
  start = source.find(async_needle);
  if (start == std::string::npos) start = source.find(needle);
  if (start == std::string::npos) throw std::runtime_error("runtime module specialization could not find function: " + name);
  const auto params = source.find('(', start);
  if (params == std::string::npos) throw std::runtime_error("runtime module specialization found malformed parameters: " + name);
  std::size_t depth = 0;
  char quote = 0;
  bool escape = false;
  for (std::size_t i = params; i < source.size(); ++i) {
    const char c = source[i];
    if (quote) {
      if (escape) { escape = false; continue; }
      if (c == '\\') { escape = true; continue; }
      if (c == quote) quote = 0;
      continue;
    }
    if (c == '\'' || c == '"' || c == '`') { quote = c; continue; }
    if (c == '(') ++depth;
    else if (c == ')') {
      if (depth == 0) throw std::runtime_error("runtime module specialization parameter underflow: " + name);
      --depth;
      if (depth == 0) {
        const auto open = source.find('{', i + 1);
        if (open == std::string::npos) throw std::runtime_error("runtime module specialization found malformed function body: " + name);
        return open;
      }
    }
  }
  throw std::runtime_error("runtime module specialization found unterminated parameters: " + name);
}

std::size_t find_matching_brace(const std::string& source, std::size_t open) {
  std::size_t depth = 0;
  char quote = 0;
  bool escape = false;
  bool line_comment = false;
  bool block_comment = false;
  for (std::size_t i = open; i < source.size(); ++i) {
    const char c = source[i];
    const char n = i + 1 < source.size() ? source[i + 1] : '\0';
    if (line_comment) { if (c == '\n') line_comment = false; continue; }
    if (block_comment) { if (c == '*' && n == '/') { block_comment = false; ++i; } continue; }
    if (quote) {
      if (escape) { escape = false; continue; }
      if (c == '\\') { escape = true; continue; }
      if (c == quote) quote = 0;
      continue;
    }
    if (c == '/' && n == '/') { line_comment = true; ++i; continue; }
    if (c == '/' && n == '*') { block_comment = true; ++i; continue; }
    if (c == '\'' || c == '"' || c == '`') { quote = c; continue; }
    if (c == '{') ++depth;
    else if (c == '}') {
      if (depth == 0) throw std::runtime_error("runtime module specialization brace underflow");
      --depth;
      if (depth == 0) return i;
    }
  }
  throw std::runtime_error("runtime module specialization found unterminated function");
}

void replace_function(std::string& source, const std::string& name, const std::string& replacement) {
  std::size_t start = 0;
  const auto open = find_function_open(source, name, start);
  const auto close = find_matching_brace(source, open);
  source.replace(start, close - start + 1, replacement);
}

std::string json_escape(const std::string& text) {
  std::string out;
  for (const char c : text) {
    if (c == '"' || c == '\\') out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

} // namespace

std::vector<std::string> RuntimeModulePlan::enabled_modules() const {
  std::vector<std::string> out{"core", "route", "dom", "quickjs"};
  if (network) out.emplace_back("network");
  if (timers) out.emplace_back("timers");
  if (events) out.emplace_back("events");
  if (storage) out.emplace_back("storage");
  if (crypto) out.emplace_back("crypto");
  if (navigation) out.emplace_back("navigation");
  if (forms) out.emplace_back("forms");
  if (observers) out.emplace_back("observers");
  if (animation) out.emplace_back("animation");
  return out;
}

RuntimeModulePlan plan_runtime_modules(const SiteGraph& graph, const CapabilityGraph& c, const BuildOptions& options) {
  RuntimeModulePlan p;
  p.network = has(c.browser_apis, "fetch") || has(c.browser_apis, "XMLHttpRequest") ||
      has(c.browser_apis, "WebSocket") || has(c.browser_apis, "EventSource");
  p.timers = has(c.browser_apis, "setTimeout") || has(c.browser_apis, "setInterval") ||
      has(c.browser_apis, "requestAnimationFrame");
  p.events = has(c.browser_apis, "addEventListener") || has(c.browser_apis, "dispatchEvent") ||
      contains_inline_event_attribute(graph);
  const bool needs_storage = has(c.browser_apis, "localStorage") || has(c.browser_apis, "sessionStorage") ||
      has(c.browser_apis, "indexedDB");
  const bool needs_crypto = has(c.browser_apis, "crypto.getRandomValues") || has(c.browser_apis, "crypto.subtle");
  auto apply_policy = [](const char* name, const std::string& mode, bool needed) {
    if (mode == "deny" && needed) throw std::runtime_error(std::string("capability '") + name + "' is required by the project but denied in venom.toml");
    if (mode == "allow" || mode == "read-only") return true;
    if (mode == "auto") return needed;
    return false;
  };
  p.network = apply_policy("fetch", options.capability_fetch, p.network);
  p.timers = apply_policy("timers", options.capability_timers, p.timers);
  p.storage = apply_policy("storage", options.capability_storage, needs_storage);
  p.crypto = apply_policy("crypto", options.capability_crypto, needs_crypto);
  p.navigation = has(c.browser_apis, "history") || has(c.browser_apis, "location") || graph.routes.size() > 1;
  p.forms = has(c.browser_apis, "FormData") || has(c.browser_apis, "SubmitEvent") ||
      has(c.browser_apis, "HTMLFormElement") || has(c.browser_apis, "form.submit") ||
      has(c.browser_apis, "form.requestSubmit") || has(c.browser_apis, "input.checked") ||
      has(c.browser_apis, "input.value") || has(c.browser_apis, "select.value");
  p.observers = has(c.browser_apis, "MutationObserver") || has(c.browser_apis, "ResizeObserver") ||
      has(c.browser_apis, "IntersectionObserver");
  p.animation = has(c.browser_apis, "requestAnimationFrame") || has(c.browser_apis, "cancelAnimationFrame");
  return p;
}

std::string specialize_runtime_modules(std::string source, const RuntimeModulePlan& plan) {
  if (!plan.network) {
    replace_function(source, "requestFetch", "async function requestFetch() { throw new Error('V-MOD-NET'); }");
  }
  if (!plan.timers) {
    replace_function(source, "scheduleTimer", "function scheduleTimer() { throw new Error('V-MOD-TIMER'); }");
    replace_function(source, "cancelTimer", "function cancelTimer(id) { return Object.freeze({ id: id >>> 0, cancelled: false }); }");
  }
  if (!plan.events) {
    replace_function(source, "createEventQueue", "function createEventQueue() { const empty = Object.freeze({ size: 0, maxRecords: 0, dispatches: 0, maxDispatchesPerRoute: 0, last: null }); return Object.freeze({ push: () => Object.freeze({ queued: false, size: 0 }), snapshot: () => empty, clear: () => 0 }); }");
    replace_function(source, "bindInlineEventAttribute", "function bindInlineEventAttribute() { return false; }");
  }
  const auto modules = plan.enabled_modules();
  std::ostringstream marker;
  marker << "\nconst __VENOM_RUNTIME_MODULES__ = Object.freeze([";
  for (std::size_t i = 0; i < modules.size(); ++i) {
    if (i) marker << ',';
    marker << '\"' << json_escape(modules[i]) << '\"';
  }
  marker << "]);\n";
  marker << "function __venomRuntimeModuleEnabled(name) { return __VENOM_RUNTIME_MODULES__.includes(String(name || '')); }\n";
  source.insert(0, marker.str());
  return source;
}

std::string runtime_module_plan_json(const RuntimeModulePlan& plan) {
  const auto modules = plan.enabled_modules();
  std::ostringstream out;
  out << '[';
  for (std::size_t i = 0; i < modules.size(); ++i) {
    if (i) out << ',';
    out << '\"' << json_escape(modules[i]) << '\"';
  }
  out << ']';
  return out.str();
}

} // namespace venom::compiler
