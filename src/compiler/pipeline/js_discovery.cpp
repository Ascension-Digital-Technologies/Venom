#include "compiler/pipeline/js_discovery.hpp"
#include "compiler/core/site.hpp"
#include "compiler/commands/remote.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace venom::compiler::detail {
namespace {

std::string to_string(const std::vector<unsigned char>& bytes) {
  return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::string lower_ascii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}


struct ParsedRealmDirective {
  std::string realm;
  bool isolated = false;
  bool module = false;
  bool malformed = false;
};

ParsedRealmDirective parse_realm_directive(std::string text) {
  text = lower_ascii(std::move(text));
  const auto marker = text.find("@venom:");
  if (marker == std::string::npos) return {};
  text.erase(0, marker + 7u);
  if (const auto newline = text.find_first_of("\r\n"); newline != std::string::npos) text.resize(newline);
  while (!text.empty() && (std::isspace(static_cast<unsigned char>(text.front())) != 0 || text.front() == '*')) text.erase(text.begin());
  std::istringstream tokens(text);
  std::string head;
  tokens >> head;
  if (head != "browser" && head != "protected") {
    if (head.find("browser") != std::string::npos || head.find("protected") != std::string::npos) return {{}, false, false, true};
    return {};
  }
  ParsedRealmDirective result;
  result.realm = head;
  std::string modifier;
  while (tokens >> modifier) {
    while (!modifier.empty() && (modifier.back() == ',' || modifier.back() == ';')) modifier.pop_back();
    if (modifier == "isolated") result.isolated = true;
    else if (modifier == "module") result.module = true;
    else { result.malformed = true; break; }
  }
  if (result.realm == "browser" && (result.isolated || result.module)) result.malformed = true;
  if (result.isolated && result.module) result.malformed = true;
  return result;
}

std::string normalize_slashes(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  while (!value.empty() && value.front() == '/') value.erase(value.begin());
  while (value.rfind("./", 0) == 0) value.erase(0, 2);
  std::vector<std::string> parts;
  std::string current;
  std::stringstream input(value);
  while (std::getline(input, current, '/')) {
    if (current.empty() || current == ".") continue;
    if (current == "..") { if (!parts.empty()) parts.pop_back(); continue; }
    parts.push_back(current);
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < parts.size(); ++i) { if (i) out << '/'; out << parts[i]; }
  return out.str();
}

std::string dirname_of(std::string path) {
  path = normalize_slashes(std::move(path));
  const auto pos = path.find_last_of('/');
  return pos == std::string::npos ? std::string{} : path.substr(0, pos + 1);
}

bool is_name_char(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) != 0 || c == '-' || c == '_' || c == ':' || c == '.';
}

bool is_js_ident_start(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalpha(u) != 0 || c == '_' || c == '$';
}

bool is_js_ident_continue(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) != 0 || c == '_' || c == '$';
}

bool has_file_scope_directive(const std::string& source, std::string_view realm) {
  std::size_t i = 0;
  if (source.size() >= 3 && static_cast<unsigned char>(source[0]) == 0xefu &&
      static_cast<unsigned char>(source[1]) == 0xbbu && static_cast<unsigned char>(source[2]) == 0xbfu) i = 3;
  while (i < source.size()) {
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
    if (i + 1 >= source.size()) break;
    if (source[i] == '/' && source[i + 1] == '/') {
      const auto end = source.find('\n', i + 2);
      const auto parsed = parse_realm_directive(source.substr(i + 2, (end == std::string::npos ? source.size() : end) - (i + 2)));
      if (parsed.malformed) throw std::runtime_error("malformed file-scope Venom realm directive");
      if (parsed.realm == realm) return true;
      i = end == std::string::npos ? source.size() : end + 1;
      continue;
    }
    if (source[i] == '/' && source[i + 1] == '*') {
      const auto end = source.find("*/", i + 2);
      if (end == std::string::npos) break;
      const auto parsed = parse_realm_directive(source.substr(i + 2, end - (i + 2)));
      if (parsed.malformed) throw std::runtime_error("malformed file-scope Venom realm directive");
      if (parsed.realm == realm) return true;
      i = end + 2;
      continue;
    }
    break;
  }
  return false;
}

bool has_file_scope_venom_browser_directive(const std::string& source) { return has_file_scope_directive(source, "browser"); }
bool has_file_scope_venom_protected_directive(const std::string& source) { return has_file_scope_directive(source, "protected"); }

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

} // namespace

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

bool has_module_references(const std::string& source) {
  return !scan_module_references(source).empty();
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
      const bool file_browser_directive = has_file_scope_venom_browser_directive(chunk.code);
      const bool file_protected_directive = has_file_scope_venom_protected_directive(chunk.code);
      if (file_browser_directive && file_protected_directive) {
        throw std::runtime_error("conflicting file-scope Venom directives in " + chunk.source +
                                 "; exactly one of browser or protected may be selected");
      }
      if ((browser_attribute && file_protected_directive) ||
          (protected_attribute && file_browser_directive)) {
        throw std::runtime_error("conflicting HTML data-venom and file-scope Venom directives in " + chunk.source);
      }
      const bool explicit_browser = browser_attribute || file_browser_directive;
      const bool explicit_protected = protected_attribute || file_protected_directive;
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

  return chunks;
}


} // namespace venom::compiler::detail
