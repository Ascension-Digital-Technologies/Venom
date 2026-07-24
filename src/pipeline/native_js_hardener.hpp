#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace venom::compiler::native_js_hardener {

struct HardenerRuntimeStats {
  std::size_t initializations = 0;
  std::size_t calls = 0;
  std::size_t resets = 0;
  std::size_t fallbacks = 0;
  std::size_t subprocess_calls = 0;
  std::size_t subprocess_retries = 0;
  std::size_t subprocess_failures = 0;
  std::size_t input_bytes = 0;
  std::size_t output_bytes = 0;
  double initialization_ms = 0.0;
  double execution_ms = 0.0;
  double total_ms = 0.0;
};

void configure_worker_executable(std::string path);
std::string harden(const std::string& kind, const std::string& source, std::uint32_t seed);
std::string harden_in_process(const std::string& kind, const std::string& source, std::uint32_t seed);
HardenerRuntimeStats runtime_stats();
void reset_runtime_stats();
void shutdown();
const char* terser_version();
const char* obfuscator_version();

}  // namespace venom::compiler::native_js_hardener
