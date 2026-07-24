#include "pipeline/build_report.hpp"
#include "pipeline/build_package_metadata.hpp"
#include "package/format.hpp"

#include <algorithm>
#include <iostream>
#include <string>

namespace venom::compiler::build_report_detail {
using namespace build_package_detail;
std::size_t count_marker(const std::vector<unsigned char>& bytes, const std::string& marker) {
  if (marker.empty() || bytes.size() < marker.size()) {
    return 0;
  }
  std::size_t count = 0;
  auto it = bytes.begin();
  while ((it = std::search(it, bytes.end(), marker.begin(), marker.end())) != bytes.end()) {
    ++count;
    ++it;
  }
  return count;
}
void print_build_protection_report(const Profile& profile,
                                   const BuildOptions& options,
                                   const std::filesystem::path& package_path,
                                   const std::vector<unsigned char>& package_bytes,
                                   const venom::package::Package& package,
                                   bool external_manifest) {
  std::size_t encrypted_sections = 0;
  std::size_t compressed_sections = 0;
  bool manifest_section = false;
  bool package_binding = false;
  bool release_profile = false;
  bool layout_polymorphic = false;
  bool lazy_sections = false;
  std::size_t turbojs_bytecode_records = 0;
  for (const auto& section : package.sections) {
    if ((section.flags & venom::package::SectionFlagEncrypted) != 0u) {
      ++encrypted_sections;
    }
    if ((section.flags & venom::package::SectionFlagCompressed) != 0u) {
      ++compressed_sections;
    }
    if (section.type == venom::package::SectionType::Manifest && section.name == "manifest.txt") {
      manifest_section = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-binding.vbind")) {
      package_binding = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "release-profile.vrpf")) {
      release_profile = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "package-layout.vlay")) {
      layout_polymorphic = true;
    }
    if (section.type == venom::package::SectionType::Integrity && venom::package::section_name_matches(section.type, section.name, "lazy-sections.vlazy")) {
      lazy_sections = true;
    }
    if (section.type == venom::package::SectionType::JavaScript) {
      turbojs_bytecode_records += count_marker(section.data, "VTJSE006");
    }
  }
  const auto runtime_sections = count_marker(package_bytes, "VAEAD001");
  const auto sodium_sections = count_marker(package_bytes, "VSODIUM1");
  const auto legacy_sections = count_marker(package_bytes, "VSEAL001");
  const bool audited = sodium_sections != 0u;
  const bool browser_runnable = sodium_sections == 0u;
  const bool release_pass = 
                            encrypted_sections != 0u &&
                            legacy_sections == 0u &&
                            !external_manifest &&
                            !manifest_section &&
                            package_binding &&
                            release_profile &&
                            (!options.require_audited_crypto || audited);
  if (options.verbosity > 1) {
  std::cout << "Protection report:\n"
            << "  profile: " << profile.name << "\n"
            << "  target: " << options.security_target << "\n"
            << "  package: " << package_path.string() << "\n"
            << "  protected_sections: " << encrypted_sections << "\n"
            << "  compressed_sections: " << compressed_sections << "\n"
            << "  provider: " << selected_section_sealer_name(profile, options.crypto_provider) << "\n"
            << "  provider_runtime_sections: " << runtime_sections << "\n"
            << "  provider_libsodium_sections: " << sodium_sections << "\n"
            << "  provider_legacy_sections: " << legacy_sections << "\n"
            << "  turbojs_bytecode_records: " << turbojs_bytecode_records << "\n"
            << "  package_binding: " << (package_binding ? "yes" : "no") << "\n"
            << "  release_profile: " << (release_profile ? "yes" : "no") << "\n"
            << "  layout_polymorphic: " << (layout_polymorphic ? "yes" : "no") << "\n"
            << "  lazy_sections: " << (lazy_sections ? "yes" : "no") << "\n"
            << "  debug_metadata: " << (profile.strip_debug_metadata ? "stripped" : "present") << "\n"
            << "  external_debug_manifest: " << (external_manifest ? "yes" : "no") << "\n"
            << "  package_manifest_section: " << (manifest_section ? "yes" : "no") << "\n"
            << "  browser_executable: " << (browser_runnable ? "yes" : "no") << "\n"
            << "  native_private_crypto: " << (audited ? "yes" : "no") << "\n"
            << "  release_status: " << (release_pass ? "PASS" : "CHECK") << "\n";
  }
}

} // namespace venom::compiler::build_report_detail
