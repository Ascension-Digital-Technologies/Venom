#include "compiler/commands/dist_analyzer.hpp"
#include "package/hash.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace venom::compiler {
namespace {

struct FileInfo {
  std::filesystem::path relative;
  std::uintmax_t size = 0;
  std::string category;
  std::string digest;
};

std::vector<unsigned char> read_bytes(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::runtime_error("failed to read distribution file: " + path.string());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

std::string category_for(const std::filesystem::path& path) {
  const auto ext = lower(path.extension().string());
  const auto generic = path.generic_string();
  if (ext == ".wasm") return "WebAssembly";
  if (ext == ".vbc" || ext == ".pax") return "Protected package";
  if (ext == ".js" || ext == ".mjs" || ext == ".cjs") {
    if (generic.find("assets/javascript/") != std::string::npos || generic.find("assets/runtime/") != std::string::npos || generic.find("assets/loader/") != std::string::npos || generic.find("assets/workers/") != std::string::npos) return "Venom runtime JavaScript";
    return "Browser JavaScript";
  }
  if (ext == ".css") return "Styles";
  if (ext == ".html" || ext == ".htm") return "HTML";
  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif" || ext == ".webp" || ext == ".svg" || ext == ".ico" || ext == ".avif") return "Images";
  if (ext == ".woff" || ext == ".woff2" || ext == ".ttf" || ext == ".otf" || ext == ".eot") return "Fonts";
  if (ext == ".json") return "Metadata";
  return "Other";
}

std::string escape_json(const std::string& value) {
  std::ostringstream out;
  for (const char raw_c : value) {
    const auto c = static_cast<unsigned char>(raw_c);
    switch (c) {
      case '"': out << "\\\""; break;
      case '\\': out << "\\\\"; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
        else out << static_cast<char>(c);
    }
  }
  return out.str();
}

std::string human_bytes(std::uintmax_t bytes) {
  static const char* units[] = {"B", "KiB", "MiB", "GiB"};
  double value = static_cast<double>(bytes);
  std::size_t unit = 0;
  while (value >= 1024.0 && unit + 1 < 4) { value /= 1024.0; ++unit; }
  std::ostringstream out;
  out << std::fixed << std::setprecision(unit == 0 ? 0 : 2) << value << ' ' << units[unit];
  return out.str();
}

bool contains_forbidden_marker(const std::filesystem::path& path, std::string& marker) {
  const auto ext = lower(path.extension().string());
  if (ext != ".js" && ext != ".json" && ext != ".html" && ext != ".txt") return false;
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  static const std::vector<std::string> markers = {
    "function minimax", "Piece Square Tables", "VAEAD001", "VSEAL001",
    "VENOM_QJS_RUNTIME_ABI_", "js/ai-engine.js::", "src/compiler/"
  };
  for (const auto& item : markers) {
    if (text.find(item) != std::string::npos) { marker = item; return true; }
  }
  return false;
}

} // namespace

bool analyze_distribution(const DistAnalyzeOptions& options) {
  const auto root = std::filesystem::absolute(options.input);
  if (!std::filesystem::is_directory(root)) throw std::runtime_error("analyze requires a distribution directory");

  std::vector<FileInfo> files;
  std::map<std::string, std::uintmax_t> category_sizes;
  std::map<std::string, std::vector<std::filesystem::path>> duplicate_groups;
  std::vector<std::filesystem::path> loose_asset_files;
  std::vector<std::pair<std::filesystem::path, std::string>> leak_markers;
  std::uintmax_t total = 0;

  for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) continue;
    const auto relative = std::filesystem::relative(entry.path(), root);
    const auto size = entry.file_size();
    auto bytes = read_bytes(entry.path());
    const auto digest = venom::package::sha256_hex(bytes);
    const auto category = category_for(relative);
    files.push_back({relative, size, category, digest});
    total += size;
    category_sizes[category] += size;
    duplicate_groups[digest].push_back(relative);
    if (relative.parent_path() == std::filesystem::path("assets")) loose_asset_files.push_back(relative);
    std::string marker;
    if (contains_forbidden_marker(entry.path(), marker)) leak_markers.emplace_back(relative, marker);
  }

  std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) { return a.size > b.size; });
  std::uintmax_t duplicate_waste = 0;
  std::vector<std::vector<std::filesystem::path>> duplicates;
  for (const auto& duplicate_entry : duplicate_groups) {
    const auto& digest = duplicate_entry.first;
    const auto& paths = duplicate_entry.second;
    if (paths.size() < 2) continue;
    duplicates.push_back(paths);
    const auto it = std::find_if(files.begin(), files.end(), [&digest](const FileInfo& info) { return info.digest == digest; });
    if (it != files.end()) duplicate_waste += it->size * (paths.size() - 1);
  }

  const bool clean = loose_asset_files.empty() && leak_markers.empty();
  if (options.format == OutputFormat::Json) {
    std::cout << "{\n  \"root\": \"" << escape_json(root.generic_string()) << "\",\n"
              << "  \"fileCount\": " << files.size() << ",\n"
              << "  \"totalBytes\": " << total << ",\n"
              << "  \"duplicateBytes\": " << duplicate_waste << ",\n"
              << "  \"clean\": " << (clean ? "true" : "false") << ",\n"
              << "  \"categories\": {";
    bool first = true;
    for (const auto& [name, size] : category_sizes) {
      if (!first) std::cout << ',';
      std::cout << "\n    \"" << escape_json(name) << "\": " << size;
      first = false;
    }
    std::cout << "\n  },\n  \"largestFiles\": [";
    for (std::size_t i = 0; i < std::min<std::size_t>(10, files.size()); ++i) {
      if (i) std::cout << ',';
      std::cout << "\n    {\"path\": \"" << escape_json(files[i].relative.generic_string()) << "\", \"bytes\": " << files[i].size << ", \"category\": \"" << escape_json(files[i].category) << "\"}";
    }
    std::cout << "\n  ],\n  \"looseAssets\": [";
    for (std::size_t i = 0; i < loose_asset_files.size(); ++i) { if (i) std::cout << ','; std::cout << "\"" << escape_json(loose_asset_files[i].generic_string()) << "\""; }
    std::cout << "],\n  \"leakMarkers\": [";
    for (std::size_t i = 0; i < leak_markers.size(); ++i) { if (i) std::cout << ','; std::cout << "{\"path\":\"" << escape_json(leak_markers[i].first.generic_string()) << "\",\"marker\":\"" << escape_json(leak_markers[i].second) << "\"}"; }
    std::cout << "]\n}\n";
    return clean;
  }

  std::cout << "Venom distribution analysis\n"
            << "  root: " << root.string() << "\n"
            << "  files: " << files.size() << "\n"
            << "  total: " << human_bytes(total) << "\n"
            << "  duplicate waste: " << human_bytes(duplicate_waste) << "\n\n"
            << "Composition\n";
  for (const auto& [name, size] : category_sizes) {
    const double pct = total ? (100.0 * static_cast<double>(size) / static_cast<double>(total)) : 0.0;
    std::cout << "  " << std::left << std::setw(28) << name << std::right << std::setw(12) << human_bytes(size) << "  " << std::fixed << std::setprecision(1) << pct << "%\n";
  }
  std::cout << "\nLargest files\n";
  for (std::size_t i = 0; i < std::min<std::size_t>(10, files.size()); ++i) {
    std::cout << "  " << std::left << std::setw(62) << files[i].relative.generic_string() << std::right << human_bytes(files[i].size) << "\n";
  }
  std::cout << "\nLayout\n";
  if (loose_asset_files.empty()) std::cout << "  [PASS] no loose files at assets/ root\n";
  else for (const auto& path : loose_asset_files) std::cout << "  [WARN] loose asset: " << path.generic_string() << "\n";
  if (duplicates.empty()) std::cout << "  [PASS] no byte-identical duplicate files\n";
  else std::cout << "  [WARN] " << duplicates.size() << " duplicate group(s), " << human_bytes(duplicate_waste) << " avoidable\n";
  if (leak_markers.empty()) std::cout << "  [PASS] no known protected-source or legacy decoder markers\n";
  else for (const auto& [path, marker] : leak_markers) std::cout << "  [FAIL] marker \"" << marker << "\" in " << path.generic_string() << "\n";
  std::cout << "\nResult: " << (clean ? "PASS" : "REVIEW REQUIRED") << "\n";
  return clean;
}

} // namespace venom::compiler
