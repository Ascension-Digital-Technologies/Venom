#pragma once

#include "venom/package/format.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace venom::package {

enum class SectionCryptoProvider {
  RuntimeSoftware,
  LibsodiumXChaCha20Poly1305,
};

struct Section {
  SectionType type = SectionType::Manifest;
  std::uint32_t flags = SectionFlagNone;
  std::string name;
  std::vector<unsigned char> data;
  std::uint64_t hash = 0;
  std::uint64_t offset = 0;
  std::uint64_t raw_size = 0;
  std::uint64_t stored_size = 0;
};

class Writer {
public:
  void set_flags(std::uint32_t flags);
  void set_compression_enabled(bool enabled);
  void set_section_encryption_enabled(bool enabled);
  void set_section_name_redaction_enabled(bool enabled);
  void set_layout_polymorphism_enabled(bool enabled);
  void set_layout_seed(std::uint32_t seed);
  void set_layout_max_padding(std::uint32_t max_padding);
  void set_section_crypto_provider(SectionCryptoProvider provider);
  void add_section(SectionType type, std::string name, std::vector<unsigned char> data);
  void add_section(SectionType type, std::uint32_t flags, std::string name, std::vector<unsigned char> data);
  void write(const std::filesystem::path& path) const;

private:
  std::uint32_t flags_ = 0;
  bool compression_enabled_ = false;
  bool section_encryption_enabled_ = false;
  bool section_name_redaction_enabled_ = false;
  bool layout_polymorphism_enabled_ = false;
  std::uint32_t layout_seed_ = 0;
  std::uint32_t layout_max_padding_ = 31;
  SectionCryptoProvider section_crypto_provider_ = SectionCryptoProvider::RuntimeSoftware;
  std::vector<Section> sections_;
};

} // namespace venom::package
