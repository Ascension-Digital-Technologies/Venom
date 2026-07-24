#include "base/error.hpp"
#include "pipeline/security_artifact_inspection.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace venom::compiler::security_detail {
std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    raise_error("VENOM-E5000", "failed to read key file " + path.string());
  }
  return std::string(std::istreambuf_iterator<char>(in), {});
}

std::vector<unsigned char> read_binary_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    raise_error("VENOM-E5000", "failed to read " + path.string());
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

} // namespace venom::compiler::security_detail
