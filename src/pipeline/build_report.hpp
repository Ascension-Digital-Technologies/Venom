#pragma once

#include "pipeline/build.hpp"
#include "core/profile.hpp"
#include "package/reader.hpp"

#include <filesystem>
#include <vector>

namespace venom::compiler::build_report_detail {
std::size_t count_marker(const std::vector<unsigned char>& bytes, const std::string& marker);
void print_build_protection_report(const Profile& profile,
                                   const BuildOptions& options,
                                   const std::filesystem::path& package_path,
                                   const std::vector<unsigned char>& package_bytes,
                                   const venom::package::Package& package,
                                   bool external_manifest);
}
