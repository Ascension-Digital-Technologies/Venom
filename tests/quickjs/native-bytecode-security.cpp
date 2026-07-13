#include "quickjs/bytecode.hpp"
#include "quickjs.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t read_u32(const std::vector<unsigned char>& bytes, std::size_t offset) {
  if (offset + 4u > bytes.size()) throw std::runtime_error("truncated u32");
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u) |
         (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u) |
         (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
}

bool contains(const std::vector<unsigned char>& haystack, const std::string& needle) {
  return std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end()) != haystack.end();
}

[[noreturn]] void fail(const std::string& message) {
  std::cerr << "native bytecode security test failed: " << message << '\n';
  std::exit(1);
}
} // namespace

int main() {
  const std::string source =
      "/* VENOM_SOURCE_RECOVERY_SENTINEL_7F31D9 */\n"
      "globalThis.__venomBytecodeResult = 40 + 2;\n";
  const auto record = venom::quickjs::compile_native_quickjs_bytecode(
      source, "native-bytecode-security.js", false);

  if (!venom::quickjs::is_native_quickjs_bytecode(record)) fail("VQJSBC03 magic missing");
  if (contains(record, "VQJSBC01") || contains(record, "VQJSBC02")) fail("legacy record marker present");
  if (contains(record, "VENOM_SOURCE_RECOVERY_SENTINEL_7F31D9")) fail("source comment survived stripped bytecode");
  if (contains(record, source)) fail("complete JavaScript source is embedded in the record");
  if (contains(record, "native-bytecode-security.js")) fail("compiler source filename survived stripped bytecode");
  if (record.size() < 48u || read_u32(record, 8u) != 3u) fail("invalid native record header");

  const auto bytecode_size = read_u32(record, 16u);
  const auto payload_offset = read_u32(record, 44u);
  if (payload_offset != 48u || payload_offset + bytecode_size != record.size()) fail("invalid payload bounds");

  JSRuntime* runtime = JS_NewRuntime();
  JSContext* context = runtime ? JS_NewContext(runtime) : nullptr;
  if (!runtime || !context) fail("could not create QuickJS runtime/context");

  JSValue object = JS_ReadObject(context, record.data() + payload_offset, bytecode_size, JS_READ_OBJ_BYTECODE);
  if (JS_IsException(object)) fail("JS_ReadObject rejected native payload");
  JSValue result = JS_EvalFunction(context, object);
  if (JS_IsException(result)) fail("JS_EvalFunction rejected native payload");
  JS_FreeValue(context, result);

  JSValue global = JS_GetGlobalObject(context);
  JSValue value = JS_GetPropertyStr(context, global, "__venomBytecodeResult");
  int32_t number = 0;
  if (JS_ToInt32(context, &number, value) != 0 || number != 42) fail("native bytecode did not execute correctly");
  JS_FreeValue(context, value);
  JS_FreeValue(context, global);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);

  const std::vector<venom::quickjs::ModuleSourceRecord> modules = {
      {"assets/entry.js", "s_entry_7a91", "import { value } from './dependency.js'; export const result = value + 1;"},
      {"assets/dependency.js", "s_dep_84c2", "export const value = 41;"},
  };
  const auto module_record = venom::quickjs::compile_native_quickjs_bytecode(
      modules.front().source, modules.front().compile_name, true, true, &modules);
  if (contains(module_record, "./dependency.js") || contains(module_record, "assets/dependency.js")) {
    fail("original module request survived native bytecode serialization");
  }
  if (!contains(module_record, "s_dep_84c2")) {
    fail("opaque module request was not serialized");
  }
  std::cout << "native VQJSBC03 compile/read/execute, source stripping, and module request redaction checks: PASS\n";
  return 0;
}
