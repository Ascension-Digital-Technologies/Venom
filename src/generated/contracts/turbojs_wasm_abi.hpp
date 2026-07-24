#pragma once
// Generated from contracts/turbojs-wasm-abi.json. Do not edit.
#include <array>
#include <string_view>
namespace venom::generated::turbojs_wasm_abi {
inline constexpr unsigned runtime_abi = 12u;
inline constexpr unsigned bridge_abi = 1u;
inline constexpr unsigned release_abi_fingerprint = 0x33141281u;
inline constexpr std::string_view bytecode_format = "VTJSBC03";
inline constexpr std::array<std::string_view, 24> required_exports = {
  "memory",
  "venom_tjs_context_alloc",
  "venom_tjs_context_free",
  "venom_tjs_context_set_limits",
  "venom_tjs_context_set_execution_limits",
  "venom_tjs_script_buffer_alloc",
  "venom_tjs_script_buffer_capacity",
  "venom_tjs_script_buffer_set_expected_hash",
  "venom_tjs_script_buffer_free",
  "venom_tjs_bytecode_validate",
  "venom_tjs_execute_bytecode",
  "venom_tjs_compact_result_ptr",
  "venom_tjs_diversification_install",
  "venom_tjs_exception_message_ptr",
  "venom_tjs_exception_message_size",
  "venom_tjs_exception_clear",
  "venom_tjs_upstream_turbojs_ready",
  "venom_tjs_bridge_abi",
  "venom_tjs_bridge_input_alloc",
  "venom_tjs_bridge_input_capacity",
  "venom_tjs_bridge_invoke",
  "venom_tjs_bridge_result_ptr",
  "venom_tjs_bridge_result_size",
  "venom_tjs_bridge_release",
};
inline constexpr std::array<std::string_view, 2> optional_exports = {
  "venom_tjs_result_ptr",
  "venom_tjs_result_size",
};
inline constexpr std::array<std::string_view, 5> allowed_toolchain_exports = {
  "__indirect_function_table",
  "_emscripten_stack_restore",
  "emscripten_stack_get_current",
  "_initialize",
  "__cxa_increment_exception_refcount",
};
inline constexpr std::array<std::string_view, 1> development_trust_domains = {
  "development-turbojs-compiler",
};
inline constexpr std::array<std::string_view, 1> release_trust_domains = {
  "package-decoder-wasm",
};
}
