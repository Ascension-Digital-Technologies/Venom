#pragma once

#include "package/format.hpp"
#include "package/reader.hpp"
#include "package/writer.hpp"
#include "vm/polymorph.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler::package_plan {

struct Section {
  venom::package::SectionType type = venom::package::SectionType::Manifest;
  std::uint32_t flags = venom::package::SectionFlagNone;
  std::string name;
  std::vector<unsigned char> data;
};

struct WriterOptions {
  std::uint32_t package_flags = 0u;
  bool compression_enabled = false;
  bool section_encryption_enabled = false;
  bool section_name_redaction_enabled = false;
  bool layout_polymorphism_enabled = false;
  std::uint32_t layout_seed = 0u;
  std::uint32_t layout_max_padding = 0u;
  venom::package::SectionCryptoProvider crypto_provider = venom::package::SectionCryptoProvider::RuntimeSoftware;
};

struct Artifact {
  std::filesystem::path path;
  std::vector<unsigned char> bytes;
  venom::package::Package package;
};

class Plan {
 public:
  void add(venom::package::SectionType type,
           std::string name,
           std::vector<unsigned char> data,
           std::uint32_t flags = venom::package::SectionFlagNone);

  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] const std::vector<Section>& sections() const noexcept;
  [[nodiscard]] std::vector<Section>& sections() noexcept;

  void shuffle(const venom::vm::PolymorphicPlan& polymorphic_plan);
  [[nodiscard]] Artifact write_verified(const std::filesystem::path& path,
                                        const WriterOptions& options);

 private:
  std::vector<Section> sections_;
};

} // namespace venom::compiler::package_plan
