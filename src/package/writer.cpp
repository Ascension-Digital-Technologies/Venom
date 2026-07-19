#include "venom/base/error.hpp"
#include "venom/package/writer.hpp"

#include "compress.hpp"
#include "venom/package/crypto.hpp"
#include "venom/package/hash.hpp"

#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace venom::package {

namespace {
void append_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

void append_u64(std::vector<unsigned char>& out, std::uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<unsigned char>((value >> (i * 8)) & 0xffu));
  }
}

void put_u64_at(std::vector<unsigned char>& out, std::size_t offset, std::uint64_t value) {
  if (offset + 8u > out.size()) {
    raise_error("VENOM-E4000", "internal package hash offset out of range");
  }
  for (int i = 0; i < 8; ++i) {
    out[offset + static_cast<std::size_t>(i)] = static_cast<unsigned char>((value >> (i * 8)) & 0xffu);
  }
}

std::uint32_t checked_u32(std::uint64_t value, const char* label) {
  if (value > std::numeric_limits<std::uint32_t>::max()) {
    raise_error("VENOM-E4000", std::string(label) + " does not fit in u32");
  }
  return static_cast<std::uint32_t>(value);
}

std::uint64_t align8(std::uint64_t value) {
  return (value + 7u) & ~std::uint64_t{7u};
}

void append_padding_to(std::vector<unsigned char>& out, std::uint64_t offset) {
  while (out.size() < offset) {
    out.push_back(0u);
  }
}

void append_polymorphic_padding_to(std::vector<unsigned char>& out, std::uint64_t offset, std::mt19937& rng) {
  while (out.size() < offset) {
    out.push_back(static_cast<unsigned char>(rng() & 0xffu));
  }
}
} // namespace

void Writer::set_flags(std::uint32_t flags) {
  flags_ = flags;
}

void Writer::set_compression_enabled(bool enabled) {
  compression_enabled_ = enabled;
}

void Writer::set_section_encryption_enabled(bool enabled) {
  section_encryption_enabled_ = enabled;
}

void Writer::set_section_name_redaction_enabled(bool enabled) {
  section_name_redaction_enabled_ = enabled;
}

void Writer::set_layout_polymorphism_enabled(bool enabled) {
  layout_polymorphism_enabled_ = enabled;
}

void Writer::set_layout_seed(std::uint32_t seed) {
  layout_seed_ = seed;
}

void Writer::set_layout_max_padding(std::uint32_t max_padding) {
  layout_max_padding_ = max_padding;
}

void Writer::set_section_crypto_provider(SectionCryptoProvider provider) {
  section_crypto_provider_ = provider;
}

void Writer::add_section(SectionType type, std::string name, std::vector<unsigned char> data) {
  add_section(type, SectionFlagNone, std::move(name), std::move(data));
}

void Writer::add_section(SectionType type, std::uint32_t flags, std::string name, std::vector<unsigned char> data) {
  validate_section_name(name);
  sections_.push_back({type, flags, std::move(name), std::move(data), 0, 0, 0, 0});
}

void Writer::write(const std::filesystem::path& path) const {
  if (sections_.size() > std::numeric_limits<std::uint32_t>::max()) {
    raise_error("VENOM-E4000", "too many package sections");
  }

  std::vector<Section> sections = sections_;
  std::vector<unsigned char> names;
  names.reserve(sections.size() * 24u);

  struct NameSpan {
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
  };
  std::vector<NameSpan> name_spans;
  name_spans.reserve(sections.size());

  for (auto& section : sections) {
    section.raw_size = static_cast<std::uint64_t>(section.data.size());
    section.stored_size = section.raw_size;

    if (compression_enabled_ &&
        (section.flags & SectionFlagCompressed) == 0u &&
        compression_is_worthwhile(section.data)) {
      auto compressed = compress_rle_v1(section.data);
      if (compressed.size() < section.data.size()) {
        section.data = std::move(compressed);
        section.flags |= SectionFlagCompressed;
      }
    }

    if (section_name_redaction_enabled_ && should_redact_section_name(section.type)) {
      section.name = protected_section_alias(section.type, section.name);
    }

    if (section_encryption_enabled_ && (section.flags & SectionFlagEncrypted) != 0u) {
      if (section_crypto_provider_ == SectionCryptoProvider::LibsodiumXChaCha20Poly1305) {
        section.data = seal_section_libsodium_v1(section.type, section.name, section.data);
      } else {
        section.data = seal_section_v1(section.type, section.name, section.data);
      }
    } else if ((section.flags & SectionFlagEncrypted) != 0u) {
      raise_error("VENOM-E4000", "encrypted package section requested without a section sealer: " + section.name);
    }

    section.stored_size = static_cast<std::uint64_t>(section.data.size());
    section.hash = fnv1a64(section.data);
    const auto name_offset = checked_u32(names.size(), "section name table offset");
    const auto name_size = checked_u32(section.name.size(), "section name size");
    names.insert(names.end(), section.name.begin(), section.name.end());
    name_spans.push_back({name_offset, name_size});
  }

  const std::uint64_t section_table_offset = kHeaderSize;
  const std::uint64_t section_table_size = static_cast<std::uint64_t>(sections.size()) * kSectionEntrySize;
  const std::uint64_t name_table_offset = section_table_offset + section_table_size;
  const std::uint64_t name_table_size = names.size();
  const std::uint64_t payload_offset = align8(name_table_offset + name_table_size);

  const std::uint32_t layout_seed = layout_seed_ != 0u ? layout_seed_ : 0xA45C0DEu;
  std::mt19937 layout_rng(layout_seed ^ 0x9E3779B9u);
  std::uint64_t cursor = payload_offset;
  for (auto& section : sections) {
    if (layout_polymorphism_enabled_ && layout_max_padding_ != 0u) {
      cursor += static_cast<std::uint64_t>(layout_rng() % (layout_max_padding_ + 1u));
    }
    cursor = align8(cursor);
    section.offset = cursor;
    cursor += static_cast<std::uint64_t>(section.data.size());
    cursor = align8(cursor);
  }
  const std::uint64_t payload_size = cursor - payload_offset;

  std::vector<unsigned char> bytes;
  bytes.reserve(static_cast<std::size_t>(cursor));
  bytes.insert(bytes.end(), kMagic, kMagic + sizeof(kMagic));
  append_u32(bytes, kVersion);
  append_u32(bytes, kRuntimeAbi);
  append_u32(bytes, flags_);
  append_u32(bytes, static_cast<std::uint32_t>(sections.size()));
  append_u64(bytes, section_table_offset);
  append_u64(bytes, section_table_size);
  append_u64(bytes, name_table_offset);
  append_u64(bytes, name_table_size);
  append_u64(bytes, payload_offset);
  append_u64(bytes, payload_size);
  append_u64(bytes, 0); // package hash is computed after the full image is assembled.

  if (bytes.size() != kHeaderSize) {
    raise_error("VENOM-E4000", "internal package header size mismatch");
  }

  for (std::size_t i = 0; i < sections.size(); ++i) {
    const auto& section = sections[i];
    append_u32(bytes, static_cast<std::uint32_t>(section.type));
    append_u32(bytes, section.flags);
    append_u32(bytes, name_spans[i].offset);
    append_u32(bytes, name_spans[i].size);
    append_u64(bytes, section.offset);
    append_u64(bytes, section.stored_size);
    append_u64(bytes, section.raw_size);
    append_u64(bytes, section.hash);
  }

  bytes.insert(bytes.end(), names.begin(), names.end());
  if (layout_polymorphism_enabled_) {
    std::mt19937 padding_rng(layout_seed ^ 0xBADC0DEu);
    append_polymorphic_padding_to(bytes, payload_offset, padding_rng);
    for (const auto& section : sections) {
      if (bytes.size() != section.offset) {
        append_polymorphic_padding_to(bytes, section.offset, padding_rng);
      }
      bytes.insert(bytes.end(), section.data.begin(), section.data.end());
      append_polymorphic_padding_to(bytes, align8(bytes.size()), padding_rng);
    }
  } else {
    append_padding_to(bytes, payload_offset);
    for (const auto& section : sections) {
      if (bytes.size() != section.offset) {
        append_padding_to(bytes, section.offset);
      }
      bytes.insert(bytes.end(), section.data.begin(), section.data.end());
      append_padding_to(bytes, align8(bytes.size()));
    }
  }

  const std::uint64_t package_hash = fnv1a64(bytes);
  put_u64_at(bytes, static_cast<std::size_t>(kPackageHashOffset), package_hash);

  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    raise_error("VENOM-E4000", "failed to write package " + path.string());
  }
  out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!out) {
    raise_error("VENOM-E4000", "failed while writing package " + path.string());
  }
}

} // namespace venom::package
