#pragma once
#include <filesystem>
#include <string>

namespace venom::compiler {
struct UpdateOptions {
  std::string action;
  std::string channel = "stable";
  std::string manifest;
  std::filesystem::path prefix;
  std::filesystem::path public_key;
  bool require_signature = false;
  bool yes = false;
  bool dry_run = false;
  std::string format = "text";
};
bool manage_update(const UpdateOptions& options, const std::filesystem::path& executable);
}
