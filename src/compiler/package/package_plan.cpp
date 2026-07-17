#include "compiler/package/package_plan.hpp"

#include "package/format.hpp"
#include "vm/polymorph.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace venom::compiler::package_plan {

void Plan::add(venom::package::SectionType type,
               std::string name,
               std::vector<unsigned char> data,
               std::uint32_t flags) {
  sections_.push_back(Section{type, flags, std::move(name), std::move(data)});
}

std::size_t Plan::size() const noexcept { return sections_.size(); }
bool Plan::empty() const noexcept { return sections_.empty(); }
const std::vector<Section>& Plan::sections() const noexcept { return sections_; }
std::vector<Section>& Plan::sections() noexcept { return sections_; }

void Plan::shuffle(const venom::vm::PolymorphicPlan& polymorphic_plan) {
  if (!polymorphic_plan.enabled || sections_.size() < 3u) return;
  venom::vm::DiversificationRng rng(polymorphic_plan, "package-section-order");
  std::shuffle(sections_.begin(), sections_.end(), rng);
}

Artifact Plan::write_verified(const std::filesystem::path& path,
                              const WriterOptions& options) {
  if (sections_.empty()) throw std::runtime_error("cannot write an empty package plan");

  venom::package::Writer writer;
  writer.set_flags(options.package_flags);
  writer.set_compression_enabled(options.compression_enabled);
  writer.set_section_encryption_enabled(options.section_encryption_enabled);
  writer.set_section_name_redaction_enabled(options.section_name_redaction_enabled);
  writer.set_layout_polymorphism_enabled(options.layout_polymorphism_enabled);
  writer.set_layout_seed(options.layout_seed);
  writer.set_layout_max_padding(options.layout_max_padding);
  writer.set_section_crypto_provider(options.crypto_provider);

  for (auto& section : sections_) {
    writer.add_section(section.type, section.flags, std::move(section.name), std::move(section.data));
  }
  writer.write(path);

  Artifact artifact;
  artifact.path = path;
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("failed to reopen package artifact: " + path.string());
  artifact.bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  artifact.package = venom::package::read_package(path);
  if (artifact.package.sections.empty()) {
    throw std::runtime_error("package writer produced no readable sections");
  }
  return artifact;
}

} // namespace venom::compiler::package_plan
