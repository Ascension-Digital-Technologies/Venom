#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace venom::turbojs {

constexpr std::uint32_t kRuntimeAbiVersion = 12;
constexpr std::uint32_t kRuntimePackageVersion = 83;
constexpr std::uint32_t kDefaultHeapLimitBytes = 8u * 1024u * 1024u;
constexpr std::uint32_t kDefaultStackLimitBytes = 256u * 1024u;
constexpr std::uint32_t kDefaultScriptBufferLimitBytes = 768u * 1024u;
constexpr std::uint32_t kDefaultContextLimit = 64u;
constexpr std::uint32_t kDefaultHostCallLimit = 4096u;
constexpr std::uint32_t kDefaultConsoleEventLimit = 1024u;
constexpr std::uint32_t kDefaultModuleRecordLimit = 512u;

struct AbiEntry {
  const char* name;
  const char* kind;
  const char* signature;
  const char* purpose;
};

const std::vector<AbiEntry>& runtime_abi_entries();
std::string runtime_abi_table_text();
std::string host_import_table_text();
std::uint64_t abi_table_hash();
std::uint64_t host_import_table_hash();

struct ParityProbe {
  std::uint32_t abi = kRuntimeAbiVersion;
  std::uint32_t package_version = kRuntimePackageVersion;
  std::uint64_t abi_hash = 0;
  std::uint64_t host_import_hash = 0;
  std::string native_eval;
};

std::string parity_probe_text(const ParityProbe& probe);

} // namespace venom::turbojs
