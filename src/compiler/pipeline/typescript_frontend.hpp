#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

namespace venom::compiler::typescript {

struct TranspileResult {
  std::string javascript;
  std::size_t erased_type_declarations = 0;
  std::size_t erased_annotations = 0;
  std::size_t erased_assertions = 0;
  std::size_t lowered_jsx_elements = 0;
  std::string source_map;
  std::string frontend;
  std::string frontend_version;
  std::size_t diagnostic_count = 0;
  bool typescript = false;
  bool tsx = false;
};

struct CacheStats {
  std::size_t hits = 0;
  std::size_t misses = 0;
  std::size_t writes = 0;
};

void configure_cache(bool enabled, const std::filesystem::path& directory, bool verbose = false);
CacheStats cache_stats();
void reset_cache_stats();

bool is_typescript_path(const std::string& filename);
TranspileResult transpile(const std::string& source, const std::string& filename);
const char* frontend_name();
const char* frontend_version();

} // namespace venom::compiler::typescript
