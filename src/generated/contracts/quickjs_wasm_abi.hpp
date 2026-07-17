#pragma once
// Generated from contracts/quickjs-wasm-abi.json. Do not edit.
#include <array>
#include <string_view>
namespace venom::generated::quickjs_wasm_abi {
inline constexpr unsigned runtime_abi = 12u;
inline constexpr unsigned bridge_abi = 1u;
inline constexpr std::string_view bytecode_format = "VQJSBC03";
inline constexpr std::array<std::string_view, 24> required_exports = {
  "memory",
  "venom_qjs_context_alloc",
  "venom_qjs_context_free",
  "venom_qjs_context_set_limits",
  "venom_qjs_context_set_execution_limits",
  "venom_qjs_script_buffer_alloc",
  "venom_qjs_script_buffer_capacity",
  "venom_qjs_script_buffer_set_expected_hash",
  "venom_qjs_script_buffer_free",
  "venom_qjs_bytecode_validate",
  "venom_qjs_execute_bytecode",
  "venom_qjs_compact_result_ptr",
  "venom_qjs_diversification_install",
  "venom_qjs_exception_message_ptr",
  "venom_qjs_exception_message_size",
  "venom_qjs_exception_clear",
  "venom_qjs_upstream_quickjs_ready",
  "venom_qjs_bridge_abi",
  "venom_qjs_bridge_input_alloc",
  "venom_qjs_bridge_input_capacity",
  "venom_qjs_bridge_invoke",
  "venom_qjs_bridge_result_ptr",
  "venom_qjs_bridge_result_size",
  "venom_qjs_bridge_release",
};
inline constexpr std::array<std::string_view, 2> optional_exports = {
  "venom_qjs_result_ptr",
  "venom_qjs_result_size",
};
inline constexpr std::array<std::string_view, 5> allowed_toolchain_exports = {
  "__indirect_function_table",
  "_emscripten_stack_restore",
  "emscripten_stack_get_current",
  "_initialize",
  "__cxa_increment_exception_refcount",
};
inline constexpr std::array<std::string_view, 1> development_trust_domains = {
  "development-quickjs-compiler",
};
inline constexpr std::array<std::string_view, 1> release_trust_domains = {
  "package-decoder-wasm",
};
}
