#include "venom/quickjs/bytecode.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

extern "C" {
std::uint32_t venom_qjs_context_alloc(void);
std::uint32_t venom_qjs_context_free(std::uint32_t);
std::uint32_t venom_qjs_context_set_limits(std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_context_set_execution_limits(std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_script_buffer_set_expected_hash(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_test_load_bytes(std::uint32_t, const unsigned char*, std::uint32_t);
const char* venom_qjs_test_exception_message(void);
std::uint32_t venom_qjs_test_source_byte(std::uint32_t);
std::uint32_t venom_qjs_script_buffer_free(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_bytecode_validate(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_execute_bytecode(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_console_event_count(void);
std::uint32_t venom_qjs_exception_message_ptr(void);
std::uint32_t venom_qjs_exception_message_size(void);
}

static std::uint32_t fnv1a32(const std::vector<unsigned char>& bytes) {
  std::uint32_t h = 2166136261u;
  for (const auto byte : bytes) { h ^= byte; h *= 16777619u; }
  return h;
}

int main() {
  const std::vector<venom::quickjs::ModuleSourceRecord> modules = {
      {"entry.js", "entry.js", "import { value } from './dep.js'; const m = await import('./dyn.js'); if (value + m.dynamicValue !== 42) throw new Error('module graph result mismatch');"},
      {"dep.js", "dep.js", "export const value = 40;"},
      {"dyn.js", "dyn.js", "export const dynamicValue = 2;"},
  };
  auto bundle = venom::quickjs::compile_native_quickjs_module_bundle("entry.js", modules, false);
  bool missing_rejected = false;
  try {
    const std::vector<venom::quickjs::ModuleSourceRecord> missing = {
        {"entry.js", "entry.js", "import { value } from './missing.js';"},
    };
    (void)venom::quickjs::compile_native_quickjs_module_bundle("entry.js", missing, false);
  } catch (const std::exception&) {
    missing_rejected = true;
  }
  if (!missing_rejected) return 7;
  if (bundle.size() < 24 || std::memcmp(bundle.data(), "VQJSMB04", 8) != 0) {
    std::cerr << "module bundle magic missing\n";
    return 1;
  }
  const auto ctx = venom_qjs_context_alloc();
  if (!ctx) return 2;
  venom_qjs_context_set_limits(ctx, 8u * 1024u * 1024u, 256u * 1024u);
  venom_qjs_context_set_execution_limits(ctx, 1000000u, 1024u);
  const auto ptr = 0u;
  if (!venom_qjs_test_load_bytes(ctx, bundle.data(), static_cast<std::uint32_t>(bundle.size()))) return 3;
  venom_qjs_script_buffer_set_expected_hash(ctx, fnv1a32(bundle));
  if (venom_qjs_bytecode_validate(ctx, static_cast<std::uint32_t>(bundle.size())) != 0u) return 4;
  if (venom_qjs_execute_bytecode(ctx, static_cast<std::uint32_t>(bundle.size())) != 1u) {
    if (const char* message = venom_qjs_test_exception_message()) std::cerr << message;
    std::cerr << "\nmodule bundle execution failed\n";
    return 5;
  }
  auto tampered = bundle;
  tampered.back() ^= 0x5au;
  if (!venom_qjs_test_load_bytes(ctx, tampered.data(), static_cast<std::uint32_t>(tampered.size()))) return 8;
  venom_qjs_script_buffer_set_expected_hash(ctx, fnv1a32(tampered));
  if (venom_qjs_bytecode_validate(ctx, static_cast<std::uint32_t>(tampered.size())) == 0u) return 9;
  if (venom_qjs_test_source_byte(0u) == 0u) return 10;
  venom_qjs_script_buffer_free(ctx, ptr);
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(tampered.size()); ++i) {
    if (venom_qjs_test_source_byte(i) != 0u) return 11;
  }
  if (!venom_qjs_test_load_bytes(ctx, bundle.data(), static_cast<std::uint32_t>(bundle.size()))) return 12;
  if (!venom_qjs_context_free(ctx)) return 13;
  for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(bundle.size()); ++i) {
    if (venom_qjs_test_source_byte(i) != 0u) return 14;
  }
  return 0;
}
