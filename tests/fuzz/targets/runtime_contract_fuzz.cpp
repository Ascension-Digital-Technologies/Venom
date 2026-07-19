#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {
std::uint32_t read_u32(const std::uint8_t* data, std::size_t size, std::size_t off) {
  if (off > size || size - off < 4) throw std::runtime_error("truncated u32");
  return static_cast<std::uint32_t>(data[off]) |
         (static_cast<std::uint32_t>(data[off + 1]) << 8u) |
         (static_cast<std::uint32_t>(data[off + 2]) << 16u) |
         (static_cast<std::uint32_t>(data[off + 3]) << 24u);
}

bool magic_is(const std::uint8_t* data, std::size_t size, const char (&magic)[9]) {
  if (size < 8) return false;
  for (std::size_t i = 0; i < 8; ++i) {
    if (data[i] != static_cast<std::uint8_t>(magic[i])) return false;
  }
  return true;
}

void parse_policy(const std::uint8_t* data, std::size_t size) {
  if (size != 24 || !magic_is(data, size, "VRPOL002")) throw std::runtime_error("invalid policy");
  if (read_u32(data, size, 8) != 2u) throw std::runtime_error("policy version");
  const auto flags = read_u32(data, size, 12);
  if ((flags & ~0x0000ffffu) != 0u) throw std::runtime_error("policy flags");
}

void parse_fingerprint(const std::uint8_t* data, std::size_t size) {
  if (size != 24 || !magic_is(data, size, "VQAF0001")) throw std::runtime_error("invalid fingerprint");
  if (read_u32(data, size, 8) != 1u) throw std::runtime_error("fingerprint version");
  if (read_u32(data, size, 12) != 12u) throw std::runtime_error("runtime abi");
}

void parse_diversification(const std::uint8_t* data, std::size_t size) {
  constexpr std::size_t kHostCalls = 89;
  constexpr std::size_t kResultFields = 8;
  constexpr std::size_t kExpected = 8 + 4 * 4 + (kHostCalls + kResultFields) * 2;
  if (size != kExpected || !magic_is(data, size, "VRDIV003")) throw std::runtime_error("invalid diversification");
  if (read_u32(data, size, 8) != 3u) throw std::runtime_error("diversification version");
  if (read_u32(data, size, 12) != kHostCalls || read_u32(data, size, 16) != kResultFields) {
    throw std::runtime_error("diversification counts");
  }
  std::array<bool, kHostCalls> host_seen{};
  std::array<bool, kResultFields> field_seen{};
  std::size_t off = 24;
  for (std::size_t i = 0; i < kHostCalls; ++i, off += 2) {
    const auto v = static_cast<std::size_t>(data[off] | (static_cast<unsigned>(data[off + 1]) << 8u));
    if (v >= kHostCalls || host_seen[v]) throw std::runtime_error("host permutation");
    host_seen[v] = true;
  }
  for (std::size_t i = 0; i < kResultFields; ++i, off += 2) {
    const auto v = static_cast<std::size_t>(data[off] | (static_cast<unsigned>(data[off + 1]) << 8u));
    if (v >= kResultFields || field_seen[v]) throw std::runtime_error("field permutation");
    field_seen[v] = true;
  }
}

void parse_contract(const std::uint8_t* data, std::size_t size) {
  if (magic_is(data, size, "VRPOL002")) return parse_policy(data, size);
  if (magic_is(data, size, "VQAF0001")) return parse_fingerprint(data, size);
  if (magic_is(data, size, "VRDIV003")) return parse_diversification(data, size);
  throw std::runtime_error("unknown contract");
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  try { parse_contract(data, size); } catch (...) {}
  return 0;
}
