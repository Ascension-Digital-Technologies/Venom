#pragma once

#include "package/format.hpp"

#include <string>
#include <vector>

namespace venom::package {

class IntegrityProvider {
public:
  virtual ~IntegrityProvider() = default;
  virtual std::string name() const = 0;
  virtual std::string digest_hex(const std::vector<unsigned char>& data) const = 0;
};

class Sha256IntegrityProvider final : public IntegrityProvider {
public:
  std::string name() const override;
  std::string digest_hex(const std::vector<unsigned char>& data) const override;
};

class AeadProvider {
public:
  virtual ~AeadProvider() = default;
  virtual std::string name() const = 0;
  virtual bool available() const = 0;
  virtual std::vector<unsigned char> seal(const std::vector<unsigned char>& plaintext) = 0;
  virtual std::vector<unsigned char> open(const std::vector<unsigned char>& ciphertext) = 0;
};

class NoAeadProvider final : public AeadProvider {
public:
  std::string name() const override;
  bool available() const override;
  std::vector<unsigned char> seal(const std::vector<unsigned char>& plaintext) override;
  std::vector<unsigned char> open(const std::vector<unsigned char>& ciphertext) override;
};

// Backwards-compatible alias for older call sites; v0.10 keeps this boundary
// AEAD explicitly so compression/integrity are not confused with encryption.
using CryptoProvider = AeadProvider;
using NoCryptoProvider = NoAeadProvider;

// Section sealing boundary. The default browser-runnable provider remains
// runtime-decodable. The optional libsodium provider uses audited
// XChaCha20-Poly1305-IETF and requires VENOM_PACKAGE_KEY at build/open time.
std::vector<unsigned char> seal_section_v1(SectionType type,
                                           const std::string& name,
                                           const std::vector<unsigned char>& plaintext);
std::vector<unsigned char> seal_section_libsodium_v1(SectionType type,
                                                     const std::string& name,
                                                     const std::vector<unsigned char>& plaintext);
bool libsodium_aead_available();
std::string libsodium_aead_provider_name();
void set_package_key_hex_override(const std::string& hex_key);
void clear_package_key_hex_override();
std::string normalize_package_key_hex(const std::string& input);
std::vector<unsigned char> open_section_v1(SectionType type,
                                           const std::string& name,
                                           const std::vector<unsigned char>& sealed);
bool is_sealed_section_v1(const std::vector<unsigned char>& bytes);

} // namespace venom::package
