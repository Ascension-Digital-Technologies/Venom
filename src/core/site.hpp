#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler {

struct SiteFile {
  std::filesystem::path absolute;
  std::string relative;
  std::string extension;
  std::vector<unsigned char> bytes;
};

struct SiteGraph {
  std::filesystem::path root;
  std::vector<SiteFile> files;
  std::vector<std::string> routes;
  std::vector<std::string> styles;
  std::vector<std::string> scripts;
  std::vector<std::string> assets;
};

SiteGraph collect_site(const std::filesystem::path& root,
                       const std::filesystem::path& excluded_output = {});
std::string route_from_html_path(const std::string& relative);

} // namespace venom::compiler
