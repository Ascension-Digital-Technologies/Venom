#pragma once
#include <filesystem>
#include <string>
namespace venom::compiler {
struct RuntimeOptions { std::string action; std::filesystem::path directory; bool force = false; };
bool manage_runtime(const RuntimeOptions& options);
std::filesystem::path default_runtime_directory();
}
