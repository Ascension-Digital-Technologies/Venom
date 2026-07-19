#include "venom/base/error.hpp"
#include "venom/package/reader.hpp"

#include "compress.hpp"
#include "venom/package/crypto.hpp"
#include "venom/package/hash.hpp"

#include <cctype>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace venom::package {

namespace {
std::vector<unsigned char> read_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    raise_error("VENOM-E4000", "failed to read package " + path.string());
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

std::uint32_t read_u32_at(const std::vector<unsigned char>& bytes, std::uint64_t offset) {
  if (offset + 4u > bytes.size()) {
    raise_error("VENOM-E4000", "truncated package while reading u32");
  }
  return static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset)]) |
         (static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 1u)]) << 8u) |
         (static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 2u)]) << 16u) |
         (static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 3u)]) << 24u);
}

std::uint64_t read_u64_at(const std::vector<unsigned char>& bytes, std::uint64_t offset) {
  if (offset + 8u > bytes.size()) {
    raise_error("VENOM-E4000", "truncated package while reading u64");
  }
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value |= static_cast<std::uint64_t>(bytes[static_cast<std::size_t>(offset + static_cast<std::uint64_t>(i))]) << (i * 8);
  }
  return value;
}

std::string read_string_at(const std::vector<unsigned char>& bytes, std::uint64_t offset, std::uint64_t size) {
  if (offset + size > bytes.size()) {
    raise_error("VENOM-E4000", "truncated package while reading section name");
  }
  return std::string(reinterpret_cast<const char*>(bytes.data() + static_cast<std::size_t>(offset)), static_cast<std::size_t>(size));
}

std::vector<unsigned char> read_bytes_at(const std::vector<unsigned char>& bytes, std::uint64_t offset, std::uint64_t size) {
  if (offset + size > bytes.size()) {
    raise_error("VENOM-E4000", "truncated package while reading section data");
  }
  if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    raise_error("VENOM-E4000", "section data is too large for this platform");
  }
  const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(offset);
  const auto end = begin + static_cast<std::ptrdiff_t>(size);
  return std::vector<unsigned char>(begin, end);
}

void validate_range(std::uint64_t offset, std::uint64_t size, std::uint64_t file_size, const char* label) {
  if (offset > file_size || size > file_size || offset + size < offset || offset + size > file_size) {
    raise_error("VENOM-E4000", std::string("invalid package ") + label + " range");
  }
}


std::vector<std::string> split_tab(const std::string& line) {
  std::vector<std::string> parts;
  std::string current;
  std::stringstream input(line);
  while (std::getline(input, current, '\t')) {
    parts.push_back(current);
  }
  return parts;
}

bool is_hex_digest(const std::string& value) {
  if (value.size() != 64u) {
    return false;
  }
  for (char ch : value) {
    if (std::isxdigit(static_cast<unsigned char>(ch)) == 0) {
      return false;
    }
  }
  return true;
}

std::string integrity_key(SectionType type, const std::string& name) {
  return std::to_string(static_cast<std::uint32_t>(type)) + "\t" + name;
}



std::string section_text(const Section& section) {
  return std::string(reinterpret_cast<const char*>(section.data.data()), section.data.size());
}

const Section* find_section(const Package& pkg, SectionType type, const std::string& name) {
  for (const auto& section : pkg.sections) {
    if (section.type == type && section_name_matches(type, section.name, name)) {
      return &section;
    }
  }
  return nullptr;
}

void verify_package_features(const Package& pkg) {
  const bool release = (pkg.flags & PackageFlagReleaseProfile) != 0u;
  const auto* feature_section = find_section(pkg, SectionType::PackageFeatures, "package-features.vfeat");
  if (!feature_section) {
    if (release) {
      raise_error("VENOM-E4000", "release package is missing package feature table");
    }
    return;
  }

  const auto text = section_text(*feature_section);
  std::stringstream input(text);
  std::string line;
  bool saw_magic = false;
  bool saw_version = false;
  bool saw_package_version = false;
  bool saw_feature_count = false;
  std::uint64_t declared_features = 0;
  std::uint64_t parsed_features = 0;
  std::uint64_t required_release_features = 0;

  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    if (line == "VENOM_PACKAGE_FEATURES_V2") {
      saw_magic = true;
      continue;
    }
    if (line == "version=2") {
      saw_version = true;
      continue;
    }
    if (line.rfind("package_version=", 0) == 0) {
      const auto version = static_cast<std::uint32_t>(std::stoul(line.substr(16)));
      if (version != kVersion) {
        raise_error("VENOM-E4000", "package feature table version mismatch");
      }
      saw_package_version = true;
      continue;
    }
    if (line.rfind("feature_count=", 0) == 0) {
      declared_features = static_cast<std::uint64_t>(std::stoull(line.substr(14)));
      saw_feature_count = true;
      continue;
    }
    if (line.rfind("feature\t", 0) == 0) {
      const auto parts = split_tab(line);
      if (parts.size() != 8u) {
        raise_error("VENOM-E4000", "invalid package feature table entry");
      }
      const auto raw_type = static_cast<std::uint32_t>(std::stoul(parts[5]));
      if (!is_known_section_type(raw_type)) {
        raise_error("VENOM-E4000", "package feature table references unknown section type");
      }
      if (!is_hex_digest(parts[7])) {
        raise_error("VENOM-E4000", "package feature table has invalid section digest");
      }
      const auto section_type = static_cast<SectionType>(raw_type);
      const auto* section = find_section(pkg, section_type, parts[6]);
      if (!section) {
        raise_error("VENOM-E4000", "package feature table references missing section: " + parts[6]);
      }
      if (sha256_hex(section->data) != parts[7]) {
        raise_error("VENOM-E4000", "package feature table digest mismatch: " + parts[6]);
      }
      if (release && parts[4] == "true") {
        ++required_release_features;
      }
      ++parsed_features;
      continue;
    }
  }

  if (!saw_magic || !saw_version || !saw_package_version || !saw_feature_count) {
    raise_error("VENOM-E4000", "package feature table is missing required fields");
  }
  if (declared_features != parsed_features) {
    raise_error("VENOM-E4000", "package feature table feature count mismatch");
  }
  if (release && required_release_features == 0u) {
    raise_error("VENOM-E4000", "release package feature table has no required release features");
  }
}

void verify_integrity_metadata(const Package& pkg) {
  const Section* metadata = nullptr;
  for (const auto& section : pkg.sections) {
    if (section.type == SectionType::Integrity && section_name_matches(SectionType::Integrity, section.name, "integrity-auth.vsig")) {
      metadata = &section;
      break;
    }
  }

  if (metadata == nullptr) {
    if ((pkg.flags & PackageFlagIntegrityMetadata) != 0u || (pkg.flags & PackageFlagReleaseProfile) != 0u) {
      raise_error("VENOM-E4000", "package is missing integrity-auth.vsig metadata");
    }
    return;
  }

  const std::string text(reinterpret_cast<const char*>(metadata->data.data()), metadata->data.size());
  std::stringstream lines(text);
  std::string line;
  if (!std::getline(lines, line) || line != "VENOM_INTEGRITY_V1") {
    raise_error("VENOM-E4000", "invalid integrity metadata header");
  }

  bool saw_provider = false;
  bool saw_scope = false;
  bool saw_section_count = false;
  std::uint64_t declared_sections = 0;
  std::unordered_map<std::string, std::pair<std::uint64_t, std::string>> expected;

  while (std::getline(lines, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      continue;
    }
    if (line == "provider=sha256-software-v1") {
      saw_provider = true;
      continue;
    }
    if (line == "scope=decoded-sections") {
      saw_scope = true;
      continue;
    }
    if (line.rfind("section_count=", 0) == 0) {
      declared_sections = std::stoull(line.substr(14));
      saw_section_count = true;
      continue;
    }
    if (line.rfind("section\t", 0) == 0) {
      const auto parts = split_tab(line);
      if (parts.size() != 5u) {
        raise_error("VENOM-E4000", "invalid integrity section entry");
      }
      const auto raw_type = static_cast<std::uint32_t>(std::stoul(parts[1]));
      if (!is_known_section_type(raw_type)) {
        raise_error("VENOM-E4000", "integrity metadata references unknown section type");
      }
      const auto size = static_cast<std::uint64_t>(std::stoull(parts[3]));
      if (!is_hex_digest(parts[4])) {
        raise_error("VENOM-E4000", "invalid integrity SHA-256 digest");
      }
      const auto key = integrity_key(static_cast<SectionType>(raw_type), parts[2]);
      if (!expected.emplace(key, std::make_pair(size, parts[4])).second) {
        raise_error("VENOM-E4000", "duplicate integrity metadata entry: " + parts[2]);
      }
      continue;
    }
    // Other key/value lines are forward-compatible diagnostics.
  }

  if (!saw_provider || !saw_scope || !saw_section_count) {
    raise_error("VENOM-E4000", "integrity metadata is missing required fields");
  }
  if (declared_sections != expected.size()) {
    raise_error("VENOM-E4000", "integrity metadata section count mismatch");
  }

  std::uint64_t covered = 0;
  for (const auto& section : pkg.sections) {
    if (section.type == SectionType::Integrity && section_name_matches(SectionType::Integrity, section.name, "integrity-auth.vsig")) {
      continue;
    }
    const auto key = integrity_key(section.type, section.name);
    const auto found = expected.find(key);
    if (found == expected.end()) {
      raise_error("VENOM-E4000", "integrity metadata does not cover section: " + section.name);
    }
    if (found->second.first != section.data.size()) {
      raise_error("VENOM-E4000", "integrity size mismatch: " + section.name);
    }
    const auto actual = sha256_hex(section.data);
    if (actual != found->second.second) {
      raise_error("VENOM-E4000", "integrity SHA-256 mismatch: " + section.name);
    }
    ++covered;
  }
  if (covered != expected.size()) {
    raise_error("VENOM-E4000", "integrity metadata references missing sections");
  }
}
} // namespace

Package read_package_bytes(const std::vector<unsigned char>& bytes) {
  if (bytes.size() < kHeaderSize) {
    raise_error("VENOM-E4000", "package is smaller than the VBC header");
  }
  if (!std::equal(kMagic, kMagic + sizeof(kMagic), bytes.begin())) {
    raise_error("VENOM-E4000", "invalid package magic");
  }

  Package pkg;
  pkg.file_size = static_cast<std::uint64_t>(bytes.size());
  pkg.version = read_u32_at(bytes, 8);
  pkg.runtime_abi = read_u32_at(bytes, 12);
  pkg.flags = read_u32_at(bytes, 16);
  const auto section_count = read_u32_at(bytes, 20);
  pkg.section_table_offset = read_u64_at(bytes, 24);
  pkg.section_table_size = read_u64_at(bytes, 32);
  pkg.name_table_offset = read_u64_at(bytes, 40);
  pkg.name_table_size = read_u64_at(bytes, 48);
  pkg.payload_offset = read_u64_at(bytes, 56);
  pkg.payload_size = read_u64_at(bytes, 64);
  pkg.package_hash = read_u64_at(bytes, kPackageHashOffset);

  if (pkg.version != kVersion) {
    raise_error("VENOM-E4000", "unsupported package version: " + std::to_string(pkg.version));
  }
  if (pkg.runtime_abi != kRuntimeAbi) {
    raise_error("VENOM-E4000", "unsupported runtime ABI: " + std::to_string(pkg.runtime_abi));
  }
  const std::uint32_t known_package_flags = PackageFlagProtectProfile | PackageFlagReleaseProfile |
                                            PackageFlagPolymorphic | PackageFlagCompressedSections |
                                            PackageFlagCryptoProviderReady | PackageFlagIntegrityMetadata |
                                            PackageFlagAeadProviderReady |
                                            PackageFlagRuntimeHardened |
                                            PackageFlagWasmRuntime |
                                            PackageFlagHostBridge |
                                            PackageFlagBinaryDomOps |
                                            PackageFlagFetchBridge |
                                            PackageFlagAsyncHostQueue |
                                            PackageFlagTimerBridge |
                                            PackageFlagEventQueue |
                                            PackageFlagQuickJsBridge |
                                            PackageFlagScriptIsolation |
                                            PackageFlagScriptPolicy |
                                            PackageFlagQuickJsChunks |
                                            PackageFlagQuickJsEngine |
                                            PackageFlagScriptEngineFallback |
                                            PackageFlagQuickJsEngineModule |
                                            PackageFlagQuickJsContextLifecycle |
                                            PackageFlagHostCapabilities |
                                            PackageFlagQuickJsAdapterDiagnostics |
                                            PackageFlagQuickJsWasmRuntime |
                                            PackageFlagQuickJsSourceTransfer |
                                            PackageFlagQuickJsConsoleBridge |
                                            PackageFlagQuickJsExecutionRecords |
                                            PackageFlagQuickJsResultBridge |
                                            PackageFlagQuickJsFallbackPolicy |
                                            PackageFlagQuickJsEngineBackend;
  if ((pkg.flags & ~known_package_flags) != 0u) {
    raise_error("VENOM-E4000", "unknown package flags: " + std::to_string(pkg.flags & ~known_package_flags));
  }
  if ((pkg.flags & PackageFlagReleaseProfile) != 0u) {
    if ((pkg.flags & PackageFlagPolymorphic) == 0u) {
      raise_error("VENOM-E4000", "release package is missing the polymorphic flag");
    }
    if ((pkg.flags & PackageFlagCryptoProviderReady) == 0u) {
      raise_error("VENOM-E4000", "release package is missing the crypto-provider-ready flag");
    }
    if ((pkg.flags & PackageFlagIntegrityMetadata) == 0u) {
      raise_error("VENOM-E4000", "release package is missing integrity metadata");
    }
    if ((pkg.flags & PackageFlagRuntimeHardened) == 0u) {
      raise_error("VENOM-E4000", "release package is missing runtime hardening metadata");
    }
  }
  if (pkg.section_table_offset != kHeaderSize) {
    raise_error("VENOM-E4000", "invalid package section table offset");
  }
  if (pkg.section_table_size != static_cast<std::uint64_t>(section_count) * kSectionEntrySize) {
    raise_error("VENOM-E4000", "invalid package section table size");
  }

  validate_range(pkg.section_table_offset, pkg.section_table_size, pkg.file_size, "section table");
  validate_range(pkg.name_table_offset, pkg.name_table_size, pkg.file_size, "name table");
  validate_range(pkg.payload_offset, pkg.payload_size, pkg.file_size, "payload");

  auto bytes_for_hash = bytes;
  for (std::uint64_t i = 0; i < 8; ++i) {
    bytes_for_hash[static_cast<std::size_t>(kPackageHashOffset + i)] = 0;
  }
  if (fnv1a64(bytes_for_hash) != pkg.package_hash) {
    raise_error("VENOM-E4000", "package hash mismatch");
  }

  constexpr std::uint32_t kMaxSections = 4096u;
  constexpr std::uint64_t kMaxPackageBytes = 64ull * 1024ull * 1024ull;
  constexpr std::uint64_t kMaxNameTableBytes = 1ull * 1024ull * 1024ull;
  constexpr std::uint64_t kMaxDecodedSectionBytes = 32ull * 1024ull * 1024ull;
  constexpr std::uint64_t kMaxDecodedPackageBytes = 128ull * 1024ull * 1024ull;
  if (pkg.file_size > kMaxPackageBytes) {
    raise_error("VENOM-E4000", "package exceeds maximum encoded size");
  }
  if (section_count > kMaxSections) {
    raise_error("VENOM-E4000", "package section count exceeds limit");
  }
  if (pkg.name_table_size > kMaxNameTableBytes) {
    raise_error("VENOM-E4000", "package name table exceeds limit");
  }

  std::uint64_t total_decoded_size = 0;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> payload_ranges;
  payload_ranges.reserve(section_count);
  std::unordered_set<std::string> section_identities;
  pkg.sections.reserve(section_count);
  for (std::uint32_t i = 0; i < section_count; ++i) {
    const auto entry_offset = pkg.section_table_offset + static_cast<std::uint64_t>(i) * kSectionEntrySize;
    const auto raw_type = read_u32_at(bytes, entry_offset + 0u);
    if (!is_known_section_type(raw_type)) {
      raise_error("VENOM-E4000", "unknown package section type: " + std::to_string(raw_type));
    }
    const auto flags = read_u32_at(bytes, entry_offset + 4u);
    const std::uint32_t known_section_flags = SectionFlagCompressed | SectionFlagEncrypted;
    if ((flags & ~known_section_flags) != 0u) {
      raise_error("VENOM-E4000", "unknown section flags: " + std::to_string(flags & ~known_section_flags));
    }
    const auto name_offset = read_u32_at(bytes, entry_offset + 8u);
    const auto name_size = read_u32_at(bytes, entry_offset + 12u);
    const auto data_offset = read_u64_at(bytes, entry_offset + 16u);
    const auto data_size = read_u64_at(bytes, entry_offset + 24u);
    const auto raw_size = read_u64_at(bytes, entry_offset + 32u);
    const auto expected_hash = read_u64_at(bytes, entry_offset + 40u);

    if (raw_size > kMaxDecodedSectionBytes) {
      raise_error("VENOM-E4000", "section decoded size exceeds limit");
    }
    if (total_decoded_size > kMaxDecodedPackageBytes - raw_size) {
      raise_error("VENOM-E4000", "package decoded size exceeds limit");
    }
    total_decoded_size += raw_size;

    validate_range(pkg.name_table_offset + name_offset, name_size, pkg.file_size, "section name");
    validate_range(data_offset, data_size, pkg.file_size, "section data");
    if (data_offset < pkg.payload_offset || data_offset + data_size > pkg.payload_offset + pkg.payload_size) {
      raise_error("VENOM-E4000", "section data is outside the payload range");
    }
    const bool encrypted = (flags & SectionFlagEncrypted) != 0u;
    if ((flags & SectionFlagCompressed) != 0u) {
      if (!encrypted && raw_size < data_size) {
        raise_error("VENOM-E4000", "compressed section raw size is smaller than stored size");
      }
    } else if (!encrypted && raw_size != data_size) {
      raise_error("VENOM-E4000", "uncompressed section raw size does not match stored size");
    }

    Section section;
    section.type = static_cast<SectionType>(raw_type);
    section.flags = flags;
    section.name = read_string_at(bytes, pkg.name_table_offset + name_offset, name_size);
    validate_section_name(section.name);
    const auto identity = std::to_string(raw_type) + "\t" + section.name;
    if (!section_identities.insert(identity).second) {
      raise_error("VENOM-E4000", "duplicate package section identity");
    }
    const auto range_end = data_offset + data_size;
    for (const auto& range : payload_ranges) {
      if (data_offset < range.second && range.first < range_end) {
        raise_error("VENOM-E4000", "overlapping package section payloads");
      }
    }
    if (data_size != 0u) {
      payload_ranges.emplace_back(data_offset, range_end);
    }
    auto stored_data = read_bytes_at(bytes, data_offset, data_size);
    section.hash = expected_hash;
    section.offset = data_offset;
    section.stored_size = data_size;
    section.raw_size = raw_size;

    if (fnv1a64(stored_data) != expected_hash) {
      raise_error("VENOM-E4000", "package section hash mismatch: " + section.name);
    }
    if ((flags & SectionFlagEncrypted) != 0u) {
      stored_data = open_section_v1(section.type, section.name, stored_data);
    }
    if ((flags & SectionFlagCompressed) != 0u) {
      if (raw_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        raise_error("VENOM-E4000", "compressed section raw size is too large for this platform");
      }
      section.data = decompress_rle_v1(stored_data, static_cast<std::size_t>(raw_size));
    } else {
      if (stored_data.size() != raw_size) {
        raise_error("VENOM-E4000", "section decoded size mismatch: " + section.name);
      }
      section.data = std::move(stored_data);
    }

    pkg.sections.push_back(std::move(section));
  }

  verify_package_features(pkg);
  verify_integrity_metadata(pkg);

  return pkg;
}

Package read_package_bytes(const unsigned char* data, std::size_t size) {
  if (data == nullptr && size != 0u) {
    raise_error("VENOM-E4000", "null package byte pointer");
  }
  if (size == 0u) {
    return read_package_bytes(std::vector<unsigned char>{});
  }
  return read_package_bytes(std::vector<unsigned char>(data, data + size));
}

Package read_package(const std::filesystem::path& path) {
  return read_package_bytes(read_file(path));
}

} // namespace venom::package
