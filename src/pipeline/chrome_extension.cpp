#include "venom/base/error.hpp"
#include "venom/internal/pipeline/chrome_extension.hpp"
#include "venom/internal/pipeline/build_support.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace venom::compiler::chrome_extension {
namespace {

struct JsonValue {
  enum class Kind { Null, Boolean, Number, String, Array, Object } kind = Kind::Null;
  bool boolean = false;
  double number = 0.0;
  std::string string;
  std::vector<JsonValue> array;
  std::map<std::string, JsonValue> object;
};

class JsonParser {
 public:
  explicit JsonParser(std::string_view input) : input_(input) {}

  JsonValue parse() {
    skip_space();
    auto value = parse_value();
    skip_space();
    if (position_ != input_.size()) fail("trailing data");
    return value;
  }

 private:
  std::string_view input_;
  std::size_t position_ = 0;

  [[noreturn]] void fail(const std::string& reason) const {
    raise_error("VENOM-E5000", "invalid Chrome extension manifest JSON at byte " +
                             std::to_string(position_) + ": " + reason);
  }

  void skip_space() {
    while (position_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[position_]))) ++position_;
  }

  bool consume(char value) {
    skip_space();
    if (position_ < input_.size() && input_[position_] == value) { ++position_; return true; }
    return false;
  }

  JsonValue parse_value() {
    skip_space();
    if (position_ >= input_.size()) fail("unexpected end of input");
    const char c = input_[position_];
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '"') { JsonValue v; v.kind = JsonValue::Kind::String; v.string = parse_string(); return v; }
    if (c == 't' && input_.substr(position_, 4) == "true") { position_ += 4; JsonValue v; v.kind = JsonValue::Kind::Boolean; v.boolean = true; return v; }
    if (c == 'f' && input_.substr(position_, 5) == "false") { position_ += 5; JsonValue v; v.kind = JsonValue::Kind::Boolean; return v; }
    if (c == 'n' && input_.substr(position_, 4) == "null") { position_ += 4; return {}; }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
    fail("unexpected token");
  }

  JsonValue parse_object() {
    JsonValue value; value.kind = JsonValue::Kind::Object;
    if (!consume('{')) fail("expected object");
    skip_space();
    if (consume('}')) return value;
    while (true) {
      skip_space();
      if (position_ >= input_.size() || input_[position_] != '"') fail("expected object key");
      auto key = parse_string();
      if (!consume(':')) fail("expected ':'");
      if (!value.object.emplace(std::move(key), parse_value()).second) fail("duplicate object key");
      if (consume('}')) break;
      if (!consume(',')) fail("expected ',' or '}'");
    }
    return value;
  }

  JsonValue parse_array() {
    JsonValue value; value.kind = JsonValue::Kind::Array;
    if (!consume('[')) fail("expected array");
    skip_space();
    if (consume(']')) return value;
    while (true) {
      value.array.push_back(parse_value());
      if (consume(']')) break;
      if (!consume(',')) fail("expected ',' or ']'");
    }
    return value;
  }

  std::string parse_string() {
    if (position_ >= input_.size() || input_[position_++] != '"') fail("expected string");
    std::string output;
    while (position_ < input_.size()) {
      const unsigned char c = static_cast<unsigned char>(input_[position_++]);
      if (c == '"') return output;
      if (c < 0x20) fail("control character in string");
      if (c != '\\') { output.push_back(static_cast<char>(c)); continue; }
      if (position_ >= input_.size()) fail("unterminated escape");
      const char e = input_[position_++];
      switch (e) {
        case '"': output.push_back('"'); break; case '\\': output.push_back('\\'); break;
        case '/': output.push_back('/'); break; case 'b': output.push_back('\b'); break;
        case 'f': output.push_back('\f'); break; case 'n': output.push_back('\n'); break;
        case 'r': output.push_back('\r'); break; case 't': output.push_back('\t'); break;
        case 'u': {
          if (position_ + 4 > input_.size()) fail("short unicode escape");
          unsigned code = 0;
          for (int i = 0; i < 4; ++i) {
            const char h = input_[position_++];
            code <<= 4;
            if (h >= '0' && h <= '9') code += static_cast<unsigned>(h - '0');
            else if (h >= 'a' && h <= 'f') code += static_cast<unsigned>(h - 'a' + 10);
            else if (h >= 'A' && h <= 'F') code += static_cast<unsigned>(h - 'A' + 10);
            else fail("invalid unicode escape");
          }
          if (code <= 0x7f) output.push_back(static_cast<char>(code));
          else if (code <= 0x7ff) { output.push_back(static_cast<char>(0xc0 | (code >> 6))); output.push_back(static_cast<char>(0x80 | (code & 0x3f))); }
          else { output.push_back(static_cast<char>(0xe0 | (code >> 12))); output.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f))); output.push_back(static_cast<char>(0x80 | (code & 0x3f))); }
          break;
        }
        default: fail("invalid string escape");
      }
    }
    fail("unterminated string");
  }

  JsonValue parse_number() {
    const auto begin = position_;
    if (input_[position_] == '-') ++position_;
    while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
    if (position_ < input_.size() && input_[position_] == '.') { ++position_; while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_; }
    if (position_ < input_.size() && (input_[position_] == 'e' || input_[position_] == 'E')) {
      ++position_; if (position_ < input_.size() && (input_[position_] == '+' || input_[position_] == '-')) ++position_;
      while (position_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[position_]))) ++position_;
    }
    JsonValue value; value.kind = JsonValue::Kind::Number;
    try { value.number = std::stod(std::string(input_.substr(begin, position_ - begin))); }
    catch (...) { fail("invalid number"); }
    return value;
  }
};

std::string text(const SiteFile& file) {
  return std::string(reinterpret_cast<const char*>(file.bytes.data()), file.bytes.size());
}

const SiteFile* find(const SiteGraph& graph, const std::string& relative) {
  for (const auto& file : graph.files) if (file.relative == relative) return &file;
  return nullptr;
}

const JsonValue* member(const JsonValue& value, const std::string& name) {
  if (value.kind != JsonValue::Kind::Object) return nullptr;
  const auto it = value.object.find(name);
  return it == value.object.end() ? nullptr : &it->second;
}

std::string string_member(const JsonValue& value, const std::string& name) {
  const auto* found = member(value, name);
  return found && found->kind == JsonValue::Kind::String ? found->string : std::string{};
}

std::string normalize_resource_path(std::string path) {
  std::replace(path.begin(), path.end(), '\\', '/');
  while (path.rfind("./", 0) == 0) path.erase(0, 2);
  if (path.empty() || path.front() == '/' || path.find(':') != std::string::npos ||
      path == ".." || path.rfind("../", 0) == 0 || path.find("/../") != std::string::npos) {
    raise_error("VENOM-E5000", "Chrome extension manifest contains unsafe resource path: " + path);
  }
  return path;
}

void add_reference(ManifestAnalysis& analysis, std::set<std::string>& seen, std::string path,
                   ExecutionContext context, std::string source, bool required = true) {
  if (path.empty()) return;
  path = normalize_resource_path(std::move(path));
  const std::string key = path + "\n" + execution_context_name(context);
  if (!seen.insert(key).second) return;
  analysis.resources.push_back({std::move(path), context, std::move(source), required});
}

void add_string_array(const JsonValue* value, std::vector<std::string>& output) {
  if (!value || value->kind != JsonValue::Kind::Array) return;
  for (const auto& entry : value->array) if (entry.kind == JsonValue::Kind::String) output.push_back(entry.string);
}

void add_icon_object(ManifestAnalysis& analysis, std::set<std::string>& seen, const JsonValue* value, const std::string& source) {
  if (!value || value->kind != JsonValue::Kind::Object) return;
  for (const auto& [_, entry] : value->object) if (entry.kind == JsonValue::Kind::String) add_reference(analysis, seen, entry.string, ExecutionContext::StaticResource, source);
}

bool wildcard_match(std::string_view pattern, std::string_view value) {
  std::size_t p = 0, v = 0, star = std::string_view::npos, match = 0;
  while (v < value.size()) {
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == value[v])) { ++p; ++v; continue; }
    if (p < pattern.size() && pattern[p] == '*') { star = p++; match = v; continue; }
    if (star != std::string_view::npos) { p = star + 1; v = ++match; continue; }
    return false;
  }
  while (p < pattern.size() && pattern[p] == '*') ++p;
  return p == pattern.size();
}

void add_wildcard_resources(const SiteGraph& graph, ManifestAnalysis& analysis, std::set<std::string>& seen,
                            const std::string& pattern, const std::string& source) {
  const auto normalized = normalize_resource_path(pattern);
  for (const auto& file : graph.files) {
    if (wildcard_match(normalized, file.relative)) {
      add_reference(analysis, seen, file.relative, ExecutionContext::StaticResource, source, false);
    }
  }
}

void scan_html_dependencies(const SiteGraph& graph, ManifestAnalysis& analysis, std::set<std::string>& seen) {
  static const std::regex attr(R"((?:src|href)\s*=\s*[\"']([^\"'#?]+)[\"'])", std::regex::icase);
  static const std::regex css_url(R"(url\(\s*[\"']?([^\"')?#]+))", std::regex::icase);
  static const std::regex js_import(R"((?:from\s*|import\s*\(\s*|import\s*)[\"']([^\"']+)[\"'])");
  static const std::regex runtime_url(R"((?:chrome\.)?runtime\.getURL\s*\(\s*[\"']([^\"']+)[\"'])");
  static const std::regex offscreen_url(R"(offscreen\.createDocument\s*\(\s*\{[^}]*url\s*:\s*[\"']([^\"']+)[\"'])", std::regex::icase);
  static const std::regex script_file(R"((?:files|js)\s*:\s*\[([^\]]+)\])", std::regex::icase);
  static const std::regex quoted(R"([\"']([^\"']+\.(?:js|mjs|css|html|json|wasm|png|jpg|jpeg|svg|gif|webp|ico|txt))[\"'])", std::regex::icase);

  auto directory = [](const std::string& path) {
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string{} : path.substr(0, slash + 1);
  };
  auto resolve = [&](const std::string& owner, const std::string& ref) {
    if (ref.empty() || ref[0] == '#' || ref.find("://") != std::string::npos ||
        ref.rfind("data:", 0) == 0 || ref.rfind("node:", 0) == 0 ||
        ref.front() == '@') return std::string{};
    std::filesystem::path combined = ref.front() == '/' ? std::filesystem::path(ref.substr(1)) : std::filesystem::path(directory(owner)) / ref;
    const auto normalized = combined.lexically_normal().generic_string();
    if (normalized == ".." || normalized.rfind("../", 0) == 0) return std::string{};
    return normalized;
  };

  // Traverse from manifest entrypoints only. This avoids pulling tests, build
  // scripts, and package tooling into the extension merely because they exist
  // under the project directory.
  std::size_t cursor = 0;
  while (cursor < analysis.resources.size()) {
    const auto owner_reference = analysis.resources[cursor++];
    const auto* file = find(graph, owner_reference.path);
    if (!file) continue;
    if (file->extension != ".html" && file->extension != ".htm" && file->extension != ".css" &&
        file->extension != ".js" && file->extension != ".mjs") continue;
    const auto source = text(*file);
    // JavaScript module imports execute in the same Chrome world as their
    // entrypoint. HTML-linked resources inherit the page context; CSS URL
    // dependencies remain static resources.
    const auto dependency_context = file->extension == ".css"
        ? ExecutionContext::StaticResource : owner_reference.context;
    const auto& pattern = file->extension == ".css" ? css_url :
        (file->extension == ".html" || file->extension == ".htm" ? attr : js_import);
    for (auto it = std::sregex_iterator(source.begin(), source.end(), pattern); it != std::sregex_iterator(); ++it) {
      const auto resolved = resolve(file->relative, (*it)[1].str());
      if (!resolved.empty()) add_reference(analysis, seen, resolved, dependency_context, file->relative, false);
    }
    if (file->extension == ".js" || file->extension == ".mjs") {
      for (auto it = std::sregex_iterator(source.begin(), source.end(), runtime_url); it != std::sregex_iterator(); ++it) {
        const auto resolved = resolve(file->relative, (*it)[1].str());
        if (!resolved.empty()) add_reference(analysis, seen, resolved, ExecutionContext::StaticResource, file->relative, false);
      }
      for (auto it = std::sregex_iterator(source.begin(), source.end(), offscreen_url); it != std::sregex_iterator(); ++it) {
        const auto resolved = resolve(file->relative, (*it)[1].str());
        if (!resolved.empty()) add_reference(analysis, seen, resolved, ExecutionContext::ExtensionPage, file->relative, false);
      }
      for (auto it = std::sregex_iterator(source.begin(), source.end(), script_file); it != std::sregex_iterator(); ++it) {
        const auto list = (*it)[1].str();
        for (auto jt = std::sregex_iterator(list.begin(), list.end(), quoted); jt != std::sregex_iterator(); ++jt) {
          const auto resolved = resolve(file->relative, (*jt)[1].str());
          if (!resolved.empty()) add_reference(analysis, seen, resolved, ExecutionContext::ContentScriptIsolated, file->relative, false);
        }
      }
      if (source.find("import(") != std::string::npos && !std::regex_search(source, js_import)) {
        analysis.warnings.push_back(file->relative + ": dynamic import could not be resolved statically");
      }
      if (source.find("executeScript") != std::string::npos && !std::regex_search(source, script_file)) {
        analysis.warnings.push_back(file->relative + ": chrome.scripting.executeScript files could not be resolved statically");
      }
      if (source.find("registerContentScripts") != std::string::npos && !std::regex_search(source, script_file)) {
        analysis.warnings.push_back(file->relative + ": chrome.scripting.registerContentScripts files could not be resolved statically");
      }
    }
  }
}

ManifestAnalysis analyze_manifest(const SiteGraph& graph, const JsonValue& root) {
  if (root.kind != JsonValue::Kind::Object) raise_error("VENOM-E5000", "Chrome extension manifest must be a JSON object");
  ManifestAnalysis analysis;
  std::set<std::string> seen;
  if (const auto* version = member(root, "manifest_version"); version && version->kind == JsonValue::Kind::Number) analysis.manifest_version = static_cast<int>(version->number);
  analysis.minimum_chrome_version = string_member(root, "minimum_chrome_version");
  add_string_array(member(root, "permissions"), analysis.permissions);
  add_string_array(member(root, "host_permissions"), analysis.host_permissions);
  add_icon_object(analysis, seen, member(root, "icons"), "manifest.icons");
  const auto default_locale = string_member(root, "default_locale");
  if (!default_locale.empty()) {
    const auto prefix = std::string("_locales/");
    for (const auto& file : graph.files) {
      if (file.relative.rfind(prefix, 0) == 0 && file.relative.size() > prefix.size()) {
        add_reference(analysis, seen, file.relative, ExecutionContext::StaticResource, "manifest.default_locale", false);
      }
    }
    add_reference(analysis, seen, "_locales/" + default_locale + "/messages.json", ExecutionContext::StaticResource, "manifest.default_locale", true);
  }

  if (const auto* action = member(root, "action")) {
    add_reference(analysis, seen, string_member(*action, "default_popup"), ExecutionContext::ExtensionPage, "manifest.action.default_popup");
    add_icon_object(analysis, seen, member(*action, "default_icon"), "manifest.action.default_icon");
  }
  if (const auto* background = member(root, "background")) {
    analysis.background_service_worker = string_member(*background, "service_worker");
    analysis.background_is_module = string_member(*background, "type") == "module";
    add_reference(analysis, seen, analysis.background_service_worker, ExecutionContext::ServiceWorker, "manifest.background.service_worker");
  }
  add_reference(analysis, seen, string_member(root, "options_page"), ExecutionContext::ExtensionPage, "manifest.options_page");
  if (const auto* options = member(root, "options_ui")) add_reference(analysis, seen, string_member(*options, "page"), ExecutionContext::ExtensionPage, "manifest.options_ui.page");
  if (const auto* side = member(root, "side_panel")) add_reference(analysis, seen, string_member(*side, "default_path"), ExecutionContext::ExtensionPage, "manifest.side_panel.default_path");
  add_reference(analysis, seen, string_member(root, "devtools_page"), ExecutionContext::DevToolsPage, "manifest.devtools_page");
  if (const auto* overrides = member(root, "chrome_url_overrides"); overrides && overrides->kind == JsonValue::Kind::Object) for (const auto& [name, value] : overrides->object) if (value.kind == JsonValue::Kind::String) add_reference(analysis, seen, value.string, ExecutionContext::ExtensionPage, "manifest.chrome_url_overrides." + name);

  if (const auto* scripts = member(root, "content_scripts"); scripts && scripts->kind == JsonValue::Kind::Array) {
    for (std::size_t i = 0; i < scripts->array.size(); ++i) {
      const auto& entry = scripts->array[i];
      if (entry.kind != JsonValue::Kind::Object) continue;
      const auto world = string_member(entry, "world");
      const auto context = world == "MAIN" ? ExecutionContext::ContentScriptMain : ExecutionContext::ContentScriptIsolated;
      if (context == ExecutionContext::ContentScriptMain) analysis.has_main_world_script = true;
      else analysis.has_isolated_content_script = true;
      if (const auto* js = member(entry, "js"); js && js->kind == JsonValue::Kind::Array) for (const auto& path : js->array) if (path.kind == JsonValue::Kind::String) add_reference(analysis, seen, path.string, context, "manifest.content_scripts[" + std::to_string(i) + "].js");
      if (const auto* css = member(entry, "css"); css && css->kind == JsonValue::Kind::Array) for (const auto& path : css->array) if (path.kind == JsonValue::Kind::String) add_reference(analysis, seen, path.string, ExecutionContext::StaticResource, "manifest.content_scripts[" + std::to_string(i) + "].css");
    }
  }
  if (const auto* sandbox = member(root, "sandbox")) if (const auto* pages = member(*sandbox, "pages"); pages && pages->kind == JsonValue::Kind::Array) for (const auto& page : pages->array) if (page.kind == JsonValue::Kind::String) add_reference(analysis, seen, page.string, ExecutionContext::SandboxPage, "manifest.sandbox.pages");
  if (const auto* rules = member(root, "declarative_net_request")) if (const auto* resources = member(*rules, "rule_resources"); resources && resources->kind == JsonValue::Kind::Array) for (const auto& resource : resources->array) if (resource.kind == JsonValue::Kind::Object) add_reference(analysis, seen, string_member(resource, "path"), ExecutionContext::StaticResource, "manifest.declarative_net_request.rule_resources");
  if (const auto* exposed = member(root, "web_accessible_resources"); exposed && exposed->kind == JsonValue::Kind::Array) {
    for (const auto& group : exposed->array) {
      if (group.kind != JsonValue::Kind::Object) continue;
      const auto* resources = member(group, "resources");
      if (!resources || resources->kind != JsonValue::Kind::Array) continue;
      for (const auto& resource : resources->array) {
        if (resource.kind != JsonValue::Kind::String) continue;
        if (resource.string.find('*') != std::string::npos || resource.string.find('?') != std::string::npos) {
          add_wildcard_resources(graph, analysis, seen, resource.string, "manifest.web_accessible_resources");
        } else {
          add_reference(analysis, seen, resource.string, ExecutionContext::StaticResource, "manifest.web_accessible_resources", false);
        }
      }
    }
  }

  analysis.has_offscreen_permission = std::find(analysis.permissions.begin(), analysis.permissions.end(), "offscreen") != analysis.permissions.end();
  analysis.has_extension_page = std::any_of(analysis.resources.begin(), analysis.resources.end(), [](const ResourceReference& resource) {
    return resource.context == ExecutionContext::ExtensionPage || resource.context == ExecutionContext::DevToolsPage;
  });
  // Prefer an offscreen document because it provides a DOM-capable isolated
  // extension origin without coupling protected execution to popup lifetime.
  // Fall back to an extension page, then a worker-safe service worker host.
  if (analysis.has_offscreen_permission) analysis.runtime_host = RuntimeHost::OffscreenDocument;
  else if (analysis.has_extension_page) analysis.runtime_host = RuntimeHost::ExtensionPage;
  else if (!analysis.background_service_worker.empty()) analysis.runtime_host = RuntimeHost::ServiceWorker;
  else analysis.runtime_host = RuntimeHost::None;

  if (analysis.runtime_host == RuntimeHost::OffscreenDocument) {
    add_reference(analysis, seen, "engine-host.js", ExecutionContext::ExtensionPage, "venom.offscreen-host", true);
  }
  scan_html_dependencies(graph, analysis, seen);
  std::sort(analysis.resources.begin(), analysis.resources.end(), [](const ResourceReference& a, const ResourceReference& b) { return std::tie(a.path, a.context, a.source) < std::tie(b.path, b.context, b.source); });
  return analysis;
}

void write_bytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) raise_error("VENOM-E5000", "failed to write Chrome extension file: " + path.string());
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!out) raise_error("VENOM-E5000", "failed to finish Chrome extension file: " + path.string());
}

std::string patch_manifest(std::string manifest) {
  static const std::regex csp_object(R"("content_security_policy"\s*:\s*\{[^}]*\})");
  static const std::regex final_brace(R"(\}\s*$)");
  const std::string csp = "\"content_security_policy\": {\"extension_pages\": \"script-src 'self' 'wasm-unsafe-eval'; object-src 'self'\"}";
  if (std::regex_search(manifest, csp_object)) manifest = std::regex_replace(manifest, csp_object, csp, std::regex_constants::format_first_only);
  else {
    auto match = std::sregex_iterator(manifest.begin(), manifest.end(), final_brace);
    if (match == std::sregex_iterator()) raise_error("VENOM-E5000", "Chrome extension manifest is not a JSON object");
    const auto pos = static_cast<std::size_t>(match->position());
    std::size_t cursor = pos;
    while (cursor > 0 && std::isspace(static_cast<unsigned char>(manifest[cursor - 1]))) --cursor;
    const bool empty = cursor > 0 && manifest[cursor - 1] == '{';
    manifest.insert(cursor, std::string(empty ? "" : ",") + "\n  " + csp + "\n");
  }
  return manifest;
}


constexpr std::string_view k_extension_shell_dir = "assets/extension";

std::string shell_path(const std::string& relative) {
  return std::string(k_extension_shell_dir) + "/" + relative;
}

void replace_all(std::string& value, const std::string& from, const std::string& to) {
  if (from.empty() || from == to) return;
  std::size_t pos = 0;
  while ((pos = value.find(from, pos)) != std::string::npos) {
    value.replace(pos, from.size(), to);
    pos += to.size();
  }
}

std::string relocate_manifest_paths(std::string manifest, const ManifestAnalysis& analysis) {
  std::set<std::string> paths;
  for (const auto& resource : analysis.resources) {
    if (!resource.path.empty() && resource.path != "manifest.json") paths.insert(resource.path);
  }
  for (const auto& path : paths) {
    replace_all(manifest, "\"" + path + "\"", "\"" + shell_path(path) + "\"");
  }
  return manifest;
}

bool is_text_extension_file(const std::string& relative) {
  const auto ext = std::filesystem::path(relative).extension().string();
  return ext == ".js" || ext == ".mjs" || ext == ".cjs" || ext == ".json" ||
         ext == ".html" || ext == ".css" || ext == ".txt" || ext == ".svg";
}

std::string relocate_chrome_api_literals(std::string source, const ManifestAnalysis& analysis) {
  // All authored extension files move together under extension/, so ordinary
  // relative imports and HTML/CSS references remain valid. Only Chrome APIs
  // that resolve paths from the extension root need their literal paths
  // prefixed. Exact resource literals are safe to rewrite in JavaScript files.
  std::set<std::string> paths;
  for (const auto& resource : analysis.resources) {
    if (!resource.path.empty() && resource.path != "manifest.json") paths.insert(resource.path);
  }
  paths.insert("engine.html");
  for (const auto& path : paths) {
    replace_all(source, "'" + path + "'", "'" + shell_path(path) + "'");
    replace_all(source, "\"" + path + "\"", "\"" + shell_path(path) + "\"");
  }
  return source;
}

std::string relocate_engine_asset_urls(std::string html) {
  replace_all(html, "src=\"assets/", "src=\"../");
  replace_all(html, "href=\"assets/", "href=\"../");
  replace_all(html, "src='assets/", "src='../");
  replace_all(html, "href='assets/", "href='../");
  return html;
}

bool passthrough(const SiteFile& file, const ManifestAnalysis& analysis) {
  if (file.relative == "manifest.json" || file.relative == "index.html" || file.relative == "protected/velocity-engine.js") return false;
  static const std::set<std::string> standard_shell = {"engine-host.js", "engine.html"};
  if (standard_shell.count(file.relative)) return true;
  return std::any_of(analysis.resources.begin(), analysis.resources.end(), [&](const ResourceReference& resource) { return resource.path == file.relative; });
}

std::string rpc_client_js(const RpcPolicy& policy) {
  std::ostringstream out;
  out << "// Generated by Venom. Do not edit.\n"
      << "(()=>{'use strict';const PROTOCOL='" << policy.protocol << "';"
      << "let nextId=1;function errorText(e){return e instanceof Error?e.message:String(e||'Venom extension RPC failed');}"
      << "async function call(name,payload,options={}){if(typeof name!=='string'||!/^[A-Za-z_$][A-Za-z0-9_$]{0,127}$/.test(name))throw new Error('Invalid protected export name');"
      << "const timeout=Math.max(1,Math.min(" << policy.max_timeout_ms << ",Number(options.timeout||" << policy.default_timeout_ms << ")));"
      << "let encoded='';try{encoded=JSON.stringify(payload===undefined?null:payload);}catch(e){throw new Error('RPC payload is not serializable: '+errorText(e));}"
      << "if(encoded.length>" << policy.max_payload_bytes << ")throw new Error('RPC payload exceeds " << policy.max_payload_bytes << " bytes');"
      << "const requestId=(Date.now().toString(36)+'-'+(nextId++).toString(36));"
      << "const response=await chrome.runtime.sendMessage({protocol:PROTOCOL,type:'CALL',requestId,exportName:name,payload,timeout});"
      << "if(!response||response.ok!==true)throw new Error(response&&response.error?response.error:'Protected runtime returned no result');return response.result;}"
      << "globalThis.VenomExtensionRPC=Object.freeze({protocol:PROTOCOL,call});})();\n";
  return out.str();
}

std::string rpc_host_js(const RpcPolicy& policy) {
  std::ostringstream out;
  out << "// Generated by Venom. Do not edit.\n"
      << "'use strict';const PROTOCOL='" << policy.protocol << "';const MAX_INFLIGHT=" << policy.max_inflight << ";const MAX_PAYLOAD=" << policy.max_payload_bytes << ";let inflight=0;"
      << "function err(e){return e instanceof Error?e.message:String(e||'Protected runtime failure');}"
      << "chrome.runtime.onMessage.addListener((m,_s,send)=>{if(!m||m.protocol!==PROTOCOL)return false;if(m.type==='PING'){send({ok:true,protocol:PROTOCOL});return false;}if(m.type!=='HOST_CALL')return false;"
      << "(async()=>{if(inflight>=MAX_INFLIGHT)throw new Error('Protected runtime is busy');if(typeof m.exportName!=='string'||!/^[A-Za-z_$][A-Za-z0-9_$]{0,127}$/.test(m.exportName))throw new Error('Invalid protected export name');"
      << "let encoded='';try{encoded=JSON.stringify(m.payload===undefined?null:m.payload);}catch(e){throw new Error('RPC payload is not serializable');}if(encoded.length>MAX_PAYLOAD)throw new Error('RPC payload is too large');"
      << "inflight++;try{const api=globalThis.venom;if(!api||typeof api.ready!=='function'||!api.exports)throw new Error('Venom protected runtime is unavailable');await api.ready();const fn=api.exports[m.exportName];if(typeof fn!=='function')throw new Error('Protected export is unavailable: '+m.exportName);return await fn(m.payload,{timeout:m.timeout});}finally{inflight--;}})().then(r=>send({ok:true,result:r,requestId:m.requestId}),e=>send({ok:false,error:err(e),requestId:m.requestId}));return true;});\n";
  return out.str();
}

std::string rpc_broker_js(const RpcPolicy& policy) {
  std::ostringstream out;
  out << "// Generated by Venom. Do not edit.\n"
      << "'use strict';const PROTOCOL='" << policy.protocol << "';const HOST_URL='assets/extension/engine.html';let creating=null;let generation=0;"
      << "async function contexts(){if(typeof chrome.runtime.getContexts!=='function')return [];return chrome.runtime.getContexts({contextTypes:['OFFSCREEN_DOCUMENT'],documentUrls:[chrome.runtime.getURL(HOST_URL)]});}"
      << "async function ping(){const r=await chrome.runtime.sendMessage({protocol:PROTOCOL,type:'PING'});return !!(r&&r.ok&&r.protocol===PROTOCOL);}"
      << "async function ensure(){if((await contexts()).length===0){if(!creating){creating=chrome.offscreen.createDocument({url:HOST_URL,reasons:['WORKERS'],justification:'Host the local Venom protected QuickJS/WASM runtime.'}).catch(e=>{const t=e instanceof Error?e.message:String(e);if(!/already exists|single offscreen/i.test(t))throw e;}).finally(()=>{creating=null;});}await creating;}"
      << "let last;for(let i=0;i<40;i++){try{if(await ping())return;}catch(e){last=e;}await new Promise(r=>setTimeout(r,50));}throw new Error('Protected runtime host did not initialize'+(last?': '+(last.message||last):''));}"
      << "async function forward(m){await ensure();let r;try{r=await chrome.runtime.sendMessage({protocol:PROTOCOL,type:'HOST_CALL',requestId:m.requestId,exportName:m.exportName,payload:m.payload,timeout:m.timeout,generation});}catch(e){generation++;await ensure();r=await chrome.runtime.sendMessage({protocol:PROTOCOL,type:'HOST_CALL',requestId:m.requestId,exportName:m.exportName,payload:m.payload,timeout:m.timeout,generation});}if(!r||r.ok!==true)throw new Error(r&&r.error?r.error:'Protected runtime host returned no result');return r;}"
      << "chrome.runtime.onMessage.addListener((m,_s,send)=>{if(!m||m.protocol!==PROTOCOL||m.type!=='CALL')return false;forward(m).then(r=>send(r),e=>send({ok:false,error:e instanceof Error?e.message:String(e),requestId:m.requestId}));return true;});\n";
  return out.str();
}

std::string background_wrapper_js(const ManifestAnalysis& analysis) {
  if (analysis.background_service_worker.empty()) return {};
  std::ostringstream out;
  out << "// Generated by Venom. Do not edit.\n";
  if (analysis.background_is_module) {
    out << "import './" << analysis.background_service_worker << "';\nimport './venom-extension-broker.js';\n";
  } else {
    out << "importScripts('" << analysis.background_service_worker << "','venom-extension-broker.js');\n";
  }
  return out.str();
}

std::string patch_background_worker(std::string manifest, const ManifestAnalysis& analysis) {
  if (analysis.background_service_worker.empty() || analysis.runtime_host != RuntimeHost::OffscreenDocument) return manifest;
  const std::regex worker(R"((\"service_worker\"\s*:\s*\")[^\"]+(\"))");
  if (!std::regex_search(manifest, worker)) raise_error("VENOM-E5000", "Chrome extension background service worker could not be patched for Venom RPC");
  return std::regex_replace(manifest, worker, "$1assets/extension/venom-background.js$2", std::regex_constants::format_first_only);
}

bool extension_module_file(const std::string& relative, const ManifestAnalysis& analysis) {
  const auto ext = std::filesystem::path(relative).extension().string();
  if (ext == ".mjs") return true;
  if (relative == "engine-host.js") return true;
  if (relative == "venom-background.js") return analysis.background_is_module;
  return analysis.background_is_module && relative == analysis.background_service_worker;
}

bool extension_worker_file(const std::string& relative, const ManifestAnalysis& analysis) {
  return relative == analysis.background_service_worker || relative == "venom-background.js" ||
         relative == "venom-extension-broker.js";
}

std::string harden_extension_js(const std::string& relative, std::string source,
                                const ManifestAnalysis& analysis,
                                const std::string& build_salt) {
  const std::string kind = extension_module_file(relative, analysis)
      ? "extension-module"
      : (extension_worker_file(relative, analysis) ? "extension-worker" : "extension-script");
  try {
    auto hardened = build_detail::ast_harden_release_js(kind, source, build_salt);
    if (hardened.empty()) raise_error("VENOM-E5000", "produced empty output");
    return hardened;
  } catch (const std::exception& error) {
    raise_error("VENOM-E5000", "Chrome extension JavaScript hardening failed for " + relative + " (" + kind + "): " + error.what());
  }
}

void write_text_file(const std::filesystem::path& path, const std::string& value) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) raise_error("VENOM-E5000", "failed to write generated Chrome extension adapter: " + path.string());
  out.write(value.data(), static_cast<std::streamsize>(value.size()));
  if (!out) raise_error("VENOM-E5000", "failed to finish generated Chrome extension adapter: " + path.string());
}

std::string engine_host_html(std::string html) {
  constexpr std::string_view generic = "  <script src=\"venom-extension-host.js\"></script>\n";
  constexpr std::string_view custom = "  <script type=\"module\" src=\"engine-host.js\"></script>\n";
  if (html.find("venom-extension-host.js") == std::string::npos) {
    const auto body = html.rfind("</body>");
    if (body == std::string::npos) raise_error("VENOM-E5000", "generated Chrome extension engine page has no closing body tag");
    html.insert(body, generic);
  }
  if (html.find("engine-host.js") == std::string::npos) {
    const auto body = html.rfind("</body>");
    html.insert(body, custom);
  }
  return html;
}

} // namespace

const char* execution_context_name(ExecutionContext context) {
  switch (context) {
    case ExecutionContext::ExtensionPage: return "extension-page";
    case ExecutionContext::ServiceWorker: return "service-worker";
    case ExecutionContext::ContentScriptIsolated: return "content-script-isolated";
    case ExecutionContext::ContentScriptMain: return "content-script-main";
    case ExecutionContext::DevToolsPage: return "devtools-page";
    case ExecutionContext::SandboxPage: return "sandbox-page";
    case ExecutionContext::StaticResource: return "static-resource";
  }
  return "unknown";
}

const char* runtime_host_name(RuntimeHost host) {
  switch (host) {
    case RuntimeHost::None: return "none";
    case RuntimeHost::ExtensionPage: return "extension-page";
    case RuntimeHost::OffscreenDocument: return "offscreen-document";
    case RuntimeHost::ServiceWorker: return "service-worker";
  }
  return "unknown";
}

ContextPolicy context_policy(ExecutionContext context) {
  switch (context) {
    case ExecutionContext::ExtensionPage:
      return {context, true, true, true, false, true};
    case ExecutionContext::ServiceWorker:
      return {context, false, true, true, true, true};
    case ExecutionContext::ContentScriptIsolated:
      return {context, true, true, false, true, true};
    case ExecutionContext::ContentScriptMain:
      return {context, true, false, false, true, true};
    case ExecutionContext::DevToolsPage:
      return {context, true, true, true, false, true};
    case ExecutionContext::SandboxPage:
      return {context, true, false, false, true, true};
    case ExecutionContext::StaticResource:
      return {context, false, false, false, false, true};
  }
  return {};
}

std::string compatibility_summary(const ManifestAnalysis& analysis) {
  std::map<ExecutionContext, std::size_t> counts;
  for (const auto& resource : analysis.resources) ++counts[resource.context];
  std::ostringstream out;
  out << "Manifest V" << analysis.manifest_version
      << "; runtime host=" << runtime_host_name(analysis.runtime_host)
      << "; extension-pages=" << counts[ExecutionContext::ExtensionPage]
      << "; service-workers=" << counts[ExecutionContext::ServiceWorker]
      << "; isolated-content=" << counts[ExecutionContext::ContentScriptIsolated]
      << "; main-world=" << counts[ExecutionContext::ContentScriptMain]
      << "; static=" << counts[ExecutionContext::StaticResource]
      << "; rpc=" << analysis.rpc_policy.protocol
      << "; warnings=" << analysis.warnings.size();
  return out.str();
}

bool is_extension_target(const std::string& target) { return target == "chrome-extension"; }

ManifestAnalysis analyze_project(const SiteGraph& graph) {
  const auto* manifest_file = find(graph, "manifest.json");
  if (!manifest_file) raise_error("VENOM-E5000", "Chrome extension target requires manifest.json at the project root");
  return analyze_manifest(graph, JsonParser(text(*manifest_file)).parse());
}

void validate_project(const SiteGraph& graph) {
  const auto analysis = analyze_project(graph);
  if (analysis.manifest_version != 3) raise_error("VENOM-E5000", "Venom Chrome extension builds currently require Manifest V3");
  if ((analysis.runtime_host == RuntimeHost::ExtensionPage || analysis.runtime_host == RuntimeHost::OffscreenDocument) && graph.routes.empty()) {
    raise_error("VENOM-E5000", "selected Chrome extension runtime host requires at least one HTML extension page");
  }
  for (const auto& resource : analysis.resources) {
    if (!resource.required || resource.path == "engine.html") continue;
    if (!find(graph, resource.path)) raise_error("VENOM-E5000", "Chrome extension resource is missing: " + resource.path + " (referenced by " + resource.source + ", context=" + execution_context_name(resource.context) + ")");
  }
  if (analysis.runtime_host == RuntimeHost::None) {
    raise_error("VENOM-E5000", "Chrome extension has no compatible host for the protected QuickJS/WASM runtime; add an extension page, a service worker, or the offscreen permission");
  }
  if (analysis.runtime_host == RuntimeHost::OffscreenDocument) {
    if (analysis.minimum_chrome_version.empty()) {
      raise_error("VENOM-E5000", "offscreen protected runtime requires minimum_chrome_version 109 or newer");
    }
    try {
      if (std::stoi(analysis.minimum_chrome_version) < 109) raise_error("VENOM-E5000", "too old");
    } catch (...) {
      raise_error("VENOM-E5000", "offscreen protected runtime requires minimum_chrome_version 109 or newer");
    }
  }
  for (const auto& resource : analysis.resources) {
    const auto policy = context_policy(resource.context);
    if ((resource.context == ExecutionContext::ContentScriptMain || resource.context == ExecutionContext::SandboxPage) &&
        resource.path.rfind("protected/", 0) == 0) {
      raise_error("VENOM-E5000", "protected source cannot execute directly in " + std::string(execution_context_name(resource.context)) + ": " + resource.path + "; use a visible RPC adapter");
    }
    (void)policy;
  }
}

void emit_extension_files(const SiteGraph& graph, const std::filesystem::path& output) {
  const auto analysis = analyze_project(graph);
  const auto* manifest_file = find(graph, "manifest.json");
  if (!manifest_file) raise_error("VENOM-E5000", "Chrome extension manifest disappeared during build");
  const auto manifest_source = text(*manifest_file);
  const auto extension_build_salt = std::string{"chrome-extension-shell-v2|"} + manifest_source;
  auto patched = patch_manifest(manifest_source);
  patched = relocate_manifest_paths(std::move(patched), analysis);
  patched = patch_background_worker(std::move(patched), analysis);
  std::ofstream manifest_out(output / "manifest.json", std::ios::binary | std::ios::trunc);
  if (!manifest_out) raise_error("VENOM-E5000", "failed to write Chrome extension manifest");
  manifest_out << patched;
  if (!manifest_out) raise_error("VENOM-E5000", "failed to finish Chrome extension manifest");

  for (const auto& file : graph.files) {
    if (!passthrough(file, analysis)) continue;
    const auto destination = output / std::filesystem::path(shell_path(file.relative));
    if (is_text_extension_file(file.relative) && (std::filesystem::path(file.relative).extension() == ".js" || std::filesystem::path(file.relative).extension() == ".mjs" || std::filesystem::path(file.relative).extension() == ".cjs")) {
      const std::string source(reinterpret_cast<const char*>(file.bytes.data()), file.bytes.size());
      auto relocated = relocate_chrome_api_literals(source, analysis);
      write_text_file(destination, harden_extension_js(file.relative, std::move(relocated), analysis, extension_build_salt));
    } else {
      write_bytes(destination, file.bytes);
    }
  }

  if (analysis.runtime_host == RuntimeHost::OffscreenDocument) {
    write_text_file(output / std::filesystem::path(shell_path("venom-extension-rpc.js")), harden_extension_js("venom-extension-rpc.js", rpc_client_js(analysis.rpc_policy), analysis, extension_build_salt));
    write_text_file(output / std::filesystem::path(shell_path("venom-extension-host.js")), harden_extension_js("venom-extension-host.js", rpc_host_js(analysis.rpc_policy), analysis, extension_build_salt));
    write_text_file(output / std::filesystem::path(shell_path("venom-extension-broker.js")), harden_extension_js("venom-extension-broker.js", rpc_broker_js(analysis.rpc_policy), analysis, extension_build_salt));
    write_text_file(output / std::filesystem::path(shell_path("venom-background.js")), harden_extension_js("venom-background.js", background_wrapper_js(analysis), analysis, extension_build_salt));
  }

  const auto generated_index = output / "index.html";
  std::string html;
  {
    std::ifstream input(generated_index, std::ios::binary);
    if (!input) raise_error("VENOM-E5000", "Chrome extension build did not emit the protected engine route");
    html.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail()) raise_error("VENOM-E5000", "failed to read Chrome extension protected engine route");
  } // Close the input before deleting index.html; Windows denies removal of open files.

  const auto host = engine_host_html(relocate_engine_asset_urls(std::move(html)));
  {
    std::filesystem::create_directories((output / std::filesystem::path(shell_path("engine.html"))).parent_path());
    std::ofstream engine_out(output / std::filesystem::path(shell_path("engine.html")), std::ios::binary | std::ios::trunc);
    if (!engine_out) raise_error("VENOM-E5000", "failed to write Chrome extension offscreen engine page");
    engine_out.write(host.data(), static_cast<std::streamsize>(host.size()));
    if (!engine_out) raise_error("VENOM-E5000", "failed to finish Chrome extension offscreen engine page");
  }

  std::error_code remove_error;
  const bool removed = std::filesystem::remove(generated_index, remove_error);
  if (remove_error || !removed) {
    raise_error("VENOM-E5000", "failed to remove generated Chrome extension index.html: " +
                             (remove_error ? remove_error.message() : std::string{"file was not removed"}));
  }
}

} // namespace venom::compiler::chrome_extension
