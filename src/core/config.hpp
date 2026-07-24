#pragma once
#include "core/options.hpp"
#include <filesystem>

namespace venom::compiler {
void apply_project_config(const std::filesystem::path& path, BuildOptions& options);
std::filesystem::path discover_project_config(const std::filesystem::path& start);
bool validate_project_config(const std::filesystem::path& path, std::string* error = nullptr);
void print_project_config(const std::filesystem::path& path, OutputFormat format);
}
