#include "base/error.hpp"
#include "core/profile.hpp"

#include "package/format.hpp"

#include <sstream>
#include <stdexcept>

namespace venom::compiler {

Profile resolve_profile(const std::string& name) {
  if (name == "prod") {
    return {ProfileKind::Prod, name,
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
  if (name == "dev") raise_error("VENOM-E1100", "the dev build profile was removed in Venom 2.0.0; use prod (venom dev now serves production-grade builds)");
  raise_error("VENOM-E1100", "unknown profile: " + name + "; expected prod");
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
