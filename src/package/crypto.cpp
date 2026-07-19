#include "venom/base/error.hpp"
#include "venom/package/crypto.hpp"

#include "venom/package/hash.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace venom::package {

namespace {
constexpr unsigned char kLegacySealMagic[8] = {'V','S','E','A','L','0','0','1'};
constexpr unsigned char kAeadMagic[8] = {'V','A','E','A','D','0','0','1'};
constexpr unsigned char kSodiumMagic[8] = {'V','S','O','D','I','U','M','1'};
constexpr std::uint32_t kSealVersion = 1u;
constexpr std::uint64_t kSealSecretA = 0x7f4a7c159e3779b9ull;
constexpr std::uint64_t kSealSecretB = 0xd6e8feb86659fd93ull;
constexpr std::uint64_t kAeadKeyA = 0x3d6f0a9b8c71e245ull;
constexpr std::uint64_t kAeadKeyB = 0xa41bc927f35d69e1ull;
constexpr std::uint64_t kAeadKeyC = 0x6c8e9cf570932bd5ull;
constexpr std::uint64_t kAeadKeyD = 0x9b62d14fa7c8305dull;
constexpr std::size_t kLegacySealHeaderSize = 40u;
constexpr std::size_t kAeadHeaderSize = 64u;
constexpr std::size_t kSodiumNonceSize = 24u;
constexpr std::size_t kSodiumKeySize = 32u;
constexpr std::size_t kSodiumTagSize = 16u;
constexpr std::size_t kSodiumHeaderSize = 8u + 4u + 4u + 8u + 8u + kSodiumNonceSize;

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

std::uint32_t read_u32(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 4u > bytes.size()) {
    raise_error("VENOM-E4000", "truncated sealed section header");
  }
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::uint64_t read_u64(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 8u > bytes.size()) {
    raise_error("VENOM-E4000", "truncated sealed section header");
  }
  std::uint64_t out = 0;
  for (int i = 0; i < 8; ++i) {
    out |= static_cast<std::uint64_t>(bytes[offset + static_cast<std::size_t>(i)]) << (i * 8);
  }
  return out;
}

std::uint64_t fnv_update(std::uint64_t hash, const unsigned char* data, std::size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= static_cast<std::uint64_t>(data[i]);
    hash *= 1099511628211ull;
  }
  return hash;
}

std::uint64_t fnv_update_u64(std::uint64_t hash, std::uint64_t value) {
  unsigned char bytes[8] = {};
  for (int i = 0; i < 8; ++i) {
    bytes[i] = static_cast<unsigned char>((value >> (i * 8)) & 0xffu);
  }
  return fnv_update(hash, bytes, sizeof(bytes));
}

std::uint64_t name_hash(const std::string& name) {
  return fnv1a64(reinterpret_cast<const unsigned char*>(name.data()), name.size());
}

std::uint64_t mix64(std::uint64_t value) {
  value += 0x9e3779b97f4a7c15ull;
  value = (value ^ (value >> 30u)) * 0xbf58476d1ce4e5b9ull;
  value = (value ^ (value >> 27u)) * 0x94d049bb133111ebull;
  return value ^ (value >> 31u);
}

std::uint64_t derive_legacy_seed(SectionType type, std::uint64_t name_digest, std::uint64_t size, std::uint64_t plain_hash) {
  std::uint64_t h = 1469598103934665603ull;
  const auto type_value = static_cast<std::uint64_t>(static_cast<std::uint32_t>(type));
  h = fnv_update_u64(h, type_value);
  h = fnv_update_u64(h, name_digest);
  h = fnv_update_u64(h, size);
  h = fnv_update_u64(h, plain_hash);
  h ^= kSealSecretA;
  h = mix64(h ^ kSealSecretB);
  return h == 0 ? kSealSecretA : h;
}

std::uint64_t next_stream_word(std::uint64_t& state) {
  state += 0x9e3779b97f4a7c15ull;
  return mix64(state);
}

std::vector<unsigned char> xor_stream(const std::vector<unsigned char>& input, std::uint64_t seed) {
  std::vector<unsigned char> out(input.size());
  std::uint64_t state = seed;
  std::uint64_t word = 0;
  for (std::size_t i = 0; i < input.size(); ++i) {
    if ((i & 7u) == 0u) {
      word = next_stream_word(state);
    }
    out[i] = static_cast<unsigned char>(input[i] ^ static_cast<unsigned char>((word >> ((i & 7u) * 8u)) & 0xffu));
  }
  return out;
}

std::uint64_t derive_aead_nonce_a(SectionType type, std::uint64_t name_digest, std::uint64_t plain_size, const std::vector<unsigned char>& plaintext) {
  std::uint64_t h = 1469598103934665603ull ^ kAeadKeyA;
  h = fnv_update_u64(h, static_cast<std::uint64_t>(static_cast<std::uint32_t>(type)));
  h = fnv_update_u64(h, name_digest);
  h = fnv_update_u64(h, plain_size);
  h = fnv_update(h, plaintext.data(), plaintext.size());
  return mix64(h ^ kAeadKeyB);
}

std::uint64_t derive_aead_nonce_b(SectionType type, std::uint64_t name_digest, std::uint64_t plain_size, const std::vector<unsigned char>& plaintext) {
  std::uint64_t h = 1469598103934665603ull ^ kAeadKeyC;
  h = fnv_update(h, plaintext.data(), plaintext.size());
  h = fnv_update_u64(h, plain_size);
  h = fnv_update_u64(h, name_digest);
  h = fnv_update_u64(h, static_cast<std::uint64_t>(static_cast<std::uint32_t>(type)));
  return mix64(h ^ kAeadKeyD);
}

std::uint64_t derive_aead_stream_seed(SectionType type, std::uint64_t name_digest, std::uint64_t plain_size, std::uint64_t nonce_a, std::uint64_t nonce_b) {
  std::uint64_t h = 1469598103934665603ull ^ kAeadKeyB;
  h = fnv_update_u64(h, static_cast<std::uint64_t>(static_cast<std::uint32_t>(type)));
  h = fnv_update_u64(h, name_digest);
  h = fnv_update_u64(h, plain_size);
  h = fnv_update_u64(h, nonce_a);
  h = fnv_update_u64(h, nonce_b);
  return mix64(h ^ kAeadKeyC);
}

std::pair<std::uint64_t, std::uint64_t> aead_tag(SectionType type,
                                                 std::uint64_t name_digest,
                                                 std::uint64_t plain_size,
                                                 std::uint64_t nonce_a,
                                                 std::uint64_t nonce_b,
                                                 const std::vector<unsigned char>& ciphertext) {
  std::uint64_t a = 1469598103934665603ull ^ kAeadKeyA;
  a = fnv_update_u64(a, static_cast<std::uint64_t>(static_cast<std::uint32_t>(type)));
  a = fnv_update_u64(a, name_digest);
  a = fnv_update_u64(a, plain_size);
  a = fnv_update_u64(a, nonce_a);
  a = fnv_update_u64(a, nonce_b);
  a = fnv_update(a, ciphertext.data(), ciphertext.size());
  a = mix64(a ^ kAeadKeyD);

  std::uint64_t b = 1469598103934665603ull ^ kAeadKeyC;
  b = fnv_update_u64(b, nonce_b);
  b = fnv_update_u64(b, nonce_a);
  b = fnv_update_u64(b, plain_size);
  b = fnv_update_u64(b, name_digest);
  b = fnv_update_u64(b, static_cast<std::uint64_t>(static_cast<std::uint32_t>(type)));
  b = fnv_update(b, ciphertext.data(), ciphertext.size());
  b = mix64(b ^ kAeadKeyB);
  return {a, b};
}

bool magic_is(const std::vector<unsigned char>& bytes, const unsigned char* magic) {
  return bytes.size() >= 8u && std::memcmp(bytes.data(), magic, 8u) == 0;
}

std::string& package_key_override() {
  static std::string value;
  return value;
}

void secure_wipe(void* ptr, std::size_t size) {
  volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
  while (size-- != 0u) *p++ = 0u;
}

struct ScopedKeyWipe {
  std::array<unsigned char, kSodiumKeySize>& key;
  ~ScopedKeyWipe() { secure_wipe(key.data(), key.size()); }
};

int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
  if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
  return -1;
}

std::array<unsigned char, kSodiumKeySize> load_package_key() {
  std::string hex = package_key_override();
  if (hex.empty()) {
    const char* value = std::getenv("VENOM_PACKAGE_KEY");
    if (value == nullptr || value[0] == '\0') {
      value = std::getenv("VENOM_PACKAGE_KEY_HEX");
    }
    if (value == nullptr || value[0] == '\0') {
      raise_error("VENOM-E4000", "libsodium crypto provider requires VENOM_PACKAGE_KEY as 64 hex characters or --key-file");
    }
    hex = normalize_package_key_hex(value);
  }
  if (hex.size() != kSodiumKeySize * 2u) {
    raise_error("VENOM-E4000", "package key must be exactly 64 hex characters");
  }
  std::array<unsigned char, kSodiumKeySize> key{};
  for (std::size_t i = 0; i < kSodiumKeySize; ++i) {
    const int hi = hex_value(hex[i * 2u]);
    const int lo = hex_value(hex[i * 2u + 1u]);
    if (hi < 0 || lo < 0) {
      secure_wipe(hex.data(), hex.size());
      raise_error("VENOM-E4000", "package key contains non-hex characters");
    }
    key[i] = static_cast<unsigned char>((hi << 4) | lo);
  }
  secure_wipe(hex.data(), hex.size());
  return key;
}

struct SodiumApi {
#if defined(_WIN32)
  HMODULE handle = nullptr;
#else
  void* handle = nullptr;
#endif
  int (*sodium_init)() = nullptr;
  void (*randombytes_buf)(void*, std::size_t) = nullptr;
  int (*encrypt)(unsigned char*, unsigned long long*, const unsigned char*, unsigned long long, const unsigned char*, unsigned long long, const unsigned char*, const unsigned char*, const unsigned char*) = nullptr;
  int (*decrypt)(unsigned char*, unsigned long long*, unsigned char*, const unsigned char*, unsigned long long, const unsigned char*, unsigned long long, const unsigned char*, const unsigned char*) = nullptr;
  bool ok = false;
};

#if defined(_WIN32)
void* sodium_symbol(HMODULE handle, const char* name) {
  return reinterpret_cast<void*>(GetProcAddress(handle, name));
}
#else
void* sodium_symbol(void* handle, const char* name) {
  return dlsym(handle, name);
}
#endif

SodiumApi& sodium_api() {
  static SodiumApi api = [] {
    SodiumApi out;
#if defined(_WIN32)
    const char* names[] = {"libsodium.dll", "sodium.dll"};
    for (const char* name : names) {
      out.handle = LoadLibraryA(name);
      if (out.handle != nullptr) break;
    }
#else
    const char* names[] = {"libsodium.so", "libsodium.so.23", "libsodium.so.26"};
    for (const char* name : names) {
      out.handle = dlopen(name, RTLD_NOW | RTLD_LOCAL);
      if (out.handle != nullptr) break;
    }
#endif
    if (out.handle == nullptr) {
      return out;
    }
    out.sodium_init = reinterpret_cast<int (*)()>(sodium_symbol(out.handle, "sodium_init"));
    out.randombytes_buf = reinterpret_cast<void (*)(void*, std::size_t)>(sodium_symbol(out.handle, "randombytes_buf"));
    out.encrypt = reinterpret_cast<int (*)(unsigned char*, unsigned long long*, const unsigned char*, unsigned long long, const unsigned char*, unsigned long long, const unsigned char*, const unsigned char*, const unsigned char*)>(sodium_symbol(out.handle, "crypto_aead_xchacha20poly1305_ietf_encrypt"));
    out.decrypt = reinterpret_cast<int (*)(unsigned char*, unsigned long long*, unsigned char*, const unsigned char*, unsigned long long, const unsigned char*, unsigned long long, const unsigned char*, const unsigned char*)>(sodium_symbol(out.handle, "crypto_aead_xchacha20poly1305_ietf_decrypt"));
    out.ok = out.sodium_init != nullptr && out.randombytes_buf != nullptr && out.encrypt != nullptr && out.decrypt != nullptr && out.sodium_init() >= 0;
    return out;
  }();
  return api;
}

std::vector<unsigned char> sodium_aad(SectionType type, std::uint64_t digest, std::uint64_t plain_size, const std::string& name) {
  std::vector<unsigned char> aad;
  aad.insert(aad.end(), kSodiumMagic, kSodiumMagic + sizeof(kSodiumMagic));
  append_u32(aad, kSealVersion);
  append_u32(aad, static_cast<std::uint32_t>(type));
  append_u64(aad, plain_size);
  append_u64(aad, digest);
  aad.insert(aad.end(), name.begin(), name.end());
  return aad;
}

std::vector<unsigned char> open_libsodium_section_v1(SectionType type,
                                                     const std::string& name,
                                                     const std::vector<unsigned char>& sealed) {
  if (sealed.size() < kSodiumHeaderSize + kSodiumTagSize || !magic_is(sealed, kSodiumMagic)) {
    raise_error("VENOM-E4000", "invalid libsodium sealed section payload: " + name);
  }
  auto& api = sodium_api();
  if (!api.ok) {
    raise_error("VENOM-E4000", "libsodium provider is not available for sealed section: " + name);
  }
  auto key = load_package_key();
  ScopedKeyWipe key_wipe{key};
  const auto version = read_u32(sealed, 8u);
  const auto sealed_type = read_u32(sealed, 12u);
  const auto plain_size = read_u64(sealed, 16u);
  const auto sealed_name_hash = read_u64(sealed, 24u);
  if (version != kSealVersion) {
    raise_error("VENOM-E4000", "unsupported libsodium sealed section version: " + name);
  }
  if (sealed_type != static_cast<std::uint32_t>(type)) {
    raise_error("VENOM-E4000", "libsodium sealed section type mismatch: " + name);
  }
  if (sealed_name_hash != name_hash(name)) {
    raise_error("VENOM-E4000", "libsodium sealed section name mismatch: " + name);
  }
  const auto cipher_size = sealed.size() - kSodiumHeaderSize;
  if (cipher_size != plain_size + kSodiumTagSize) {
    raise_error("VENOM-E4000", "libsodium sealed section size mismatch: " + name);
  }
  const auto aad = sodium_aad(type, sealed_name_hash, plain_size, name);
  std::vector<unsigned char> plaintext(static_cast<std::size_t>(plain_size));
  unsigned long long decrypted_size = 0;
  const int rc = api.decrypt(plaintext.data(),
                             &decrypted_size,
                             nullptr,
                             sealed.data() + kSodiumHeaderSize,
                             static_cast<unsigned long long>(cipher_size),
                             aad.data(),
                             static_cast<unsigned long long>(aad.size()),
                             sealed.data() + 32u,
                             key.data());
  if (rc != 0 || decrypted_size != plain_size) {
    raise_error("VENOM-E4000", "libsodium sealed section authentication mismatch: " + name);
  }
  return plaintext;
}

std::vector<unsigned char> open_legacy_sealed_section_v1(SectionType type,
                                                         const std::string& name,
                                                         const std::vector<unsigned char>& sealed) {
  if (sealed.size() < kLegacySealHeaderSize || !magic_is(sealed, kLegacySealMagic)) {
    raise_error("VENOM-E4000", "invalid legacy sealed section payload: " + name);
  }
  const auto version = read_u32(sealed, 8u);
  const auto sealed_type = read_u32(sealed, 12u);
  const auto plain_size = read_u64(sealed, 16u);
  const auto plain_hash = read_u64(sealed, 24u);
  const auto sealed_name_hash = read_u64(sealed, 32u);
  if (version != kSealVersion) {
    raise_error("VENOM-E4000", "unsupported sealed section version: " + name);
  }
  if (sealed_type != static_cast<std::uint32_t>(type)) {
    raise_error("VENOM-E4000", "sealed section type mismatch: " + name);
  }
  if (sealed_name_hash != name_hash(name)) {
    raise_error("VENOM-E4000", "sealed section name mismatch: " + name);
  }
  if (plain_size != static_cast<std::uint64_t>(sealed.size() - kLegacySealHeaderSize)) {
    raise_error("VENOM-E4000", "sealed section size mismatch: " + name);
  }

  std::vector<unsigned char> ciphertext(sealed.begin() + static_cast<std::ptrdiff_t>(kLegacySealHeaderSize), sealed.end());
  const auto seed = derive_legacy_seed(type, sealed_name_hash, plain_size, plain_hash);
  auto plaintext = xor_stream(ciphertext, seed);
  if (fnv1a64(plaintext) != plain_hash) {
    raise_error("VENOM-E4000", "sealed section authentication mismatch: " + name);
  }
  return plaintext;
}

} // namespace

std::string normalize_package_key_hex(const std::string& input) {
  std::string hex;
  hex.reserve(input.size());
  for (char ch : input) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isxdigit(uch) != 0) {
      hex.push_back(static_cast<char>(std::tolower(uch)));
    } else if (std::isspace(uch) != 0 || ch == ':' || ch == '-') {
      continue;
    } else if (ch == '#') {
      break;
    } else {
      raise_error("VENOM-E4000", "package key file contains non-hex characters");
    }
  }
  if (hex.size() != kSodiumKeySize * 2u) {
    raise_error("VENOM-E4000", "package key must be exactly 64 hex characters");
  }
  for (char ch : hex) {
    if (hex_value(ch) < 0) {
      raise_error("VENOM-E4000", "package key contains non-hex characters");
    }
  }
  return hex;
}

void set_package_key_hex_override(const std::string& hex_key) {
  package_key_override() = normalize_package_key_hex(hex_key);
}

void clear_package_key_hex_override() {
  package_key_override().clear();
}

std::string Sha256IntegrityProvider::name() const {
  return "sha256-software-v1";
}

std::string Sha256IntegrityProvider::digest_hex(const std::vector<unsigned char>& data) const {
  return sha256_hex(data);
}

std::string NoAeadProvider::name() const {
  return "none";
}

bool NoAeadProvider::available() const {
  return false;
}

std::vector<unsigned char> NoAeadProvider::seal(const std::vector<unsigned char>& plaintext) {
  return plaintext;
}

std::vector<unsigned char> NoAeadProvider::open(const std::vector<unsigned char>& ciphertext) {
  return ciphertext;
}

bool libsodium_aead_available() {
  return sodium_api().ok;
}

std::string libsodium_aead_provider_name() {
  return "libsodium-xchacha20poly1305-ietf-v1";
}

std::vector<unsigned char> seal_section_libsodium_v1(SectionType type,
                                                     const std::string& name,
                                                     const std::vector<unsigned char>& plaintext) {
  auto& api = sodium_api();
  if (!api.ok) {
    raise_error("VENOM-E4000", "libsodium provider is not available; install libsodium runtime or choose --crypto-provider runtime");
  }
  auto key = load_package_key();
  ScopedKeyWipe key_wipe{key};
  const auto digest = name_hash(name);
  const auto plain_size = static_cast<std::uint64_t>(plaintext.size());
  const auto aad = sodium_aad(type, digest, plain_size, name);
  std::array<unsigned char, kSodiumNonceSize> nonce{};
  api.randombytes_buf(nonce.data(), nonce.size());

  std::vector<unsigned char> ciphertext(plaintext.size() + kSodiumTagSize);
  unsigned long long cipher_size = 0;
  const int rc = api.encrypt(ciphertext.data(),
                             &cipher_size,
                             plaintext.data(),
                             static_cast<unsigned long long>(plaintext.size()),
                             aad.data(),
                             static_cast<unsigned long long>(aad.size()),
                             nullptr,
                             nonce.data(),
                             key.data());
  if (rc != 0 || cipher_size != plaintext.size() + kSodiumTagSize) {
    raise_error("VENOM-E4000", "libsodium failed to seal package section: " + name);
  }
  ciphertext.resize(static_cast<std::size_t>(cipher_size));

  std::vector<unsigned char> out;
  out.reserve(kSodiumHeaderSize + ciphertext.size());
  out.insert(out.end(), kSodiumMagic, kSodiumMagic + sizeof(kSodiumMagic));
  append_u32(out, kSealVersion);
  append_u32(out, static_cast<std::uint32_t>(type));
  append_u64(out, plain_size);
  append_u64(out, digest);
  out.insert(out.end(), nonce.begin(), nonce.end());
  out.insert(out.end(), ciphertext.begin(), ciphertext.end());
  return out;
}

std::vector<unsigned char> seal_section_v1(SectionType type,
                                           const std::string& name,
                                           const std::vector<unsigned char>& plaintext) {
  const auto digest = name_hash(name);
  const auto plain_size = static_cast<std::uint64_t>(plaintext.size());
  const auto nonce_a = derive_aead_nonce_a(type, digest, plain_size, plaintext);
  const auto nonce_b = derive_aead_nonce_b(type, digest, plain_size, plaintext);
  const auto seed = derive_aead_stream_seed(type, digest, plain_size, nonce_a, nonce_b);
  auto ciphertext = xor_stream(plaintext, seed);
  const auto tag = aead_tag(type, digest, plain_size, nonce_a, nonce_b, ciphertext);

  std::vector<unsigned char> out;
  out.reserve(kAeadHeaderSize + ciphertext.size());
  out.insert(out.end(), kAeadMagic, kAeadMagic + sizeof(kAeadMagic));
  append_u32(out, kSealVersion);
  append_u32(out, static_cast<std::uint32_t>(type));
  append_u64(out, plain_size);
  append_u64(out, digest);
  append_u64(out, nonce_a);
  append_u64(out, nonce_b);
  append_u64(out, tag.first);
  append_u64(out, tag.second);
  out.insert(out.end(), ciphertext.begin(), ciphertext.end());
  return out;
}

std::vector<unsigned char> open_section_v1(SectionType type,
                                           const std::string& name,
                                           const std::vector<unsigned char>& sealed) {
  if (magic_is(sealed, kSodiumMagic)) {
    return open_libsodium_section_v1(type, name, sealed);
  }
  if (magic_is(sealed, kLegacySealMagic)) {
    return open_legacy_sealed_section_v1(type, name, sealed);
  }
  if (sealed.size() < kAeadHeaderSize || !magic_is(sealed, kAeadMagic)) {
    raise_error("VENOM-E4000", "invalid AEAD sealed section payload: " + name);
  }
  const auto version = read_u32(sealed, 8u);
  const auto sealed_type = read_u32(sealed, 12u);
  const auto plain_size = read_u64(sealed, 16u);
  const auto sealed_name_hash = read_u64(sealed, 24u);
  const auto nonce_a = read_u64(sealed, 32u);
  const auto nonce_b = read_u64(sealed, 40u);
  const auto tag_a = read_u64(sealed, 48u);
  const auto tag_b = read_u64(sealed, 56u);
  if (version != kSealVersion) {
    raise_error("VENOM-E4000", "unsupported AEAD sealed section version: " + name);
  }
  if (sealed_type != static_cast<std::uint32_t>(type)) {
    raise_error("VENOM-E4000", "AEAD sealed section type mismatch: " + name);
  }
  if (sealed_name_hash != name_hash(name)) {
    raise_error("VENOM-E4000", "AEAD sealed section name mismatch: " + name);
  }
  if (plain_size != static_cast<std::uint64_t>(sealed.size() - kAeadHeaderSize)) {
    raise_error("VENOM-E4000", "AEAD sealed section size mismatch: " + name);
  }

  std::vector<unsigned char> ciphertext(sealed.begin() + static_cast<std::ptrdiff_t>(kAeadHeaderSize), sealed.end());
  const auto expected_tag = aead_tag(type, sealed_name_hash, plain_size, nonce_a, nonce_b, ciphertext);
  if (expected_tag.first != tag_a || expected_tag.second != tag_b) {
    raise_error("VENOM-E4000", "AEAD sealed section authentication mismatch: " + name);
  }
  const auto seed = derive_aead_stream_seed(type, sealed_name_hash, plain_size, nonce_a, nonce_b);
  return xor_stream(ciphertext, seed);
}

bool is_sealed_section_v1(const std::vector<unsigned char>& bytes) {
  return magic_is(bytes, kSodiumMagic) || magic_is(bytes, kAeadMagic) || magic_is(bytes, kLegacySealMagic);
}

} // namespace venom::package
