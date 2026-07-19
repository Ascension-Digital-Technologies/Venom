#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace venom::package {

using Sha256Digest = std::array<unsigned char, 32>;
using Sha384Digest = std::array<unsigned char, 48>;
using Sha512Digest = std::array<unsigned char, 64>;

std::uint64_t fnv1a64(const unsigned char* data, std::size_t size);
std::uint64_t fnv1a64(const std::vector<unsigned char>& data);

Sha256Digest sha256(const unsigned char* data, std::size_t size);
Sha256Digest sha256(const std::vector<unsigned char>& data);
std::string sha256_hex(const unsigned char* data, std::size_t size);
std::string sha256_hex(const std::vector<unsigned char>& data);
std::string sha256_base64(const unsigned char* data, std::size_t size);
std::string sha256_base64(const std::vector<unsigned char>& data);
std::string sha256_sri(const unsigned char* data, std::size_t size);
std::string sha256_sri(const std::vector<unsigned char>& data);

Sha384Digest sha384(const unsigned char* data, std::size_t size);
Sha384Digest sha384(const std::vector<unsigned char>& data);
std::string sha384_base64(const unsigned char* data, std::size_t size);
std::string sha384_base64(const std::vector<unsigned char>& data);
std::string sha384_sri(const unsigned char* data, std::size_t size);
std::string sha384_sri(const std::vector<unsigned char>& data);

Sha512Digest sha512(const unsigned char* data, std::size_t size);
Sha512Digest sha512(const std::vector<unsigned char>& data);
std::string sha512_base64(const unsigned char* data, std::size_t size);
std::string sha512_base64(const std::vector<unsigned char>& data);
std::string sha512_sri(const unsigned char* data, std::size_t size);
std::string sha512_sri(const std::vector<unsigned char>& data);
std::string bytes_to_base64(const unsigned char* data, std::size_t size);
std::string bytes_to_hex(const unsigned char* data, std::size_t size);

} // namespace venom::package
