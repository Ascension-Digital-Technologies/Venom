#include "venom/base/error.hpp"
#include "venom/internal/package/compress.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace venom::package {

namespace {
constexpr unsigned char kMagic[8] = {'V','C','L','Z','0','0','0','8'};
constexpr std::size_t kHeaderSize = 20u;
constexpr std::size_t kMaxWindow = 4096u;
constexpr std::size_t kMinMatch = 4u;
constexpr std::size_t kMaxMatch = 130u;
constexpr std::size_t kMaxLiteral = 128u;

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

std::uint32_t read_u32_at(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 4u > bytes.size()) {
    raise_error("VENOM-E4000", "truncated compressed stream while reading u32");
  }
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

std::uint64_t read_u64_at(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 8u > bytes.size()) {
    raise_error("VENOM-E4000", "truncated compressed stream while reading u64");
  }
  std::uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value |= static_cast<std::uint64_t>(bytes[offset + static_cast<std::size_t>(i)]) << (i * 8);
  }
  return value;
}

struct Match {
  std::size_t length = 0;
  std::size_t distance = 0;
};

Match find_match(const std::vector<unsigned char>& input, std::size_t pos) {
  Match best;
  const std::size_t window_start = pos > kMaxWindow ? pos - kMaxWindow : 0u;
  for (std::size_t candidate = window_start; candidate < pos; ++candidate) {
    std::size_t length = 0;
    while (length < kMaxMatch && pos + length < input.size() && input[candidate + length] == input[pos + length]) {
      ++length;
      if (candidate + length >= pos) {
        // Overlapping backrefs are legal during decode, but keeping the encoder
        // non-overlapping keeps the stream simple and deterministic.
        break;
      }
    }
    if (length > best.length && length >= kMinMatch) {
      best.length = length;
      best.distance = pos - candidate;
      if (best.length == kMaxMatch) {
        break;
      }
    }
  }
  return best;
}

void flush_literal(std::vector<unsigned char>& out, const std::vector<unsigned char>& input, std::size_t start, std::size_t end) {
  std::size_t cursor = start;
  while (cursor < end) {
    const std::size_t chunk = std::min(kMaxLiteral, end - cursor);
    out.push_back(static_cast<unsigned char>(chunk - 1u));
    out.insert(out.end(), input.begin() + static_cast<std::ptrdiff_t>(cursor), input.begin() + static_cast<std::ptrdiff_t>(cursor + chunk));
    cursor += chunk;
  }
}
} // namespace

bool compression_is_worthwhile(const std::vector<unsigned char>& input) {
  if (input.size() < 64u) {
    return false;
  }
  return compress_rle_v1(input).size() + 8u < input.size();
}

std::vector<unsigned char> compress_rle_v1(const std::vector<unsigned char>& input) {
  std::vector<unsigned char> out;
  out.reserve(input.size());
  out.insert(out.end(), kMagic, kMagic + sizeof(kMagic));
  append_u32(out, 1u);
  append_u64(out, static_cast<std::uint64_t>(input.size()));

  std::size_t literal_start = 0;
  std::size_t i = 0;
  while (i < input.size()) {
    const Match match = find_match(input, i);
    if (match.length >= kMinMatch && match.distance <= 65535u) {
      flush_literal(out, input, literal_start, i);
      out.push_back(static_cast<unsigned char>(0x80u | static_cast<unsigned char>(match.length - kMinMatch)));
      out.push_back(static_cast<unsigned char>(match.distance & 0xffu));
      out.push_back(static_cast<unsigned char>((match.distance >> 8u) & 0xffu));
      i += match.length;
      literal_start = i;
      continue;
    }
    ++i;
  }
  flush_literal(out, input, literal_start, input.size());
  return out;
}

std::vector<unsigned char> decompress_rle_v1(const std::vector<unsigned char>& input, std::size_t expected_size) {
  if (input.size() < kHeaderSize) {
    raise_error("VENOM-E4000", "compressed section is too small");
  }
  for (std::size_t i = 0; i < sizeof(kMagic); ++i) {
    if (input[i] != kMagic[i]) {
      raise_error("VENOM-E4000", "invalid compressed section magic");
    }
  }
  const auto version = read_u32_at(input, 8u);
  if (version != 1u) {
    raise_error("VENOM-E4000", "unsupported compressed section version");
  }
  const auto declared_size = read_u64_at(input, 12u);
  if (declared_size != static_cast<std::uint64_t>(expected_size)) {
    raise_error("VENOM-E4000", "compressed section raw size mismatch");
  }

  std::vector<unsigned char> out;
  out.reserve(expected_size);
  std::size_t i = kHeaderSize;
  while (i < input.size()) {
    const unsigned char token = input[i++];
    if ((token & 0x80u) != 0u) {
      const std::size_t length = static_cast<std::size_t>(token & 0x7fu) + kMinMatch;
      if (i + 2u > input.size()) {
        raise_error("VENOM-E4000", "truncated compressed backref");
      }
      const std::size_t distance = static_cast<std::size_t>(input[i]) | (static_cast<std::size_t>(input[i + 1u]) << 8u);
      i += 2u;
      if (distance == 0u || distance > out.size()) {
        raise_error("VENOM-E4000", "invalid compressed backref distance");
      }
      if (out.size() + length > expected_size) {
        raise_error("VENOM-E4000", "compressed section expanded past expected size");
      }
      const std::size_t src = out.size() - distance;
      for (std::size_t j = 0; j < length; ++j) {
        out.push_back(out[src + j]);
      }
    } else {
      const std::size_t literal_len = static_cast<std::size_t>(token) + 1u;
      if (i + literal_len > input.size()) {
        raise_error("VENOM-E4000", "truncated compressed literal");
      }
      if (out.size() + literal_len > expected_size) {
        raise_error("VENOM-E4000", "compressed section expanded past expected size");
      }
      out.insert(out.end(), input.begin() + static_cast<std::ptrdiff_t>(i), input.begin() + static_cast<std::ptrdiff_t>(i + literal_len));
      i += literal_len;
    }
  }
  if (out.size() != expected_size) {
    raise_error("VENOM-E4000", "compressed section expanded to the wrong size");
  }
  return out;
}

std::vector<unsigned char> compress_none(const std::vector<unsigned char>& input) {
  return input;
}

std::vector<unsigned char> decompress_none(const std::vector<unsigned char>& input) {
  return input;
}

} // namespace venom::package
