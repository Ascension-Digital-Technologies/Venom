#pragma once

#include <cstddef>
#include <vector>

namespace venom::package {

// Small dependency-free RLE codec used as the first production compression
// boundary. The package format carries raw/stored sizes separately, so this
// codec is deliberately simple and deterministic; it can be replaced by zstd
// or brotli behind the same API later.
bool compression_is_worthwhile(const std::vector<unsigned char>& input);
std::vector<unsigned char> compress_rle_v1(const std::vector<unsigned char>& input);
std::vector<unsigned char> decompress_rle_v1(const std::vector<unsigned char>& input, std::size_t expected_size);

std::vector<unsigned char> compress_none(const std::vector<unsigned char>& input);
std::vector<unsigned char> decompress_none(const std::vector<unsigned char>& input);

} // namespace venom::package
