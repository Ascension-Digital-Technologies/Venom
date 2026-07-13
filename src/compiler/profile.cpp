#include "compiler/profile.hpp"

#include "package/format.hpp"

#include <sstream>
#include <stdexcept>

namespace venom::compiler {

Profile resolve_profile(const std::string& name) {
  if (name == "debug") {
    return {ProfileKind::Debug, name, venom::package::PackageFlagNone,
            true, false, false, false, false, false, false, false, false, false};
  }
  if (name == "protect") {
    return {ProfileKind::Protect, name,
            venom::package::PackageFlagProtectProfile |
              venom::package::PackageFlagReleaseProfile |
              venom::package::PackageFlagPolymorphic |
              venom::package::PackageFlagCompressedSections |
              venom::package::PackageFlagCryptoProviderReady |
              venom::package::PackageFlagIntegrityMetadata |
              venom::package::PackageFlagAeadProviderReady |
              venom::package::PackageFlagRuntimeHardened |
              venom::package::PackageFlagHostBridge |
              venom::package::PackageFlagFetchBridge |
              venom::package::PackageFlagAsyncHostQueue,
            false, true, true, true, false, true, true, true, true, true};
  }
  if (name == "browser-protect") {
    return {ProfileKind::Protect, name,
            venom::package::PackageFlagProtectProfile |
              venom::package::PackageFlagReleaseProfile |
              venom::package::PackageFlagPolymorphic |
              venom::package::PackageFlagCompressedSections |
              venom::package::PackageFlagCryptoProviderReady |
              venom::package::PackageFlagIntegrityMetadata |
              venom::package::PackageFlagAeadProviderReady |
              venom::package::PackageFlagRuntimeHardened |
              venom::package::PackageFlagHostBridge |
              venom::package::PackageFlagFetchBridge |
              venom::package::PackageFlagAsyncHostQueue,
            false, true, true, true, true, true, true, true, true, true};
  }
  if (name == "native-protect") {
    return {ProfileKind::Protect, name,
            venom::package::PackageFlagProtectProfile |
              venom::package::PackageFlagReleaseProfile |
              venom::package::PackageFlagPolymorphic |
              venom::package::PackageFlagCompressedSections |
              venom::package::PackageFlagCryptoProviderReady |
              venom::package::PackageFlagIntegrityMetadata |
              venom::package::PackageFlagAeadProviderReady |
              venom::package::PackageFlagRuntimeHardened |
              venom::package::PackageFlagHostBridge |
              venom::package::PackageFlagFetchBridge |
              venom::package::PackageFlagAsyncHostQueue,
            false, true, true, true, false, true, true, true, true, true};
  }
  if (name == "release") {
    return {ProfileKind::Release, name,
            venom::package::PackageFlagReleaseProfile |
              venom::package::PackageFlagPolymorphic |
              venom::package::PackageFlagCompressedSections |
              venom::package::PackageFlagCryptoProviderReady |
              venom::package::PackageFlagIntegrityMetadata |
              venom::package::PackageFlagAeadProviderReady |
              venom::package::PackageFlagRuntimeHardened |
              venom::package::PackageFlagHostBridge |
              venom::package::PackageFlagFetchBridge |
              venom::package::PackageFlagAsyncHostQueue,
            false, true, true, true, true, true, true, true, true, true};
  }
  throw std::runtime_error("unknown profile: " + name);
}

std::string describe_profile(const Profile& profile) {
  std::ostringstream out;
  out << "profile=" << profile.name << '\n'
      << "deterministic=" << (profile.deterministic ? "true" : "false") << '\n'
      << "polymorphic=" << (profile.polymorphic ? "true" : "false") << '\n'
      << "shuffle_sections=" << (profile.shuffle_sections ? "true" : "false") << '\n'
      << "compress_sections=" << (profile.compress_sections ? "true" : "false") << '\n'
      << "fail_closed=" << (profile.fail_closed ? "true" : "false") << '\n'
      << "integrity_provider_ready=" << (profile.integrity_metadata ? "true" : "false") << '\n'
      << "integrity_metadata=" << (profile.integrity_metadata ? "true" : "false") << '\n'
      << "aead_provider_ready=" << (profile.aead_provider_ready ? "true" : "false") << '\n'
      << "signature_provider_ready=false" << '\n'
      << "runtime_hardened=" << (profile.runtime_hardened ? "true" : "false") << '\n'
      << "strip_debug_metadata=" << (profile.strip_debug_metadata ? "true" : "false") << '\n';
  return out.str();
}

} // namespace venom::compiler
