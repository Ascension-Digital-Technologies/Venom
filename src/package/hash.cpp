#include "venom/package/hash.hpp"

#include <array>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace venom::package {

std::uint64_t fnv1a64(const unsigned char* data, std::size_t size) {
  std::uint64_t hash = 1469598103934665603ull;
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= static_cast<std::uint64_t>(data[i]);
    hash *= 1099511628211ull;
  }
  return hash;
}

std::uint64_t fnv1a64(const std::vector<unsigned char>& data) {
  return fnv1a64(data.data(), data.size());
}

namespace {
constexpr std::array<std::uint32_t, 64> kSha256K = {
  0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
  0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
  0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
  0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
  0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
  0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
  0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
  0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

std::uint32_t rotr(std::uint32_t value, std::uint32_t bits) {
  return (value >> bits) | (value << (32u - bits));
}

std::uint32_t load_be32(const unsigned char* data) {
  return (static_cast<std::uint32_t>(data[0]) << 24u) |
         (static_cast<std::uint32_t>(data[1]) << 16u) |
         (static_cast<std::uint32_t>(data[2]) << 8u) |
         static_cast<std::uint32_t>(data[3]);
}

void store_be32(unsigned char* out, std::uint32_t value) {
  out[0] = static_cast<unsigned char>((value >> 24u) & 0xffu);
  out[1] = static_cast<unsigned char>((value >> 16u) & 0xffu);
  out[2] = static_cast<unsigned char>((value >> 8u) & 0xffu);
  out[3] = static_cast<unsigned char>(value & 0xffu);
}
} // namespace

Sha256Digest sha256(const unsigned char* data, std::size_t size) {
  std::uint32_t h[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u,
  };

  const std::uint64_t bit_len = static_cast<std::uint64_t>(size) * 8ull;
  const std::size_t padded_size = ((size + 9u + 63u) / 64u) * 64u;
  std::vector<unsigned char> msg(padded_size, 0u);
  if (size != 0u) {
    std::memcpy(msg.data(), data, size);
  }
  msg[size] = 0x80u;
  for (int i = 0; i < 8; ++i) {
    msg[padded_size - 1u - static_cast<std::size_t>(i)] = static_cast<unsigned char>((bit_len >> (i * 8)) & 0xffu);
  }

  for (std::size_t chunk = 0; chunk < padded_size; chunk += 64u) {
    std::uint32_t w[64] = {};
    for (int i = 0; i < 16; ++i) {
      w[i] = load_be32(msg.data() + chunk + static_cast<std::size_t>(i) * 4u);
    }
    for (int i = 16; i < 64; ++i) {
      const auto s0 = rotr(w[i - 15], 7u) ^ rotr(w[i - 15], 18u) ^ (w[i - 15] >> 3u);
      const auto s1 = rotr(w[i - 2], 17u) ^ rotr(w[i - 2], 19u) ^ (w[i - 2] >> 10u);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    std::uint32_t a = h[0];
    std::uint32_t b = h[1];
    std::uint32_t c = h[2];
    std::uint32_t d = h[3];
    std::uint32_t e = h[4];
    std::uint32_t f = h[5];
    std::uint32_t g = h[6];
    std::uint32_t hh = h[7];

    for (int i = 0; i < 64; ++i) {
      const auto s1 = rotr(e, 6u) ^ rotr(e, 11u) ^ rotr(e, 25u);
      const auto ch = (e & f) ^ ((~e) & g);
      const auto temp1 = hh + s1 + ch + kSha256K[static_cast<std::size_t>(i)] + w[i];
      const auto s0 = rotr(a, 2u) ^ rotr(a, 13u) ^ rotr(a, 22u);
      const auto maj = (a & b) ^ (a & c) ^ (b & c);
      const auto temp2 = s0 + maj;
      hh = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
  }

  Sha256Digest digest{};
  for (int i = 0; i < 8; ++i) {
    store_be32(digest.data() + static_cast<std::size_t>(i) * 4u, h[i]);
  }
  return digest;
}

Sha256Digest sha256(const std::vector<unsigned char>& data) {
  return sha256(data.data(), data.size());
}


std::string bytes_to_base64(const unsigned char* data, std::size_t size) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((size + 2u) / 3u) * 4u);
  for (std::size_t i = 0; i < size; i += 3u) {
    const auto remaining = size - i;
    const std::uint32_t block =
        (static_cast<std::uint32_t>(data[i]) << 16u) |
        (remaining > 1u ? static_cast<std::uint32_t>(data[i + 1u]) << 8u : 0u) |
        (remaining > 2u ? static_cast<std::uint32_t>(data[i + 2u]) : 0u);
    out.push_back(kAlphabet[(block >> 18u) & 0x3fu]);
    out.push_back(kAlphabet[(block >> 12u) & 0x3fu]);
    out.push_back(remaining > 1u ? kAlphabet[(block >> 6u) & 0x3fu] : '=');
    out.push_back(remaining > 2u ? kAlphabet[block & 0x3fu] : '=');
  }
  return out;
}

std::string sha256_base64(const unsigned char* data, std::size_t size) {
  const auto digest = sha256(data, size);
  return bytes_to_base64(digest.data(), digest.size());
}

std::string sha256_base64(const std::vector<unsigned char>& data) {
  return sha256_base64(data.data(), data.size());
}

std::string sha256_sri(const unsigned char* data, std::size_t size) {
  return "sha256-" + sha256_base64(data, size);
}

std::string sha256_sri(const std::vector<unsigned char>& data) {
  return sha256_sri(data.data(), data.size());
}

std::string bytes_to_hex(const unsigned char* data, std::size_t size) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (std::size_t i = 0; i < size; ++i) {
    out << std::setw(2) << static_cast<unsigned int>(data[i]);
  }
  return out.str();
}

std::string sha256_hex(const unsigned char* data, std::size_t size) {
  const auto digest = sha256(data, size);
  return bytes_to_hex(digest.data(), digest.size());
}

std::string sha256_hex(const std::vector<unsigned char>& data) {
  return sha256_hex(data.data(), data.size());
}

namespace {
constexpr std::array<std::uint64_t, 80> kSha512K = {
  0x428a2f98d728ae22ull, 0x7137449123ef65cdull, 0xb5c0fbcfec4d3b2full, 0xe9b5dba58189dbbcull,
  0x3956c25bf348b538ull, 0x59f111f1b605d019ull, 0x923f82a4af194f9bull, 0xab1c5ed5da6d8118ull,
  0xd807aa98a3030242ull, 0x12835b0145706fbeull, 0x243185be4ee4b28cull, 0x550c7dc3d5ffb4e2ull,
  0x72be5d74f27b896full, 0x80deb1fe3b1696b1ull, 0x9bdc06a725c71235ull, 0xc19bf174cf692694ull,
  0xe49b69c19ef14ad2ull, 0xefbe4786384f25e3ull, 0x0fc19dc68b8cd5b5ull, 0x240ca1cc77ac9c65ull,
  0x2de92c6f592b0275ull, 0x4a7484aa6ea6e483ull, 0x5cb0a9dcbd41fbd4ull, 0x76f988da831153b5ull,
  0x983e5152ee66dfabull, 0xa831c66d2db43210ull, 0xb00327c898fb213full, 0xbf597fc7beef0ee4ull,
  0xc6e00bf33da88fc2ull, 0xd5a79147930aa725ull, 0x06ca6351e003826full, 0x142929670a0e6e70ull,
  0x27b70a8546d22ffcull, 0x2e1b21385c26c926ull, 0x4d2c6dfc5ac42aedull, 0x53380d139d95b3dfull,
  0x650a73548baf63deull, 0x766a0abb3c77b2a8ull, 0x81c2c92e47edaee6ull, 0x92722c851482353bull,
  0xa2bfe8a14cf10364ull, 0xa81a664bbc423001ull, 0xc24b8b70d0f89791ull, 0xc76c51a30654be30ull,
  0xd192e819d6ef5218ull, 0xd69906245565a910ull, 0xf40e35855771202aull, 0x106aa07032bbd1b8ull,
  0x19a4c116b8d2d0c8ull, 0x1e376c085141ab53ull, 0x2748774cdf8eeb99ull, 0x34b0bcb5e19b48a8ull,
  0x391c0cb3c5c95a63ull, 0x4ed8aa4ae3418acbull, 0x5b9cca4f7763e373ull, 0x682e6ff3d6b2b8a3ull,
  0x748f82ee5defb2fcull, 0x78a5636f43172f60ull, 0x84c87814a1f0ab72ull, 0x8cc702081a6439ecull,
  0x90befffa23631e28ull, 0xa4506cebde82bde9ull, 0xbef9a3f7b2c67915ull, 0xc67178f2e372532bull,
  0xca273eceea26619cull, 0xd186b8c721c0c207ull, 0xeada7dd6cde0eb1eull, 0xf57d4f7fee6ed178ull,
  0x06f067aa72176fbaull, 0x0a637dc5a2c898a6ull, 0x113f9804bef90daeull, 0x1b710b35131c471bull,
  0x28db77f523047d84ull, 0x32caab7b40c72493ull, 0x3c9ebe0a15c9bebcull, 0x431d67c49c100d4cull,
  0x4cc5d4becb3e42b6ull, 0x597f299cfc657e2aull, 0x5fcb6fab3ad6faecull, 0x6c44198c4a475817ull,
};

std::uint64_t rotr64(std::uint64_t value, unsigned int bits) {
  return (value >> bits) | (value << (64u - bits));
}

std::uint64_t load_be64(const unsigned char* data) {
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value = (value << 8u) | static_cast<std::uint64_t>(data[i]);
  }
  return value;
}

void store_be64(unsigned char* out, std::uint64_t value) {
  for (int i = 7; i >= 0; --i) {
    out[i] = static_cast<unsigned char>(value & 0xffu);
    value >>= 8u;
  }
}

std::array<std::uint64_t, 8> sha512_state(const unsigned char* data,
                                          std::size_t size,
                                          std::array<std::uint64_t, 8> h) {
  // Venom limits vendored scripts to a few MiB, so the high 64 bits of the
  // SHA-512 128-bit message length are always zero.
  const std::uint64_t bit_len_low = static_cast<std::uint64_t>(size) * 8ull;
  const std::size_t padded_size = ((size + 17u + 127u) / 128u) * 128u;
  std::vector<unsigned char> msg(padded_size, 0u);
  if (size != 0u) {
    std::memcpy(msg.data(), data, size);
  }
  msg[size] = 0x80u;
  store_be64(msg.data() + padded_size - 16u, 0u);
  store_be64(msg.data() + padded_size - 8u, bit_len_low);

  for (std::size_t chunk = 0; chunk < padded_size; chunk += 128u) {
    std::uint64_t w[80] = {};
    for (int i = 0; i < 16; ++i) {
      w[i] = load_be64(msg.data() + chunk + static_cast<std::size_t>(i) * 8u);
    }
    for (int i = 16; i < 80; ++i) {
      const auto s0 = rotr64(w[i - 15], 1u) ^ rotr64(w[i - 15], 8u) ^ (w[i - 15] >> 7u);
      const auto s1 = rotr64(w[i - 2], 19u) ^ rotr64(w[i - 2], 61u) ^ (w[i - 2] >> 6u);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    std::uint64_t a = h[0];
    std::uint64_t b = h[1];
    std::uint64_t c = h[2];
    std::uint64_t d = h[3];
    std::uint64_t e = h[4];
    std::uint64_t f = h[5];
    std::uint64_t g = h[6];
    std::uint64_t hh = h[7];

    for (int i = 0; i < 80; ++i) {
      const auto s1 = rotr64(e, 14u) ^ rotr64(e, 18u) ^ rotr64(e, 41u);
      const auto ch = (e & f) ^ ((~e) & g);
      const auto temp1 = hh + s1 + ch + kSha512K[static_cast<std::size_t>(i)] + w[i];
      const auto s0 = rotr64(a, 28u) ^ rotr64(a, 34u) ^ rotr64(a, 39u);
      const auto maj = (a & b) ^ (a & c) ^ (b & c);
      const auto temp2 = s0 + maj;
      hh = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
  }
  return h;
}
} // namespace

Sha512Digest sha512(const unsigned char* data, std::size_t size) {
  const auto state = sha512_state(data, size, {
    0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull, 0xa54ff53a5f1d36f1ull,
    0x510e527fade682d1ull, 0x9b05688c2b3e6c1full, 0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull,
  });
  Sha512Digest digest{};
  for (int i = 0; i < 8; ++i) {
    store_be64(digest.data() + static_cast<std::size_t>(i) * 8u, state[static_cast<std::size_t>(i)]);
  }
  return digest;
}

Sha512Digest sha512(const std::vector<unsigned char>& data) {
  return sha512(data.data(), data.size());
}

Sha384Digest sha384(const unsigned char* data, std::size_t size) {
  const auto state = sha512_state(data, size, {
    0xcbbb9d5dc1059ed8ull, 0x629a292a367cd507ull, 0x9159015a3070dd17ull, 0x152fecd8f70e5939ull,
    0x67332667ffc00b31ull, 0x8eb44a8768581511ull, 0xdb0c2e0d64f98fa7ull, 0x47b5481dbefa4fa4ull,
  });
  Sha384Digest digest{};
  for (int i = 0; i < 6; ++i) {
    store_be64(digest.data() + static_cast<std::size_t>(i) * 8u, state[static_cast<std::size_t>(i)]);
  }
  return digest;
}

Sha384Digest sha384(const std::vector<unsigned char>& data) {
  return sha384(data.data(), data.size());
}

std::string sha512_base64(const unsigned char* data, std::size_t size) {
  const auto digest = sha512(data, size);
  return bytes_to_base64(digest.data(), digest.size());
}

std::string sha512_base64(const std::vector<unsigned char>& data) {
  return sha512_base64(data.data(), data.size());
}

std::string sha512_sri(const unsigned char* data, std::size_t size) {
  return "sha512-" + sha512_base64(data, size);
}

std::string sha512_sri(const std::vector<unsigned char>& data) {
  return sha512_sri(data.data(), data.size());
}

std::string sha384_base64(const unsigned char* data, std::size_t size) {
  const auto digest = sha384(data, size);
  return bytes_to_base64(digest.data(), digest.size());
}

std::string sha384_base64(const std::vector<unsigned char>& data) {
  return sha384_base64(data.data(), data.size());
}

std::string sha384_sri(const unsigned char* data, std::size_t size) {
  return "sha384-" + sha384_base64(data, size);
}

std::string sha384_sri(const std::vector<unsigned char>& data) {
  return sha384_sri(data.data(), data.size());
}

} // namespace venom::package
