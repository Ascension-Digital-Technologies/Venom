#pragma once

#include "package/format.hpp"
#include "package/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler::security_detail {

struct BoundAssetRecord {
  std::string role;
  std::string name;
  std::string digest;
};

std::string read_text_file(const std::filesystem::path& path);
std::vector<unsigned char> read_binary_file(const std::filesystem::path& path);
bool contains_bytes(const std::vector<unsigned char>& haystack, const std::string& needle);
std::size_t count_bytes(const std::vector<unsigned char>& haystack, const std::string& needle);
std::size_t count_js_bundle_flag(const std::vector<unsigned char>& bytes, std::uint32_t flag);
std::size_t count_js_bundle_flagged_payload_prefix(const std::vector<unsigned char>& bytes,
                                                   std::uint32_t required_flag,
                                                   const std::string& prefix);
bool payload_starts_with(const std::vector<unsigned char>& bytes, const std::string& prefix);
bool js_bundle_flagged_payload_contains(const std::vector<unsigned char>& bytes,
                                        std::uint32_t required_flag,
                                        const std::string& needle);
std::filesystem::path resolve_package_path(const std::filesystem::path& target);
std::filesystem::path resolve_dist_root(const std::filesystem::path& target,
                                        const std::filesystem::path& package);
std::string hex64(std::uint64_t value);
bool section_has_canonical_leak(venom::package::SectionType type, const std::string& name);
std::string metadata_value(const std::string& text, const std::string& key);
std::vector<BoundAssetRecord> parse_bound_assets(const std::string& text);
std::filesystem::path verification_html_path(const std::filesystem::path& dist_root);
std::filesystem::path find_loader_asset(const std::filesystem::path& dist_root);
std::filesystem::path find_style_asset(const std::filesystem::path& dist_root);
std::string public_asset_path(const std::filesystem::path& asset,
                              const std::filesystem::path& dist_root);
std::string extract_integrity_for_asset(const std::string& html, const std::string& public_path);
std::string extract_loader_binding_token(const std::string& loader_text);
std::uint64_t decoded_payload_padding_bytes(const venom::package::Package& pkg);

} // namespace venom::compiler::security_detail
