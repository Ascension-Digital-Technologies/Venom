#pragma once

#include "package/writer.hpp"

#include <filesystem>
#include <cstddef>
#include <vector>

namespace venom::package {

struct Package {
  std::uint32_t version = 0;
  std::uint32_t runtime_abi = 0;
  std::uint32_t flags = 0;
  std::uint64_t section_table_offset = 0;
  std::uint64_t section_table_size = 0;
  std::uint64_t name_table_offset = 0;
  std::uint64_t name_table_size = 0;
  std::uint64_t payload_offset = 0;
  std::uint64_t payload_size = 0;
  std::uint64_t package_hash = 0;
  std::uint64_t file_size = 0;
  std::vector<Section> sections;
};

Package read_package_bytes(const unsigned char* data, std::size_t size);
Package read_package_bytes(const std::vector<unsigned char>& bytes);
Package read_package(const std::filesystem::path& path);

} // namespace venom::package
