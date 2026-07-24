#include "build_support.hpp"
#include "pipeline/native_js_hardener.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;

const std::string kSource = "const value=41+1;globalThis.__cacheIntegrity=value;";
const std::string kSalt = "integrity-seed";

std::string shell_quote(const fs::path& path) {
#ifdef _WIN32
  std::string value = path.string();
  std::string escaped;
  escaped.reserve(value.size() + 2);
  escaped.push_back('"');
  for (const char ch : value) {
    if (ch == '"') escaped += "\\\"";
    else escaped.push_back(ch);
  }
  escaped.push_back('"');
  return escaped;
#else
  std::string escaped = "'";
  for (const char ch : path.string()) {
    if (ch == '\'') escaped += "'\\''";
    else escaped.push_back(ch);
  }
  escaped.push_back('\'');
  return escaped;
#endif
}

int child_publish(const fs::path& cache_root) {
  using namespace venom::compiler::build_detail;
  configure_hardener_cache(true, cache_root, false);
  const auto output = ast_harden_release_js("runtime", kSource, kSalt);
  configure_hardener_cache(false, {}, false);
  venom::compiler::native_js_hardener::shutdown();
  return output.empty() ? 20 : 0;
}

bool has_temporary_files(const fs::path& root) {
  std::error_code error;
  if (!fs::exists(root, error)) return false;
  for (const auto& entry : fs::recursive_directory_iterator(root)) {
    if (entry.is_regular_file() && entry.path().filename().string().find(".tmp.") != std::string::npos) return true;
  }
  return false;
}
}  // namespace

int main(int argc, char** argv) {
  using namespace venom::compiler::build_detail;
  namespace fs = std::filesystem;

  if (argc == 3 && std::string(argv[1]) == "--publish-child") {
    return child_publish(argv[2]);
  }

  const auto cache_root = fs::temp_directory_path() / "venom-hardener-cache-integrity";
  const auto race_root = fs::temp_directory_path() / "venom-hardener-cache-race";
  std::error_code error;
  fs::remove_all(cache_root, error);
  fs::remove_all(race_root, error);

  // Exercise publication from independent processes. A fixed .tmp filename
  // makes these writers corrupt or delete one another; unique same-directory
  // temporary files leave one valid cache entry and no debris.
  std::vector<std::future<int>> publishers;
  const auto executable = fs::absolute(argv[0]);
  const auto command = shell_quote(executable) + " --publish-child " + shell_quote(race_root);
  for (int index = 0; index < 4; ++index) {
    publishers.emplace_back(std::async(std::launch::async, [command] { return std::system(command.c_str()); }));
  }
  for (auto& publisher : publishers) {
    if (publisher.get() != 0) {
      std::cerr << "concurrent hardener cache publisher failed\n";
      return 1;
    }
  }
  if (has_temporary_files(race_root)) {
    std::cerr << "concurrent hardener cache publication left temporary files\n";
    return 2;
  }

  configure_hardener_cache(true, race_root, false);
  reset_hardener_cache_stats();
  const auto raced_output = ast_harden_release_js("runtime", kSource, kSalt);
  const auto raced_stats = hardener_cache_stats();
  if (raced_output.empty() || raced_stats.hits != 1 || raced_stats.invalidations != 0) {
    std::cerr << "concurrently published hardener cache entry was not reusable\n";
    return 3;
  }
  configure_hardener_cache(false, {}, false);
  venom::compiler::native_js_hardener::shutdown();

  configure_hardener_cache(true, cache_root, false);
  reset_hardener_cache_stats();
  venom::compiler::native_js_hardener::reset_runtime_stats();

  const auto first = ast_harden_release_js("runtime", kSource, kSalt);
  const auto after_first = hardener_cache_stats();
  if (first.empty() || after_first.misses != 1 || after_first.writes != 1 || after_first.hits != 0) {
    std::cerr << "unexpected first cache result\n";
    return 4;
  }

  fs::path cache_file;
  for (const auto& entry : fs::recursive_directory_iterator(cache_root)) {
    if (entry.is_regular_file() && entry.path().extension() == ".js") {
      cache_file = entry.path();
      break;
    }
  }
  if (cache_file.empty()) {
    std::cerr << "hardener cache entry was not written\n";
    return 5;
  }

  {
    std::ofstream corrupt(cache_file, std::ios::binary | std::ios::trunc);
    corrupt << "VENOM-HARDENER-CACHE-V6\nsize=999\nsha256=broken\n\ntruncated";
  }

  const auto second = ast_harden_release_js("runtime", kSource, kSalt);
  const auto after_second = hardener_cache_stats();
  if (second != first || after_second.invalidations != 1 || after_second.misses != 2 || after_second.writes != 2) {
    std::cerr << "corrupt hardener cache entry was not regenerated safely\n";
    return 6;
  }

  const auto third = ast_harden_release_js("runtime", kSource, kSalt);
  const auto after_third = hardener_cache_stats();
  if (third != first || after_third.hits != 1 || after_third.invalidations != 1) {
    std::cerr << "regenerated hardener cache entry was not reusable\n";
    return 7;
  }

  configure_hardener_cache(false, {}, false);
  venom::compiler::native_js_hardener::shutdown();
  fs::remove_all(cache_root, error);
  fs::remove_all(race_root, error);
  std::cout << "[venom] hardener cache integrity and concurrent publication: PASS\n";
  return 0;
}
