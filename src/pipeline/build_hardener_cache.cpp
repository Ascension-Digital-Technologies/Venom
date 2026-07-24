#include "pipeline/build_support.hpp"
#include "pipeline/assets.hpp"
#include "pipeline/native_js_hardener.hpp"

#include "base/error.hpp"
#include "package/hash.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>

namespace venom::compiler::build_detail {
namespace {
std::mutex g_hardener_cache_mutex;
bool g_hardener_cache_enabled = true;
bool g_hardener_cache_verbose = false;
std::filesystem::path g_hardener_cache_directory;
std::atomic<std::size_t> g_hardener_cache_hits{0};
std::atomic<std::size_t> g_hardener_cache_misses{0};
std::atomic<std::size_t> g_hardener_cache_writes{0};
std::atomic<std::size_t> g_hardener_cache_invalidations{0};
std::atomic<std::size_t> g_hardener_cache_races{0};
std::atomic<std::uint64_t> g_hardener_cache_temp_counter{0};


std::string read_text_if_present(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  return std::string(std::istreambuf_iterator<char>(in), {});
}


constexpr std::string_view kHardenerCacheMagic = "VENOM-HARDENER-CACHE-V6\n";

std::string encode_hardener_cache_entry(const std::string& output) {
  const auto digest = venom::package::sha256_hex(bytes_from_string(output));
  return std::string(kHardenerCacheMagic) +
      "size=" + std::to_string(output.size()) + "\n" +
      "sha256=" + digest + "\n\n" + output;
}

bool decode_hardener_cache_entry(const std::string& entry, std::string& output) {
  if (entry.size() < kHardenerCacheMagic.size() || entry.compare(0, kHardenerCacheMagic.size(), kHardenerCacheMagic) != 0) return false;
  const auto size_begin = kHardenerCacheMagic.size();
  const auto size_end = entry.find('\n', size_begin);
  if (size_end == std::string::npos || entry.compare(size_begin, 5, "size=") != 0) return false;
  const auto hash_begin = size_end + 1;
  const auto hash_end = entry.find('\n', hash_begin);
  if (hash_end == std::string::npos || entry.compare(hash_begin, 7, "sha256=") != 0) return false;
  if (hash_end + 1 >= entry.size() || entry[hash_end + 1] != '\n') return false;

  std::size_t declared_size = 0;
  try {
    const auto size_text = entry.substr(size_begin + 5, size_end - (size_begin + 5));
    std::size_t consumed = 0;
    declared_size = std::stoull(size_text, &consumed);
    if (consumed != size_text.size()) return false;
  } catch (...) {
    return false;
  }

  const auto declared_hash = entry.substr(hash_begin + 7, hash_end - (hash_begin + 7));
  if (declared_hash.size() != 64) return false;
  const auto payload_begin = hash_end + 2;
  if (declared_size != entry.size() - payload_begin) return false;
  output.assign(entry.data() + payload_begin, declared_size);
  return venom::package::sha256_hex(bytes_from_string(output)) == declared_hash;
}

enum class CachePublishResult { Published, ReusedConcurrentEntry };

std::filesystem::path unique_cache_temporary_path(const std::filesystem::path& path) {
  const auto sequence = g_hardener_cache_temp_counter.fetch_add(1, std::memory_order_relaxed);
  const auto thread_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  return path.string() + ".tmp." + std::to_string(stamp) + "." +
      std::to_string(thread_hash) + "." + std::to_string(sequence);
}

CachePublishResult write_text_atomic(const std::filesystem::path& path, const std::string& content) {
  std::filesystem::create_directories(path.parent_path());
  const auto temporary = unique_cache_temporary_path(path);
  {
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) raise_error("VENOM-E5000", "failed to write hardener cache entry " + temporary.string());
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.flush();
    if (!out) {
      std::error_code cleanup_error;
      std::filesystem::remove(temporary, cleanup_error);
      raise_error("VENOM-E5000", "failed to flush hardener cache entry " + temporary.string());
    }
  }

  std::error_code error;
  std::filesystem::rename(temporary, path, error);
  if (!error) return CachePublishResult::Published;

  // Another Venom process may have published the same content after our cache
  // miss. Reuse a valid winner instead of deleting or replacing it.
  const auto concurrent_entry = read_text_if_present(path);
  std::string concurrent_output;
  if (!concurrent_entry.empty() && decode_hardener_cache_entry(concurrent_entry, concurrent_output)) {
    std::error_code cleanup_error;
    std::filesystem::remove(temporary, cleanup_error);
    return CachePublishResult::ReusedConcurrentEntry;
  }

  // The destination exists but is incomplete or corrupt. Remove only that
  // invalid destination, then retry our same-directory atomic rename.
  std::error_code remove_error;
  std::filesystem::remove(path, remove_error);
  error.clear();
  std::filesystem::rename(temporary, path, error);
  if (!error) return CachePublishResult::Published;

  std::error_code cleanup_error;
  std::filesystem::remove(temporary, cleanup_error);
  raise_error("VENOM-E5000", "failed to publish hardener cache entry " + path.string());
}
}
HardenerCacheStats hardener_cache_stats() {
  return {g_hardener_cache_hits.load(), g_hardener_cache_misses.load(),
          g_hardener_cache_writes.load(), g_hardener_cache_invalidations.load(),
          g_hardener_cache_races.load()};
}
void reset_hardener_cache_stats() {
  g_hardener_cache_hits = 0;
  g_hardener_cache_misses = 0;
  g_hardener_cache_writes = 0;
  g_hardener_cache_invalidations = 0;
  g_hardener_cache_races = 0;
}

void configure_hardener_cache(bool enabled, const std::filesystem::path& directory, bool verbose) {
  std::lock_guard<std::mutex> lock(g_hardener_cache_mutex);
  g_hardener_cache_enabled = enabled;
  g_hardener_cache_verbose = verbose;
  g_hardener_cache_directory = directory;
}

std::string ast_harden_release_js(const std::string& kind, const std::string& js, const std::string& build_salt) {
  const auto nonce = short_hash_hex(
      venom::package::fnv1a64(bytes_from_string("hardener-v6|" + build_salt + "|" + kind + "|" + js)), 16);
  const auto seed = static_cast<std::uint32_t>(
      venom::package::fnv1a64(bytes_from_string("obfuscate|" + kind + "|" + nonce)) & 0xffffffffu);

  std::filesystem::path cache_file;
  bool cache_enabled = false;
  bool cache_verbose = false;
  {
    std::lock_guard<std::mutex> lock(g_hardener_cache_mutex);
    cache_enabled = g_hardener_cache_enabled && !g_hardener_cache_directory.empty();
    cache_verbose = g_hardener_cache_verbose;
    if (cache_enabled) {
      const auto identity = std::string{"venom-hardener-cache-v6|"} + build_salt + "|" + native_js_hardener::terser_version() + "|" +
          native_js_hardener::obfuscator_version() + "|" + kind + "|" + std::to_string(seed) + "|" + js;
      const auto digest = venom::package::sha256_hex(bytes_from_string(identity));
      cache_file = g_hardener_cache_directory / digest.substr(0, 2) / (digest + ".js");
    }
  }

  if (cache_enabled) {
    const auto cached_entry = read_text_if_present(cache_file);
    if (!cached_entry.empty()) {
      std::string cached_output;
      if (decode_hardener_cache_entry(cached_entry, cached_output)) {
        ++g_hardener_cache_hits;
        if (cache_verbose) std::cout << "[CACHE]    hardener hit: " << kind << " (" << cached_output.size() << " bytes)\n";
        return cached_output;
      }
      ++g_hardener_cache_invalidations;
      std::error_code remove_error;
      std::filesystem::remove(cache_file, remove_error);
      if (cache_verbose) std::cout << "[CACHE]    hardener invalidated corrupt entry: " << kind << "\n";
    }
    ++g_hardener_cache_misses;
    if (cache_verbose) std::cout << "[CACHE]    hardener miss: " << kind << " (" << js.size() << " bytes)\n" << std::flush;
  }

  const auto output = native_js_hardener::harden(kind, js, seed);
  if (output.empty()) {
    raise_error("VENOM-E5000", "embedded protected release JS hardener produced empty output for " + kind);
  }
  if (cache_enabled) {
    std::lock_guard<std::mutex> lock(g_hardener_cache_mutex);
    const auto publish_result = write_text_atomic(cache_file, encode_hardener_cache_entry(output));
    if (publish_result == CachePublishResult::Published) {
      ++g_hardener_cache_writes;
    } else {
      ++g_hardener_cache_races;
    }
  }
  return output;
}

} // namespace venom::compiler::build_detail
