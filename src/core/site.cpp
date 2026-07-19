#include "venom/base/error.hpp"
#include "venom/core/site.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace venom::compiler {

namespace {
std::string normalize_path(std::filesystem::path path) {
  auto value = path.generic_string();
  while (!value.empty() && value.front() == '/') {
    value.erase(value.begin());
  }
  return value;
}

std::string lower_ext(std::filesystem::path path) {
  auto ext = path.extension().generic_string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return ext;
}

std::vector<unsigned char> read_all(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    raise_error("VENOM-E1100", "failed to read " + path.string());
  }
  return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
}

bool is_root_project_metadata(const std::string& relative) {
  if (relative.find('/') != std::string::npos) return false;
  std::string name = relative;
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  static const char* const ignored[] = {
      "readme", "readme.md", "readme.txt",
      "changes", "changes.md", "changelog", "changelog.md",
      "contributing", "contributing.md", "security", "security.md",
      "license", "license.md", "license.txt",
      "package.json", "package-lock.json", "yarn.lock", "pnpm-lock.yaml"
  };
  return std::find(std::begin(ignored), std::end(ignored), name) != std::end(ignored);
}
} // namespace

std::string route_from_html_path(const std::string& relative) {
  if (relative == "index.html") {
    return "/";
  }
  std::string route = relative;
  if (route.size() >= 5 && route.substr(route.size() - 5) == ".html") {
    route.resize(route.size() - 5);
  }
  if (route.size() >= 6 && route.substr(route.size() - 6) == "/index") {
    route.resize(route.size() - 6);
  }
  return "/" + route;
}

SiteGraph collect_site(const std::filesystem::path& input_root) {
  const auto root = std::filesystem::absolute(input_root);
  if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
    raise_error("VENOM-E1100", "input is not a directory: " + input_root.string());
  }

  SiteGraph graph;
  graph.root = root;

  for (auto iterator = std::filesystem::recursive_directory_iterator(root);
       iterator != std::filesystem::recursive_directory_iterator(); ++iterator) {
    const auto& entry = *iterator;
    const auto absolute = entry.path();
    const auto relative = normalize_path(std::filesystem::relative(absolute, root));
    if (entry.is_directory()) {
      if (relative == ".venom-cache" || relative.rfind(".venom-cache/", 0) == 0 ||
          relative == ".git" || relative.rfind(".git/", 0) == 0) {
        iterator.disable_recursion_pending();
      }
      continue;
    }
    if (!entry.is_regular_file() || relative == "venom.lock" || is_root_project_metadata(relative)) {
      continue;
    }

    const auto ext = lower_ext(absolute);
    SiteFile file{absolute, relative, ext, read_all(absolute)};

    if (ext == ".html" || ext == ".htm") {
      graph.routes.push_back(route_from_html_path(relative));
    } else if (ext == ".css") {
      graph.styles.push_back(relative);
    } else if (ext == ".js" || ext == ".mjs" || ext == ".ts" || ext == ".tsx" || ext == ".mts" || ext == ".cts") {
      graph.scripts.push_back(relative);
    } else {
      graph.assets.push_back(relative);
    }

    graph.files.push_back(std::move(file));
  }

  std::sort(graph.files.begin(), graph.files.end(), [](const SiteFile& a, const SiteFile& b) {
    return a.relative < b.relative;
  });
  std::sort(graph.routes.begin(), graph.routes.end());
  std::sort(graph.styles.begin(), graph.styles.end());
  std::sort(graph.scripts.begin(), graph.scripts.end());
  std::sort(graph.assets.begin(), graph.assets.end());

  return graph;
}

} // namespace venom::compiler
