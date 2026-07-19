#include "venom/internal/pipeline/css.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace venom::compiler {

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

std::string normalize_slashes(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  while (value.rfind("./", 0) == 0) {
    value.erase(0, 2);
  }
  std::vector<std::string> parts;
  std::size_t cursor = 0;
  while (cursor <= value.size()) {
    const auto slash = value.find('/', cursor);
    const auto part = value.substr(cursor, slash == std::string::npos ? std::string::npos : slash - cursor);
    if (!part.empty() && part != ".") {
      if (part == ".." && !parts.empty()) parts.pop_back();
      else if (part != "..") parts.push_back(part);
    }
    if (slash == std::string::npos) break;
    cursor = slash + 1;
  }
  std::string out;
  for (const auto& part : parts) {
    if (!out.empty()) out += '/';
    out += part;
  }
  return out;
}

std::string trim(std::string value) {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) value.erase(value.begin());
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) value.pop_back();
  return value;
}

bool is_name_char(char c) {
  const auto u = static_cast<unsigned char>(c);
  return std::isalnum(u) != 0 || c == '-' || c == '_' || c == ':' || c == '.';
}

std::size_t find_case_insensitive(std::string_view haystack, std::string_view needle, std::size_t start) {
  if (needle.empty() || haystack.size() < needle.size()) return std::string::npos;
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
    if (match) return i;
  }
  return std::string::npos;
}

std::string decode_basic_entities(std::string value) {
  const std::pair<const char*, const char*> replacements[] = {
    {"&amp;", "&"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&#39;", "'"},
  };
  for (const auto& item : replacements) {
    std::size_t pos = 0;
    while ((pos = value.find(item.first, pos)) != std::string::npos) {
      value.replace(pos, std::char_traits<char>::length(item.first), item.second);
      pos += std::char_traits<char>::length(item.second);
    }
  }
  return value;
}

std::string attribute_value(std::string_view source, std::string wanted) {
  wanted = lower_ascii(std::move(wanted));
  std::size_t i = 0;
  while (i < source.size()) {
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
    if (i >= source.size() || source[i] == '/' || source[i] == '>') break;
    const auto name_start = i;
    while (i < source.size() && is_name_char(source[i])) ++i;
    if (name_start == i) {
      ++i;
      continue;
    }
    auto name = lower_ascii(std::string(source.substr(name_start, i - name_start)));
    while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
    std::string value;
    if (i < source.size() && source[i] == '=') {
      ++i;
      while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) != 0) ++i;
      if (i < source.size() && (source[i] == '\'' || source[i] == '"')) {
        const char quote = source[i++];
        const auto value_start = i;
        while (i < source.size() && source[i] != quote) ++i;
        value = std::string(source.substr(value_start, i - value_start));
        if (i < source.size()) ++i;
      } else {
        const auto value_start = i;
        while (i < source.size() && std::isspace(static_cast<unsigned char>(source[i])) == 0 && source[i] != '>' && source[i] != '/') ++i;
        value = std::string(source.substr(value_start, i - value_start));
      }
    }
    if (name == wanted) return decode_basic_entities(std::move(value));
  }
  return {};
}

bool rel_contains_stylesheet(const std::string& rel) {
  std::stringstream input(lower_ascii(rel));
  std::string token;
  while (input >> token) {
    if (token == "stylesheet") return true;
  }
  return false;
}

bool should_leave_url(const std::string& url) {
  const auto lower = lower_ascii(url);
  return lower.empty() || lower[0] == '#' || lower.rfind("data:", 0) == 0 ||
         lower.rfind("http:", 0) == 0 || lower.rfind("https:", 0) == 0 ||
         lower.rfind("//", 0) == 0 || lower.rfind("blob:", 0) == 0;
}

bool is_remote_url(const std::string& url) {
  const auto lower = lower_ascii(trim(url));
  return lower.rfind("http:", 0) == 0 || lower.rfind("https:", 0) == 0 ||
         lower.rfind("//", 0) == 0 || lower.rfind("data:", 0) == 0 ||
         lower.rfind("blob:", 0) == 0;
}

std::string dirname_of(const std::string& relative) {
  const auto normalized = normalize_slashes(relative);
  const auto slash = normalized.find_last_of('/');
  if (slash == std::string::npos) return {};
  return normalized.substr(0, slash + 1);
}

std::string resolve_relative_path(const std::string& source_path, std::string raw_path) {
  raw_path = trim(std::move(raw_path));
  if (raw_path.size() >= 2 && ((raw_path.front() == '"' && raw_path.back() == '"') ||
                               (raw_path.front() == '\'' && raw_path.back() == '\''))) {
    raw_path = raw_path.substr(1, raw_path.size() - 2);
  }
  const auto suffix = raw_path.find_first_of("?#");
  if (suffix != std::string::npos) raw_path.resize(suffix);
  while (!raw_path.empty() && raw_path.front() == '/') raw_path.erase(raw_path.begin());
  return normalize_slashes(dirname_of(source_path) + raw_path);
}

const SiteFile* find_file(const SiteGraph& graph, const std::string& relative) {
  const auto normalized = normalize_slashes(relative);
  for (const auto& file : graph.files) {
    if (normalize_slashes(file.relative) == normalized) return &file;
  }
  return nullptr;
}

std::string resolve_css_url(const std::string& css_source, std::string raw_url) {
  raw_url = trim(std::move(raw_url));
  if (raw_url.size() >= 2 && ((raw_url.front() == '"' && raw_url.back() == '"') || (raw_url.front() == '\'' && raw_url.back() == '\''))) {
    raw_url = raw_url.substr(1, raw_url.size() - 2);
  }
  if (raw_url.empty() || raw_url.front() == '/') {
    while (!raw_url.empty() && raw_url.front() == '/') raw_url.erase(raw_url.begin());
    return normalize_slashes(raw_url);
  }
  return normalize_slashes(dirname_of(css_source) + raw_url);
}

std::string rewrite_css_urls(std::string_view source_view, const std::string& source_path, const AssetPipeline& assets) {
  const std::string source(source_view);
  std::string out;
  out.reserve(source.size());

  std::size_t cursor = 0;
  while (cursor < source.size()) {
    const auto pos = find_case_insensitive(source, "url(", cursor);
    if (pos == std::string::npos) {
      out.append(source.substr(cursor));
      break;
    }

    out.append(source.substr(cursor, pos - cursor));
    const auto start = pos + 4;
    auto end = start;
    bool in_quote = false;
    char quote = '\0';
    for (; end < source.size(); ++end) {
      const char c = source[end];
      if (!in_quote && (c == '"' || c == '\'')) {
        in_quote = true;
        quote = c;
        continue;
      }
      if (in_quote && c == quote) {
        in_quote = false;
        continue;
      }
      if (!in_quote && c == ')') break;
    }
    if (end >= source.size()) {
      out.append(source.substr(pos));
      break;
    }

    const auto raw = source.substr(start, end - start);
    auto clean = trim(raw);
    if (clean.size() >= 2 && ((clean.front() == '"' && clean.back() == '"') || (clean.front() == '\'' && clean.back() == '\''))) {
      clean = clean.substr(1, clean.size() - 2);
    }

    if (should_leave_url(clean)) {
      out.append("url(").append(raw).append(")");
    } else {
      const auto resolved = resolve_css_url(source_path, clean);
      if (const auto* asset = find_asset(assets, resolved)) {
        out.append("url(\"").append(asset->css_url).append("\")");
      } else {
        out.append("url(").append(raw).append(")");
      }
    }
    cursor = end + 1;
  }

  return out;
}

void append_css(std::ostringstream& out,
                std::string_view css,
                const std::string& source_path,
                const std::string& label,
                const std::string& media,
                const AssetPipeline& assets,
                std::size_t& source_count) {
  if (trim(std::string(css)).empty()) return;
  out << "\n/* source: " << label << " */\n";
  const auto rewritten = rewrite_css_urls(css, source_path, assets);
  if (!trim(media).empty() && lower_ascii(trim(media)) != "all") {
    out << "@media " << media << " {\n" << rewritten << "\n}\n";
  } else {
    out << rewritten << "\n";
  }
  ++source_count;
}

std::vector<const SiteFile*> sorted_html_files(const SiteGraph& graph) {
  std::vector<const SiteFile*> files;
  for (const auto& file : graph.files) {
    if (file.extension == ".html" || file.extension == ".htm") files.push_back(&file);
  }
  std::sort(files.begin(), files.end(), [](const SiteFile* a, const SiteFile* b) {
    if (a->relative == "index.html") return true;
    if (b->relative == "index.html") return false;
    return a->relative < b->relative;
  });
  return files;
}

void collect_document_styles(const SiteGraph& graph,
                             const SiteFile& html_file,
                             const AssetPipeline& assets,
                             std::unordered_set<std::string>& emitted_files,
                             std::ostringstream& out,
                             std::size_t& source_count) {
  const auto html = to_string(html_file.bytes);
  std::size_t cursor = 0;
  std::size_t inline_index = 0;
  while (cursor < html.size()) {
    const auto style_pos = find_case_insensitive(html, "<style", cursor);
    const auto link_pos = find_case_insensitive(html, "<link", cursor);
    const bool use_style = style_pos != std::string::npos && (link_pos == std::string::npos || style_pos < link_pos);
    const auto pos = use_style ? style_pos : link_pos;
    if (pos == std::string::npos) break;

    const auto name_len = use_style ? 6u : 5u;
    if (pos + name_len < html.size() && is_name_char(html[pos + name_len])) {
      cursor = pos + name_len;
      continue;
    }
    const auto open_end = html.find('>', pos + name_len);
    if (open_end == std::string::npos) break;
    const auto attrs = std::string_view(html).substr(pos + name_len, open_end - pos - name_len);

    if (use_style) {
      const auto type = lower_ascii(trim(attribute_value(attrs, "type")));
      const auto close_start = find_case_insensitive(html, "</style", open_end + 1);
      if (close_start == std::string::npos) break;
      const auto close_end = html.find('>', close_start);
      if (type.empty() || type == "text/css") {
        const auto css = std::string_view(html).substr(open_end + 1, close_start - open_end - 1);
        append_css(out, css, html_file.relative,
                   html_file.relative + "#inline-style-" + std::to_string(inline_index++),
                   attribute_value(attrs, "media"), assets, source_count);
      }
      cursor = close_end == std::string::npos ? html.size() : close_end + 1;
      continue;
    }

    const auto rel = attribute_value(attrs, "rel");
    const auto href = attribute_value(attrs, "href");
    if (rel_contains_stylesheet(rel) && !href.empty() && !is_remote_url(href)) {
      const auto resolved = resolve_relative_path(html_file.relative, href);
      if (!resolved.empty() && emitted_files.insert(resolved).second) {
        const auto* css_file = find_file(graph, resolved);
        if (css_file != nullptr && css_file->extension == ".css") {
          append_css(out, to_string(css_file->bytes), css_file->relative, css_file->relative,
                     attribute_value(attrs, "media"), assets, source_count);
        }
      }
    }
    cursor = open_end + 1;
  }
}
} // namespace

std::string bundle_css(const SiteGraph& graph, const AssetPipeline& assets) {
  std::ostringstream out;
  out << "/* generated by venom; source cascade preserved */\n";
  std::unordered_set<std::string> emitted_files;
  std::size_t source_count = 0;

  for (const auto* html_file : sorted_html_files(graph)) {
    collect_document_styles(graph, *html_file, assets, emitted_files, out, source_count);
  }

  for (const auto& file : graph.files) {
    if (file.extension != ".css") continue;
    const auto normalized = normalize_slashes(file.relative);
    if (!emitted_files.insert(normalized).second) continue;
    append_css(out, to_string(file.bytes), file.relative, file.relative, {}, assets, source_count);
  }

  if (source_count == 0) {
    out << "/* source site supplied no CSS; Venom intentionally injects no visual defaults. */\n";
  }
  return out.str();
}

} // namespace venom::compiler
