#include "venom/base/error.hpp"
#include "security_artifact_inspection.hpp"

#include "venom/package/hash.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace venom::compiler::security_detail {

std::filesystem::path resolve_package_path(const std::filesystem::path& target) {
  if (std::filesystem::is_regular_file(target)) {
    return target;
  }
  if (!std::filesystem::is_directory(target)) {
    raise_error("VENOM-E5000", "verify target does not exist: " + target.string());
  }
  const auto assets_dir = target / "assets";
  const auto stable = assets_dir / "app" / "app.vbc";
  if (std::filesystem::exists(stable)) {
    return stable;
  }
  const auto legacy_stable = assets_dir / "app.vbc";
  if (std::filesystem::exists(legacy_stable)) {
    return legacy_stable;
  }
  const auto app_dir = assets_dir / "app";
  if (std::filesystem::is_directory(app_dir)) {
    std::filesystem::path discovered;
    for (const auto& entry : std::filesystem::directory_iterator(app_dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".vbc") {
        continue;
      }
      if (!discovered.empty()) {
        raise_error("VENOM-E5000", "verify found multiple VBC packages under " + app_dir.string());
      }
      discovered = entry.path();
    }
    if (!discovered.empty()) return discovered;
  }
  if (std::filesystem::is_directory(assets_dir)) {
    for (const auto& entry : std::filesystem::directory_iterator(assets_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const auto filename = entry.path().filename().string();
      if (filename.rfind("app.", 0) == 0 && entry.path().extension() == ".vbc") {
        return entry.path();
      }
    }
  }
  raise_error("VENOM-E5000", "verify could not find a VBC package under assets/app/ in " + target.string());
}

std::filesystem::path resolve_dist_root(const std::filesystem::path& target, const std::filesystem::path& package) {
  if (std::filesystem::is_directory(target)) {
    return target;
  }
  const auto parent = package.parent_path();
  if (parent.filename() == "app" && parent.parent_path().filename() == "assets") {
    return parent.parent_path().parent_path();
  }
  if (parent.filename() == "assets") {
    return parent.parent_path();
  }
  return parent;
}

std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

bool section_has_canonical_leak(venom::package::SectionType type, const std::string& name) {
  static const char* forbidden[] = {
    "route-bytecode.vmbc",
    "routes.vbrt",
    "strings.vstr",
    "runtime-policy.vhrd",
    "package-features.vfeat",
    "integrity-auth.vsig",
    "quickjs-release-gate.vqrg",
    "quickjs-bytecode-manifest.vqbm",
    "quickjs-wasm-execution.vqwe",
    "release-profile.vrpf",
    "vm-polymorph.vpol",
    "remote-vendors.vrvd",
  };
  for (const char* canonical : forbidden) {
    if (venom::package::section_name_matches(type, name, canonical) && name == canonical) {
      return true;
    }
  }
  if (name.rfind("route-chunk.", 0) == 0 || name.rfind("script-chunk.", 0) == 0 || name == "lazy-sections.vlazy") {
    return true;
  }
  return false;
}

std::vector<std::string> split_tabs_local(const std::string& line) {
  std::vector<std::string> parts;
  std::string current;
  std::stringstream input(line);
  while (std::getline(input, current, '	')) {
    parts.push_back(current);
  }
  return parts;
}

std::string metadata_value(const std::string& text, const std::string& key) {
  std::stringstream input(text);
  std::string line;
  const std::string prefix = key + "=";
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind(prefix, 0) == 0) {
      return line.substr(prefix.size());
    }
  }
  return {};
}

std::vector<BoundAssetRecord> parse_bound_assets(const std::string& text) {
  std::vector<BoundAssetRecord> assets;
  std::stringstream input(text);
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.rfind("asset\t", 0) != 0) {
      continue;
    }
    const auto parts = split_tabs_local(line);
    if (parts.size() != 4u) {
      raise_error("VENOM-E5000", "invalid package binding asset row");
    }
    assets.push_back(BoundAssetRecord{parts[1], parts[2], parts[3]});
  }
  return assets;
}

std::filesystem::path verification_html_path(const std::filesystem::path& dist_root) {
  for (const auto* name : {"index.html", "engine.html", "extension/engine.html", "assets/extension/engine.html"}) {
    const auto candidate = dist_root / name;
    if (std::filesystem::is_regular_file(candidate)) return candidate;
  }
  return {};
}

std::filesystem::path find_html_asset_reference(const std::filesystem::path& dist_root,
                                                const std::regex& pattern) {
  const auto html_path = verification_html_path(dist_root);
  if (html_path.empty()) return {};
  const auto html = read_text_file(html_path);
  std::smatch match;
  if (!std::regex_search(html, match, pattern) || match.size() < 2u) return {};
  const auto candidate = (html_path.parent_path() / std::filesystem::path(match[1].str())).lexically_normal();
  const auto relative = candidate.lexically_relative(dist_root);
  if (relative.empty() || relative.generic_string().rfind("..", 0) == 0) return {};
  return std::filesystem::is_regular_file(candidate) ? candidate : std::filesystem::path{};
}

std::filesystem::path find_loader_asset(const std::filesystem::path& dist_root) {
  const std::regex module_script(R"(<script[^>]*type=["']module["'][^>]*src=["']([^"']+\.js)["'][^>]*>)",
                                 std::regex::icase);
  auto from_html = find_html_asset_reference(dist_root, module_script);
  if (!from_html.empty()) return from_html;

  const auto javascript_dir = dist_root / "assets" / "javascript";
  if (std::filesystem::is_directory(javascript_dir)) {
    for (const auto& entry : std::filesystem::directory_iterator(javascript_dir)) {
      if (entry.is_regular_file() && entry.path().extension() == ".js") {
        const auto text = read_text_file(entry.path());
        if (!extract_loader_binding_token(text).empty()) return entry.path();
      }
    }
  }
  return {};
}

std::filesystem::path find_style_asset(const std::filesystem::path& dist_root) {
  const std::regex stylesheet(R"(<link[^>]*rel=["']stylesheet["'][^>]*href=["']([^"']+\.css)["'][^>]*>)",
                              std::regex::icase);
  auto from_html = find_html_asset_reference(dist_root, stylesheet);
  if (!from_html.empty()) return from_html;

  const auto app_dir = dist_root / "assets" / "app";
  if (!std::filesystem::is_directory(app_dir)) return {};
  for (const auto& entry : std::filesystem::directory_iterator(app_dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".css") return entry.path();
  }
  return {};
}

std::string public_asset_path(const std::filesystem::path& asset, const std::filesystem::path& dist_root) {
  if (asset.empty()) return {};
  return std::filesystem::relative(asset, dist_root).generic_string();
}

namespace {

std::string normalize_html_asset_reference(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  const auto suffix = value.find_first_of("?#");
  if (suffix != std::string::npos) value.erase(suffix);
  while (value.rfind("./", 0) == 0) value.erase(0, 2);
  while (!value.empty() && value.front() == '/') value.erase(value.begin());
  return value;
}

std::string ascii_lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string html_attribute_value(const std::string& tag, const std::string& name) {
  const auto expected = ascii_lower(name);
  std::size_t cursor = 0;
  while (cursor < tag.size()) {
    while (cursor < tag.size() &&
           !(std::isalpha(static_cast<unsigned char>(tag[cursor])) || tag[cursor] == '_' || tag[cursor] == ':')) {
      ++cursor;
    }
    const auto key_begin = cursor;
    while (cursor < tag.size() &&
           (std::isalnum(static_cast<unsigned char>(tag[cursor])) || tag[cursor] == '_' || tag[cursor] == ':' ||
            tag[cursor] == '.' || tag[cursor] == '-')) {
      ++cursor;
    }
    if (cursor == key_begin) break;
    const auto key = ascii_lower(tag.substr(key_begin, cursor - key_begin));
    while (cursor < tag.size() && std::isspace(static_cast<unsigned char>(tag[cursor]))) ++cursor;
    if (cursor >= tag.size() || tag[cursor] != '=') continue;
    ++cursor;
    while (cursor < tag.size() && std::isspace(static_cast<unsigned char>(tag[cursor]))) ++cursor;
    if (cursor >= tag.size() || (tag[cursor] != '"' && tag[cursor] != '\'')) continue;
    const char quote = tag[cursor++];
    const auto value_begin = cursor;
    const auto value_end = tag.find(quote, value_begin);
    if (value_end == std::string::npos) return {};
    if (key == expected) return tag.substr(value_begin, value_end - value_begin);
    cursor = value_end + 1;
  }
  return {};
}

} // namespace

std::string extract_integrity_for_asset(const std::string& html, const std::string& public_path) {
  const auto expected = normalize_html_asset_reference(public_path);
  if (expected.empty()) return {};

  const auto lower_html = ascii_lower(html);
  std::size_t cursor = 0;
  while (cursor < html.size()) {
    const auto script = lower_html.find("<script", cursor);
    const auto link = lower_html.find("<link", cursor);
    const auto tag_start = script == std::string::npos ? link :
                           link == std::string::npos ? script : std::min(script, link);
    if (tag_start == std::string::npos) break;
    const auto tag_end = html.find('>', tag_start);
    if (tag_end == std::string::npos) break;
    const auto tag = html.substr(tag_start, tag_end - tag_start + 1u);
    auto reference = html_attribute_value(tag, "src");
    if (reference.empty()) reference = html_attribute_value(tag, "href");
    if (!reference.empty() && normalize_html_asset_reference(reference) == expected) {
      const auto integrity = html_attribute_value(tag, "integrity");
      if (!integrity.empty()) return integrity;
    }
    cursor = tag_end + 1u;
  }
  return {};
}

std::string extract_loader_binding_token(const std::string& loader_text) {
  const std::string marker = "vbind:";
  const auto marker_start = loader_text.find(marker);
  if (marker_start != std::string::npos) {
    const auto value_start = marker_start + marker.size();
    const auto value_end = loader_text.find_first_of("\"'", value_start);
    if (value_end != std::string::npos && value_end > value_start) {
      return loader_text.substr(value_start, value_end - value_start);
    }
  }
  const std::string needle = "bindingToken: '";
  const auto start = loader_text.find(needle);
  if (start == std::string::npos) return {};
  const auto value_start = start + needle.size();
  const auto end = loader_text.find("'", value_start);
  if (end == std::string::npos) return {};
  return loader_text.substr(value_start, end - value_start);
}

std::uint64_t decoded_payload_padding_bytes(const venom::package::Package& pkg) {
  std::vector<const venom::package::Section*> sections;
  sections.reserve(pkg.sections.size());
  for (const auto& section : pkg.sections) {
    sections.push_back(&section);
  }
  std::sort(sections.begin(), sections.end(), [](const auto* a, const auto* b) {
    return a->offset < b->offset;
  });
  std::uint64_t cursor = pkg.payload_offset;
  std::uint64_t padding = 0;
  for (const auto* section : sections) {
    if (section->offset > cursor) {
      padding += section->offset - cursor;
    }
    const auto end = section->offset + section->stored_size;
    if (end > cursor) {
      cursor = end;
    }
  }
  const auto payload_end = pkg.payload_offset + pkg.payload_size;
  if (payload_end > cursor) {
    padding += payload_end - cursor;
  }
  return padding;
}




} // namespace venom::compiler::security_detail
