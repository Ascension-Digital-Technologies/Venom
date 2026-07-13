#pragma once
#include "compiler/cli.hpp"
#include <filesystem>

namespace venom::compiler {
void apply_project_config(const std::filesystem::path& path, BuildOptions& options);
std::filesystem::path discover_project_config(const std::filesystem::path& start);
}
