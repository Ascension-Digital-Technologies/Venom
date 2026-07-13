#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace venom::quickjs {

enum class BytecodeFormat : std::uint32_t {
  SourcePreservingV1 = 1,
  ProtectedPortableV2 = 2,
  NativeQuickJsObjectV1 = 3,
  NativeQuickJsObjectV3 = 4,
};

struct BytecodeChunkRecord {
  std::uint32_t order = 0;
  std::string route;
  std::string source;
  std::uint32_t source_size = 0;
  std::uint64_t source_hash = 0;
  std::uint32_t bytecode_size = 0;
  BytecodeFormat format = BytecodeFormat::NativeQuickJsObjectV3;
};

struct ModuleSourceRecord {
  std::string source_name;
  std::string compile_name;
  std::string source;
};

std::vector<unsigned char> compile_placeholder_bytecode(const std::string& source);
std::vector<unsigned char> compile_byte_buffer_record(const std::string& source);
std::vector<unsigned char> compile_protected_portable_bytecode(const std::string& source);
std::vector<unsigned char> compile_native_quickjs_bytecode(const std::string& source,
                                                           const std::string& source_name,
                                                           bool is_module,
                                                           bool redact_source_metadata = false,
                                                           const std::vector<ModuleSourceRecord>* module_sources = nullptr);
std::vector<unsigned char> compile_native_quickjs_module_bundle(const std::string& entry_source_name,
                                                               const std::vector<ModuleSourceRecord>& module_sources,
                                                               bool redact_source_metadata = false);
bool is_protected_portable_bytecode(const std::vector<unsigned char>& bytes);
bool is_native_quickjs_bytecode(const std::vector<unsigned char>& bytes);
std::string bytecode_format_name(BytecodeFormat format);
std::string bytecode_manifest_text(const std::vector<BytecodeChunkRecord>& chunks,
                                   std::uint32_t runtime_abi,
                                   std::uint32_t package_version);
std::string module_resolver_metadata_text(std::uint32_t runtime_abi,
                                          std::uint32_t package_version,
                                          std::size_t chunk_count);
std::string exception_abi_metadata_text(std::uint32_t runtime_abi,
                                        std::uint32_t package_version);
std::string host_trap_policy_metadata_text(bool strict_release,
                                           std::uint32_t runtime_abi,
                                           std::uint32_t package_version);

} // namespace venom::quickjs
