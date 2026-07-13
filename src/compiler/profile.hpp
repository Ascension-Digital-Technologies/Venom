#pragma once

#include <cstdint>
#include <string>

namespace venom::compiler {

enum class ProfileKind {
  Debug,
  Protect,
  Release,
};

struct Profile {
  ProfileKind kind;
  std::string name;
  std::uint32_t package_flags;
  bool deterministic;
  bool polymorphic;
  bool shuffle_sections;
  bool compress_sections;
  bool fail_closed;
  bool crypto_provider_ready;
  bool integrity_metadata;
  bool aead_provider_ready;
  bool runtime_hardened;
  bool strip_debug_metadata;
};

Profile resolve_profile(const std::string& name);
std::string describe_profile(const Profile& profile);

} // namespace venom::compiler
