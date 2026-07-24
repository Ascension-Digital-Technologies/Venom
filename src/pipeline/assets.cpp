#include "base/error.hpp"
#include "pipeline/assets.hpp"

#include "package/hash.hpp"
#include "pipeline/js.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace venom::compiler {

namespace {
std::string normalize_slashes(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  while (!value.empty() && value.front() == '/') {
    value.erase(value.begin());
  }
  while (value.rfind("./", 0) == 0) {
    value.erase(0, 2);
  }
  return value;
}

std::string lower_ext(std::filesystem::path path) {
  auto ext = path.extension().generic_string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

bool is_deployment_metadata(const SiteFile& file) {
  const auto relative = normalize_slashes(file.relative);
  return relative == "venom.browser.json";
}

bool is_third_party_notice(const SiteFile& file) {
  auto relative = normalize_slashes(file.relative);
  std::transform(relative.begin(), relative.end(), relative.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  const auto has_suffix = [&](const std::string& suffix) {
    return relative.size() >= suffix.size() &&
           relative.compare(relative.size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  return relative == "third_party_notices.txt" ||
         relative == "third-party-notices.txt" ||
         has_suffix("/third_party_notices.txt") ||
         has_suffix("/third-party-notices.txt");
}

bool is_public_asset(const SiteFile& file, bool production, bool standard_layout_only) {
  // Chrome-extension distributions intentionally contain only Venom's generated
  // production assets. Extension-owned loose files are emitted separately by the
  // extension target using a strict runtime allowlist, so repository metadata and
  // arbitrary project files never become hashed assets.
  if (standard_layout_only) return false;
  if (production && (is_deployment_metadata(file) || is_third_party_notice(file))) return false;
  return
         file.extension != ".html" && file.extension != ".htm" &&
         file.extension != ".css" &&
         file.extension != ".js" && file.extension != ".mjs" && file.extension != ".cjs" &&
         file.extension != ".jsx" && file.extension != ".ts" && file.extension != ".tsx" &&
         file.extension != ".mts" && file.extension != ".cts";
}

std::string dirname_of(const std::string& relative) {
  const auto normalized = normalize_slashes(relative);
  const auto slash = normalized.find_last_of('/');
  return slash == std::string::npos ? std::string{} : normalized.substr(0, slash + 1u);
}

bool is_external_reference(const std::string& value) {
  std::string lower = value;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return lower.empty() || lower.front() == '#' || lower.rfind("data:", 0) == 0 ||
         lower.rfind("blob:", 0) == 0 || lower.rfind("http:", 0) == 0 ||
         lower.rfind("https:", 0) == 0 || lower.rfind("//", 0) == 0 ||
         lower.rfind("javascript:", 0) == 0;
}

std::string clean_reference(std::string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) value.erase(value.begin());
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) value.pop_back();
  const auto suffix = value.find_first_of("?#");
  if (suffix != std::string::npos) value.resize(suffix);
  return value;
}

void add_reference(std::set<std::string>& exact,
                   std::vector<std::string>& patterns,
                   const std::string& source,
                   std::string value) {
  value = clean_reference(std::move(value));
  if (is_external_reference(value)) return;
  while (!value.empty() && value.front() == '/') value.erase(value.begin());
  if (value.empty()) return;

  const auto add = [&](std::string candidate) {
    candidate = normalize_slashes(std::move(candidate));
    if (candidate.empty()) return;
    if (candidate.find('{') != std::string::npos && candidate.find('}') != std::string::npos) {
      patterns.push_back(std::move(candidate));
    } else {
      exact.insert(std::move(candidate));
    }
  };
  // Browser code commonly uses site-root-relative asset strings even when the
  // JavaScript file itself lives below the root. Preserve both interpretations;
  // only a path that actually exists in the collected site can be selected.
  add(value);
  add(dirname_of(source) + value);
}

void scan_reference_text(std::set<std::string>& exact,
                         std::vector<std::string>& patterns,
                         const std::string& source,
                         const std::string& text) {
  // Scan quoted strings without std::regex. Besides being faster for large
  // generated bundles, this avoids a libstdc++ regex crash observed in the
  // Linux GCC asset-planning path.
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char quote = text[i];
    if (quote != '"' && quote != '\'') continue;
    const auto begin = i + 1u;
    auto end = begin;
    while (end < text.size() && text[end] != quote && text[end] != '\r' && text[end] != '\n') {
      if (text[end] == '\\' && end + 1u < text.size()) end += 2u;
      else ++end;
    }
    if (end >= text.size() || text[end] != quote) continue;
    auto value = text.substr(begin, end - begin);
    i = end;
    std::replace(value.begin(), value.end(), ',', ' ');
    std::istringstream tokens(value);
    std::string token;
    while (tokens >> token) {
      if (!token.empty() && (token.back() == 'x' || token.back() == 'w') && token.size() > 1u &&
          std::all_of(token.begin(), token.end() - 1, [](unsigned char c) { return std::isdigit(c) != 0 || c == '.'; })) {
        continue;
      }
      add_reference(exact, patterns, source, std::move(token));
    }
  }

  const auto ascii_lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
  const auto starts_with_ci = [&](std::size_t pos, std::string_view needle) {
    if (pos + needle.size() > text.size()) return false;
    for (std::size_t j = 0; j < needle.size(); ++j) {
      if (ascii_lower(text[pos + j]) != ascii_lower(needle[j])) return false;
    }
    return true;
  };

  // CSS url(...)
  for (std::size_t i = 0; i + 4u <= text.size(); ++i) {
    if (!starts_with_ci(i, "url(")) continue;
    auto begin = i + 4u;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) ++begin;
    const auto close = text.find(')', begin);
    if (close == std::string::npos) break;
    auto value = clean_reference(text.substr(begin, close - begin));
    if (value.size() >= 2u && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
      value = value.substr(1u, value.size() - 2u);
    }
    add_reference(exact, patterns, source, std::move(value));
    i = close;
  }

  // Unquoted src/href/poster attributes.
  constexpr std::string_view names[] = {"src", "href", "poster"};
  for (std::size_t i = 0; i < text.size(); ++i) {
    for (const auto name : names) {
      if (!starts_with_ci(i, name)) continue;
      if (i > 0u) {
        const auto previous = static_cast<unsigned char>(text[i - 1u]);
        if (std::isalnum(previous) != 0 || text[i - 1u] == '_' || text[i - 1u] == '-') continue;
      }
      auto cursor = i + name.size();
      while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) != 0) ++cursor;
      if (cursor >= text.size() || text[cursor] != '=') continue;
      ++cursor;
      while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) != 0) ++cursor;
      if (cursor >= text.size() || text[cursor] == '"' || text[cursor] == '\'') continue;
      const auto begin = cursor;
      while (cursor < text.size()) {
        const char c = text[cursor];
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '"' || c == '\'' ||
            c == '=' || c == '<' || c == '>' || c == '`') break;
        ++cursor;
      }
      if (cursor > begin) add_reference(exact, patterns, source, text.substr(begin, cursor - begin));
      i = cursor;
      break;
    }
  }
}

bool matches_template_from(const std::string& path,
                           const std::string& pattern,
                           std::size_t path_pos,
                           std::size_t pattern_pos) {
  while (pattern_pos < pattern.size()) {
    if (pattern[pattern_pos] != '{') {
      if (path_pos >= path.size() || path[path_pos] != pattern[pattern_pos]) return false;
      ++path_pos;
      ++pattern_pos;
      continue;
    }

    const auto close = pattern.find('}', pattern_pos + 1u);
    if (close == std::string::npos) {
      if (path_pos >= path.size() || path[path_pos] != pattern[pattern_pos]) return false;
      ++path_pos;
      ++pattern_pos;
      continue;
    }

    // A placeholder matches one or more characters inside the current path
    // segment. It may have a literal suffix, for example {piece}.png.
    const auto segment_end = path.find('/', path_pos);
    const auto limit = segment_end == std::string::npos ? path.size() : segment_end;
    if (path_pos >= limit) return false;
    const auto next_pattern = close + 1u;
    for (auto candidate = path_pos + 1u; candidate <= limit; ++candidate) {
      if (matches_template_from(path, pattern, candidate, next_pattern)) return true;
    }
    return false;
  }
  return path_pos == path.size();
}

bool matches_template(const std::string& path, const std::string& pattern) {
  // Match literal text and treat each {...} placeholder as a non-empty wildcard
  // constrained to a single path segment. This supports both {name} and
  // embedded forms such as {piece}.png without runtime std::regex compilation.
  return matches_template_from(path, pattern, 0u, 0u);
}

std::set<std::string> planned_public_sources(const SiteGraph& graph,
                                             const std::vector<JsChunk>& planned_scripts) {
  std::set<std::string> exact;
  std::vector<std::string> patterns;

  // Every HTML file is an explicit route. Only stylesheets reachable from those
  // routes are scanned; an unrelated CSS file is never silently bundled.
  std::set<std::string> planned_styles;
  for (const auto& file : graph.files) {
    if (file.extension != ".html" && file.extension != ".htm") continue;
    const std::string text(reinterpret_cast<const char*>(file.bytes.data()), file.bytes.size());
    scan_reference_text(exact, patterns, file.relative, text);
  }
  for (const auto& path : exact) {
    if (lower_ext(path) == ".css") planned_styles.insert(path);
  }
  for (const auto& file : graph.files) {
    if (file.extension != ".css" || planned_styles.find(normalize_slashes(file.relative)) == planned_styles.end()) continue;
    const std::string text(reinterpret_cast<const char*>(file.bytes.data()), file.bytes.size());
    scan_reference_text(exact, patterns, file.relative, text);
  }
  for (const auto& chunk : planned_scripts) {
    scan_reference_text(exact, patterns, chunk.source, chunk.code);
  }

  std::set<std::string> selected;
  for (const auto& file : graph.files) {
    if (!is_public_asset(file, true, false)) continue;
    const auto relative = normalize_slashes(file.relative);
    if (exact.find(relative) != exact.end() ||
        std::any_of(patterns.begin(), patterns.end(), [&](const std::string& pattern) {
          return matches_template(relative, pattern);
        })) {
      selected.insert(relative);
    }
  }
  return selected;
}

bool is_image_asset(const std::string& mime) {
  return mime.rfind("image/", 0) == 0;
}

std::string safe_stem(std::string relative) {
  relative = normalize_slashes(std::move(relative));
  const auto ext_pos = relative.find_last_of('.');
  if (ext_pos != std::string::npos) {
    relative.resize(ext_pos);
  }
  for (char& c : relative) {
    const auto u = static_cast<unsigned char>(c);
    if (std::isalnum(u) == 0 && c != '-' && c != '_') {
      c = '_';
    }
  }
  while (!relative.empty() && relative.front() == '_') relative.erase(relative.begin());
  while (!relative.empty() && relative.back() == '_') relative.pop_back();
  if (relative.empty()) relative = "asset";
  return relative;
}

void write_bytes(const std::filesystem::path& path, const std::vector<unsigned char>& bytes) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    raise_error("VENOM-E5000", "failed to write asset " + path.string());
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::string make_manifest(const std::vector<AssetRecord>& records) {
  std::ostringstream out;
  out << "VASM0005\n";
  out << "version\t1\n";
  out << "count\t" << records.size() << "\n";
  out << "source\toutput\tpublic\tmime\tsize\thash\n";
  for (const auto& record : records) {
    out << record.source << '\t'
        << record.output_name << '\t'
        << record.public_path << '\t'
        << record.mime << '\t'
        << record.size << '\t'
        << short_hash_hex(record.hash, 16)
        << "\n";
  }
  return out.str();
}
} // namespace

std::string short_hash_hex(std::uint64_t value, std::size_t digits) {
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << value;
  auto text = out.str();
  if (digits > 0 && digits < text.size()) {
    text.resize(digits);
  }
  return text;
}

std::string detect_mime_type(const std::string& relative_path) {
  const auto ext = lower_ext(std::filesystem::path(relative_path));
  if (ext == ".svg") return "image/svg+xml";
  if (ext == ".png") return "image/png";
  if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
  if (ext == ".gif") return "image/gif";
  if (ext == ".webp") return "image/webp";
  if (ext == ".avif") return "image/avif";
  if (ext == ".ico") return "image/x-icon";
  if (ext == ".woff") return "font/woff";
  if (ext == ".woff2") return "font/woff2";
  if (ext == ".ttf") return "font/ttf";
  if (ext == ".otf") return "font/otf";
  if (ext == ".eot") return "application/vnd.ms-fontobject";
  if (ext == ".json") return "application/json";
  if (ext == ".xml") return "application/xml";
  if (ext == ".pdf") return "application/pdf";
  if (ext == ".txt") return "text/plain; charset=utf-8";
  if (ext == ".wasm") return "application/wasm";
  if (ext == ".mp4") return "video/mp4";
  if (ext == ".webm") return "video/webm";
  if (ext == ".mp3") return "audio/mpeg";
  if (ext == ".wav") return "audio/wav";
  return "application/octet-stream";
}

AssetPipeline build_asset_pipeline(const SiteGraph& graph,
                                   const std::vector<JsChunk>& planned_scripts,
                                   bool hashed,
                                   bool production,
                                   bool standard_layout_only) {
  AssetPipeline pipeline;
  pipeline.hashed = hashed;
  std::unordered_set<std::string> used_names;
  const auto planned_sources = planned_public_sources(graph, planned_scripts);

  for (const auto& file : graph.files) {
    if (!is_public_asset(file, production, standard_layout_only) ||
        planned_sources.find(normalize_slashes(file.relative)) == planned_sources.end()) {
      continue;
    }

    AssetRecord record;
    record.source = normalize_slashes(file.relative);
    record.hash = venom::package::fnv1a64(file.bytes);
    record.size = static_cast<std::uint64_t>(file.bytes.size());
    record.mime = detect_mime_type(file.relative);
    record.file = &file;

    const auto ext = std::filesystem::path(file.relative).extension().generic_string();
    const auto base = safe_stem(file.relative);
    if (hashed) {
      record.output_name = base + "." + short_hash_hex(record.hash, 12) + ext;
    } else {
      record.output_name = normalize_slashes(file.relative);
      for (char& c : record.output_name) {
        if (c == '/') c = '_';
      }
    }

    while (used_names.find(record.output_name) != used_names.end()) {
      record.output_name = safe_stem(file.relative) + "." + short_hash_hex(record.hash, 12) + "." + std::to_string(used_names.size()) + ext;
    }
    used_names.insert(record.output_name);

    if (is_image_asset(record.mime)) {
      record.output_name = "images/" + record.output_name;
      record.public_path = "assets/" + record.output_name;
      // Production CSS is emitted under assets/app/, so image URLs must step
      // back to the shared assets root before entering assets/images/.
      record.css_url = "../" + record.output_name;
    } else if (is_third_party_notice(file)) {
      record.output_name = "app/third-party-notices.txt";
      record.public_path = "assets/" + record.output_name;
      record.css_url = "../" + record.output_name;
    } else {
      record.public_path = "assets/" + record.output_name;
      record.css_url = "../" + record.output_name;
    }
    pipeline.by_source[record.source] = pipeline.records.size();
    pipeline.records.push_back(std::move(record));
  }

  std::sort(pipeline.records.begin(), pipeline.records.end(), [](const AssetRecord& a, const AssetRecord& b) {
    return a.source < b.source;
  });
  pipeline.by_source.clear();
  for (std::size_t i = 0; i < pipeline.records.size(); ++i) {
    pipeline.by_source[pipeline.records[i].source] = i;
  }
  pipeline.manifest_text = make_manifest(pipeline.records);
  return pipeline;
}

const AssetRecord* find_asset(const AssetPipeline& pipeline, const std::string& source) {
  const auto normalized = normalize_slashes(source);
  const auto it = pipeline.by_source.find(normalized);
  if (it == pipeline.by_source.end()) {
    return nullptr;
  }
  return &pipeline.records[it->second];
}

void emit_public_assets(const AssetPipeline& pipeline, const std::filesystem::path& output_assets_dir) {
  std::filesystem::create_directories(output_assets_dir);
  for (const auto& record : pipeline.records) {
    if (record.file == nullptr) {
      raise_error("VENOM-E5000", "asset pipeline record is missing source file: " + record.source);
    }
    write_bytes(output_assets_dir / record.output_name, record.file->bytes);
  }
}

} // namespace venom::compiler
