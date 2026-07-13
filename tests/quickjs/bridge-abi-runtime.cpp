#include "quickjs/bytecode.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
std::uint32_t venom_qjs_context_alloc(void);
std::uint32_t venom_qjs_context_free(std::uint32_t);
std::uint32_t venom_qjs_context_set_limits(std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_context_set_execution_limits(std::uint32_t, std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_test_load_bytes(std::uint32_t, const unsigned char*, std::uint32_t);
std::uint32_t venom_qjs_script_buffer_set_expected_hash(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_execute_bytecode(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_bridge_abi(void);
std::uint32_t venom_qjs_test_bridge_load_bytes(std::uint32_t, const unsigned char*, std::uint32_t);
std::uint32_t venom_qjs_bridge_invoke(std::uint32_t, std::uint32_t);
std::uint32_t venom_qjs_bridge_result_size(void);
std::uint32_t venom_qjs_test_bridge_result_byte(std::uint32_t);
std::uint32_t venom_qjs_bridge_release(std::uint32_t);
const char* venom_qjs_test_exception_message(void);
}

static std::uint32_t fnv1a32(const std::vector<unsigned char>& bytes) {
  std::uint32_t h = 2166136261u;
  for (const auto byte : bytes) { h ^= byte; h *= 16777619u; }
  return h;
}

int main() {
  const auto ctx = venom_qjs_context_alloc();
  if (!ctx || venom_qjs_bridge_abi() != 1u) return 1;
  venom_qjs_context_set_limits(ctx, 8u * 1024u * 1024u, 256u * 1024u);
  venom_qjs_context_set_execution_limits(ctx, 1000000u, 1024u);
  const std::string registry_source = "globalThis.__venomProtectedBridge={\"math.js::add\":(a,b)=>({sum:a+b})};";
  const std::vector<venom::quickjs::ModuleSourceRecord> modules = {{"bridge-registry.js", "bridge-registry.js", registry_source}};
  const auto registry = venom::quickjs::compile_native_quickjs_module_bundle("bridge-registry.js", modules, false);
  if (!venom_qjs_test_load_bytes(ctx, registry.data(), static_cast<std::uint32_t>(registry.size()))) return 2;
  venom_qjs_script_buffer_set_expected_hash(ctx, fnv1a32(registry));
  if (!venom_qjs_execute_bytecode(ctx, static_cast<std::uint32_t>(registry.size()))) {
    std::cerr << venom_qjs_test_exception_message() << '\n';
    return 3;
  }
  const std::string request = "{\"candidate\":\"math.js::add\",\"args\":[7,9]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(request.data()), static_cast<std::uint32_t>(request.size()))) return 4;
  if (!venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(request.size()))) {
    std::cerr << venom_qjs_test_exception_message() << '\n';
    return 5;
  }
  std::string result;
  for (std::uint32_t i = 0; i < venom_qjs_bridge_result_size(); ++i) result.push_back(static_cast<char>(venom_qjs_test_bridge_result_byte(i)));
  if (result != "{\"sum\":16}") { std::cerr << result << '\n'; return 6; }
  if (!venom_qjs_bridge_release(ctx)) return 7;
  const std::string missing = "{\"candidate\":\"math.js::missing\",\"args\":[]}";
  if (!venom_qjs_test_bridge_load_bytes(ctx, reinterpret_cast<const unsigned char*>(missing.data()), static_cast<std::uint32_t>(missing.size()))) return 8;
  if (venom_qjs_bridge_invoke(ctx, static_cast<std::uint32_t>(missing.size())) != 0u) return 9;
  const std::string error = venom_qjs_test_exception_message();
  if (error.find("not registered") == std::string::npos) return 10;
  venom_qjs_bridge_release(ctx);
  venom_qjs_context_free(ctx);
  std::cout << "[venom] QuickJS bridge ABI runtime: PASS\n";
  return 0;
}
