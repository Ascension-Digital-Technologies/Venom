#include "compiler/pipeline/security_artifact_inspection.hpp"

#include "package/hash.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace venom::compiler::security_detail {

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to read key file " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), {});
}

std::vector<unsigned char> read_binary_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("failed to read " + path.string());
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

bool contains_bytes(const std::vector<unsigned char>& haystack, const std::string& needle) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return false;
  }
  return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end()) != haystack.end();
}

std::size_t count_bytes(const std::vector<unsigned char>& haystack, const std::string& needle) {
  if (needle.empty() || haystack.size() < needle.size()) {
    return 0;
  }
  std::size_t count = 0;
  auto it = haystack.begin();
  while ((it = std::search(it, haystack.end(), needle.begin(), needle.end())) != haystack.end()) {
    ++count;
    ++it;
  }
  return count;
}

std::uint32_t read_u32_le(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 4u > bytes.size()) return 0u;
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::size_t count_js_bundle_flag(const std::vector<unsigned char>& bytes, std::uint32_t flag) {
  static const std::vector<unsigned char> magic{'V','J','S','B','0','0','0','6'};
  if (bytes.size() < 24u || !std::equal(magic.begin(), magic.end(), bytes.begin())) return 0u;
  const auto count = read_u32_le(bytes, 12u);
  constexpr std::size_t entry_size = 40u;
  constexpr std::size_t flags_offset = 28u;
  if (24u + static_cast<std::size_t>(count) * entry_size > bytes.size()) return 0u;
  std::size_t matches = 0u;
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto flags = read_u32_le(bytes, 24u + static_cast<std::size_t>(i) * entry_size + flags_offset);
    if ((flags & flag) != 0u) ++matches;
  }
  return matches;
}

bool payload_starts_with(const std::vector<unsigned char>& bytes, const std::string& prefix) {
  return !prefix.empty() && bytes.size() >= prefix.size() &&
         std::equal(prefix.begin(), prefix.end(), bytes.begin());
}

std::size_t count_js_bundle_flagged_payload_prefix(const std::vector<unsigned char>& bytes,
                                                   std::uint32_t required_flag,
                                                   const std::string& prefix) {
  static const std::vector<unsigned char> magic{'V','J','S','B','0','0','0','6'};
  if (prefix.empty() || bytes.size() < 24u || !std::equal(magic.begin(), magic.end(), bytes.begin())) return 0u;
  const auto count = read_u32_le(bytes, 12u);
  const auto text_size = read_u32_le(bytes, 16u);
  constexpr std::size_t entry_size = 40u;
  constexpr std::size_t code_offset_field = 16u;
  constexpr std::size_t code_size_field = 20u;
  constexpr std::size_t flags_offset = 28u;
  const auto table_end = 24u + static_cast<std::size_t>(count) * entry_size;
  if (table_end > bytes.size() || static_cast<std::size_t>(text_size) > bytes.size() - table_end) return 0u;
  const auto code_base = table_end + static_cast<std::size_t>(text_size);
  std::size_t matches = 0u;
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto entry = 24u + static_cast<std::size_t>(i) * entry_size;
    const auto flags = read_u32_le(bytes, entry + flags_offset);
    if ((flags & required_flag) == 0u) continue;
    const auto code_offset = static_cast<std::size_t>(read_u32_le(bytes, entry + code_offset_field));
    const auto code_size = static_cast<std::size_t>(read_u32_le(bytes, entry + code_size_field));
    if (code_offset > bytes.size() - code_base || code_size > bytes.size() - code_base - code_offset) continue;
    if (code_size < prefix.size()) continue;
    const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(code_base + code_offset);
    if (std::equal(prefix.begin(), prefix.end(), begin)) ++matches;
  }
  return matches;
}


bool js_bundle_flagged_payload_contains(const std::vector<unsigned char>& bytes,
                                        std::uint32_t required_flag,
                                        const std::string& needle) {
  static const std::vector<unsigned char> magic{'V','J','S','B','0','0','0','6'};
  if (needle.empty() || bytes.size() < 24u || !std::equal(magic.begin(), magic.end(), bytes.begin())) return false;
  const auto count = read_u32_le(bytes, 12u);
  const auto text_size = read_u32_le(bytes, 16u);
  constexpr std::size_t entry_size = 40u;
  constexpr std::size_t code_offset_field = 16u;
  constexpr std::size_t code_size_field = 20u;
  constexpr std::size_t flags_offset = 28u;
  const auto table_end = 24u + static_cast<std::size_t>(count) * entry_size;
  if (table_end > bytes.size() || static_cast<std::size_t>(text_size) > bytes.size() - table_end) return false;
  const auto code_base = table_end + static_cast<std::size_t>(text_size);
  for (std::uint32_t i = 0; i < count; ++i) {
    const auto entry = 24u + static_cast<std::size_t>(i) * entry_size;
    const auto flags = read_u32_le(bytes, entry + flags_offset);
    if ((flags & required_flag) == 0u) continue;
    const auto code_offset = static_cast<std::size_t>(read_u32_le(bytes, entry + code_offset_field));
    const auto code_size = static_cast<std::size_t>(read_u32_le(bytes, entry + code_size_field));
    if (code_offset > bytes.size() - code_base || code_size > bytes.size() - code_base - code_offset) return true;
    const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(code_base + code_offset);
    const auto finish = begin + static_cast<std::ptrdiff_t>(code_size);
    if (std::search(begin, finish, needle.begin(), needle.end()) != finish) return true;
  }
  return false;
}


std::filesystem::path resolve_package_path(const std::filesystem::path& target) {
  if (std::filesystem::is_regular_file(target)) {
    return target;
  }
  if (!std::filesystem::is_directory(target)) {
    throw std::runtime_error("release-check target does not exist: " + target.string());
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
    for (const auto& entry : std::filesystem::directory_iterator(app_dir)) {
      if (!entry.is_regular_file()) {
        continue;
      }
      const auto filename = entry.path().filename().string();
      if (filename.rfind("app.", 0) == 0 && entry.path().extension() == ".vbc") {
        return entry.path();
      }
    }
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
  throw std::runtime_error("release-check could not find assets/app/app.vbc or assets/app/app.<hash>.vbc under " + target.string());
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
      throw std::runtime_error("invalid package binding asset row");
    }
    assets.push_back(BoundAssetRecord{parts[1], parts[2], parts[3]});
  }
  return assets;
}

std::filesystem::path find_loader_asset(const std::filesystem::path& dist_root) {
  const auto assets_dir = dist_root / "assets";
  if (!std::filesystem::is_directory(assets_dir)) {
    return {};
  }
  for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_dir)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto filename = entry.path().filename().string();
    if (filename.rfind("loader", 0) == 0 && entry.path().extension() == ".js") {
      return entry.path();
    }
  }
  return {};
}

std::filesystem::path find_style_asset(const std::filesystem::path& dist_root) {
  const auto style_dir = dist_root / "assets" / "style";
  if (!std::filesystem::is_directory(style_dir)) {
    return {};
  }
  for (const auto& entry : std::filesystem::directory_iterator(style_dir)) {
    if (!entry.is_regular_file()) continue;
    const auto filename = entry.path().filename().string();
    if (filename.rfind("s.", 0) == 0 && entry.path().extension() == ".css") return entry.path();
  }
  return {};
}

std::string public_asset_path(const std::filesystem::path& asset, const std::filesystem::path& dist_root) {
  if (asset.empty()) return {};
  return std::filesystem::relative(asset, dist_root).generic_string();
}

std::string extract_integrity_for_asset(const std::string& html, const std::string& public_path) {
  if (public_path.empty()) return {};
  const auto asset_pos = html.find(public_path);
  if (asset_pos == std::string::npos) return {};
  const auto tag_start = html.rfind('<', asset_pos);
  const auto tag_end = html.find('>', asset_pos);
  if (tag_start == std::string::npos || tag_end == std::string::npos || tag_end <= tag_start) return {};
  const auto tag = html.substr(tag_start, tag_end - tag_start + 1u);
  const std::string marker = "integrity=\"";
  const auto integrity_pos = tag.find(marker);
  if (integrity_pos == std::string::npos) return {};
  const auto value_start = integrity_pos + marker.size();
  const auto value_end = tag.find('"', value_start);
  if (value_end == std::string::npos) return {};
  return tag.substr(value_start, value_end - value_start);
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
