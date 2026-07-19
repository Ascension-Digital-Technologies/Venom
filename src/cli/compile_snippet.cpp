#include "venom/base/error.hpp"
#include "venom/internal/cli/compile_snippet.hpp"
#include "venom/quickjs/bytecode.hpp"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
std::string base64_encode(const std::vector<unsigned char>& bytes) {
  static constexpr char alphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((bytes.size() + 2u) / 3u) * 4u);
  std::size_t i = 0;
  while (i + 3u <= bytes.size()) {
    const std::uint32_t value = (static_cast<std::uint32_t>(bytes[i]) << 16u) |
                                (static_cast<std::uint32_t>(bytes[i + 1u]) << 8u) |
                                static_cast<std::uint32_t>(bytes[i + 2u]);
    out.push_back(alphabet[(value >> 18u) & 63u]);
    out.push_back(alphabet[(value >> 12u) & 63u]);
    out.push_back(alphabet[(value >> 6u) & 63u]);
    out.push_back(alphabet[value & 63u]);
    i += 3u;
  }
  if (i < bytes.size()) {
    std::uint32_t value = static_cast<std::uint32_t>(bytes[i]) << 16u;
    out.push_back(alphabet[(value >> 18u) & 63u]);
    if (i + 1u < bytes.size()) {
      value |= static_cast<std::uint32_t>(bytes[i + 1u]) << 8u;
      out.push_back(alphabet[(value >> 12u) & 63u]);
      out.push_back(alphabet[(value >> 6u) & 63u]);
      out.push_back('=');
    } else {
      out.push_back(alphabet[(value >> 12u) & 63u]);
      out.push_back('=');
      out.push_back('=');
    }
  }
  return out;
}

std::uint32_t fnv1a32(const std::vector<unsigned char>& bytes) {
  std::uint32_t hash = 2166136261u;
  for (const auto byte : bytes) {
    hash ^= static_cast<std::uint32_t>(byte);
    hash *= 16777619u;
  }
  return hash;
}
} // namespace

bool compile_snippet(std::istream& input, std::ostream& output) {
  const std::string source((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
  if (source.empty()) raise_error("VENOM-E1000", "compile-snippet received empty source");
  if (source.size() > 1024u * 1024u) raise_error("VENOM-E1000", "compile-snippet source exceeds 1 MiB");

  const auto bytecode = venom::quickjs::compile_native_quickjs_bytecode(
      source, "playground/user-script.js", false, true, nullptr);
  if (bytecode.empty()) raise_error("VENOM-E1000", "QuickJS produced an empty bytecode record");

  output << "{\"ok\":true,\"format\":\"VQJSBC03\",\"byteLength\":"
         << bytecode.size() << ",\"byteHash\":" << fnv1a32(bytecode)
         << ",\"bytecode\":\"" << base64_encode(bytecode) << "\"}\n";
  return true;
}

} // namespace venom::compiler
