#pragma once
#include <filesystem>
#include <string>

namespace venom::compiler {
struct NewProjectOptions { std::filesystem::path directory; bool force = false; };
struct InitProjectOptions { std::filesystem::path directory = "."; bool force = false; };
bool create_project(const NewProjectOptions& options);
bool initialize_project(const InitProjectOptions& options);
}
