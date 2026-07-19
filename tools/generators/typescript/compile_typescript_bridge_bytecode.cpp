#include "quickjs.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

static std::string read_text(const char* path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error(std::string("unable to read ") + path);
  return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
static std::string exception_message(JSContext* context) {
  JSValue exception = JS_GetException(context);
  const char* text = JS_ToCString(context, exception);
  std::string message = text ? text : "QuickJS bridge compiler exception";
  if (text) JS_FreeCString(context, text);
  JS_FreeValue(context, exception);
  return message;
}
int main(int argc, char** argv) {
  if (argc != 3) { std::cerr << "usage: compile_typescript_bridge_bytecode <bridge.js> <output.bin>\n"; return 2; }
  try {
    const auto source = read_text(argv[1]);
    JSRuntime* runtime = JS_NewRuntime();
    JSContext* context = runtime ? JS_NewContext(runtime) : nullptr;
    if (!runtime || !context) throw std::runtime_error("failed to create QuickJS compiler context");
    JSValue object = JS_Eval(context, source.data(), source.size(), "<venom-typescript-bridge>",
                             JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(object)) throw std::runtime_error(exception_message(context));
    size_t size = 0;
    uint8_t* bytes = JS_WriteObject(context, &size, object,
                                    JS_WRITE_OBJ_BYTECODE | JS_WRITE_OBJ_STRIP_SOURCE | JS_WRITE_OBJ_STRIP_DEBUG);
    JS_FreeValue(context, object);
    if (!bytes || !size) throw std::runtime_error("QuickJS emitted empty bridge bytecode");
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(size));
    js_free(context, bytes);
    JS_FreeContext(context); JS_FreeRuntime(runtime);
    std::cout << "compiled " << source.size() << " bridge source bytes to " << size << " bytecode bytes\n";
    return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << "\n"; return 1; }
}
