#include "venom/base/error.hpp"
#include "venom/internal/frontends/typescript/typescript_runtime.hpp"

#include "venom/core/diagnostic.hpp"
#include "venom/generated/compiler/typescript_compiler_asset.hpp"
#include "venom/generated/compiler/typescript_bridge_bytecode.hpp"
#include "venom/package/hash.hpp"

#ifdef VENOM_ENABLE_FULL_QUICKJS
extern "C" {
#include "quickjs.h"
}
#endif

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace venom::compiler::typescript {
namespace {

const char* kBridgeSource = R"VENOM_TS_BRIDGE(
(function (global) {
  "use strict";

  function normalizeDiagnostic(diagnostic) {
    var line = 0;
    var column = 0;
    if (diagnostic.file && typeof diagnostic.start === "number") {
      var position = diagnostic.file.getLineAndCharacterOfPosition(diagnostic.start);
      line = position.line + 1;
      column = position.character + 1;
    }
    return {
      code: diagnostic.code >>> 0,
      category: diagnostic.category >>> 0,
      line: line,
      column: column,
      message: ts.flattenDiagnosticMessageText(diagnostic.messageText, "\n")
    };
  }

  global.__venomTranspileTypeScript = function (source, filename) {
    var isTsx = /\.tsx$/i.test(filename);
    var compilerOptions = {
      target: ts.ScriptTarget.ES2022,
      module: ts.ModuleKind.ESNext,
      moduleResolution: ts.ModuleResolutionKind.Bundler,
      sourceMap: true,
      inlineSources: true,
      importsNotUsedAsValues: ts.ImportsNotUsedAsValues.Remove,
      preserveValueImports: false,
      useDefineForClassFields: true,
      isolatedModules: true,
      verbatimModuleSyntax: false,
      newLine: ts.NewLineKind.LineFeed
    };
    if (isTsx) compilerOptions.jsx = ts.JsxEmit.Preserve;

    var result = ts.transpileModule(source, {
      fileName: filename,
      reportDiagnostics: true,
      compilerOptions: compilerOptions
    });

    var javascript = result.outputText || "";
    javascript = javascript.replace(/\n?\/\/# sourceMappingURL=[^\s]+\s*$/, "\n");
    return {
      javascript: javascript,
      sourceMap: result.sourceMapText || "",
      version: ts.version || "unknown",
      tsx: isTsx,
      diagnostics: (result.diagnostics || []).map(normalizeDiagnostic)
    };
  };
})(globalThis);
)VENOM_TS_BRIDGE";

#ifdef VENOM_ENABLE_FULL_QUICKJS
class Value {
public:
  Value(JSContext* context, JSValue value) : context_(context), value_(value) {}
  ~Value() { JS_FreeValue(context_, value_); }
  Value(const Value&) = delete;
  Value& operator=(const Value&) = delete;
  Value(Value&& other) noexcept : context_(other.context_), value_(other.value_) { other.value_ = JS_UNDEFINED; }
  JSValue get() const noexcept { return value_; }
private:
  JSContext* context_;
  JSValue value_;
};

std::string to_string(JSContext* context, JSValueConst value, const char* property) {
  const char* text = JS_ToCString(context, value);
  if (!text) raise_error("VENOM-E2000", std::string("TypeScript frontend property is not a string: ") + property);
  std::string result(text);
  JS_FreeCString(context, text);
  return result;
}

std::string string_property(JSContext* context, JSValueConst object, const char* property) {
  Value value(context, JS_GetPropertyStr(context, object, property));
  if (JS_IsException(value.get())) raise_error("VENOM-E2000", std::string("failed to read TypeScript frontend property: ") + property);
  return to_string(context, value.get(), property);
}

std::size_t array_length(JSContext* context, JSValueConst array) {
  Value length(context, JS_GetPropertyStr(context, array, "length"));
  std::uint32_t value = 0;
  if (JS_ToUint32(context, &value, length.get()) < 0) raise_error("VENOM-E2000", "invalid TypeScript diagnostics array length");
  return value;
}

std::uint32_t uint32_property(JSContext* context, JSValueConst object, const char* property) {
  Value value(context, JS_GetPropertyStr(context, object, property));
  std::uint32_t result = 0;
  if (JS_ToUint32(context, &result, value.get()) < 0)
    raise_error("VENOM-E2000", std::string("invalid numeric TypeScript frontend property: ") + property);
  return result;
}

std::vector<TypeScriptDiagnostic> diagnostic_array(JSContext* context, JSValueConst array) {
  const auto length = array_length(context, array);
  std::vector<TypeScriptDiagnostic> result;
  result.reserve(length);
  for (std::uint32_t index = 0; index < length; ++index) {
    Value item(context, JS_GetPropertyUint32(context, array, index));
    if (!JS_IsObject(item.get())) raise_error("VENOM-E2000", "embedded TypeScript frontend returned an invalid diagnostic entry");
    TypeScriptDiagnostic diagnostic;
    diagnostic.code = uint32_property(context, item.get(), "code");
    const auto category = uint32_property(context, item.get(), "category");
    diagnostic.category = category <= 3 ? static_cast<DiagnosticCategory>(category) : DiagnosticCategory::Error;
    diagnostic.line = uint32_property(context, item.get(), "line");
    diagnostic.column = uint32_property(context, item.get(), "column");
    diagnostic.message = string_property(context, item.get(), "message");
    result.push_back(std::move(diagnostic));
  }
  return result;
}
#endif

} // namespace

EmbeddedRuntime::EmbeddedRuntime(EmbeddedRuntimeLimits limits) : limits_(limits) { initialize(); }

EmbeddedRuntime::~EmbeddedRuntime() {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  if (transpile_function_opaque_) {
    auto* value = static_cast<JSValue*>(transpile_function_opaque_);
    JS_FreeValue(context_, *value);
    delete value;
    transpile_function_opaque_ = nullptr;
  }
  if (context_) JS_FreeContext(context_);
  if (runtime_) JS_FreeRuntime(runtime_);
#endif
}

void EmbeddedRuntime::initialize() {
#ifndef VENOM_ENABLE_FULL_QUICKJS
  raise_error("VENOM-E2000", "embedded TypeScript frontend requires full QuickJS support");
#else
  const auto started = std::chrono::steady_clock::now();
  runtime_ = JS_NewRuntime();
  if (!runtime_) raise_error("VENOM-E2000", "failed to create embedded TypeScript QuickJS runtime");
  JS_SetMemoryLimit(runtime_, limits_.heap_limit_bytes);
  JS_SetMaxStackSize(runtime_, limits_.stack_limit_bytes);
  context_ = JS_NewContext(runtime_);
  if (!context_) raise_error("VENOM-E2000", "failed to create embedded TypeScript QuickJS context");

  initialize_from_source();
  const char* bridge = std::getenv("VENOM_TYPESCRIPT_BRIDGE");
  const std::string bridge_mode = bridge && *bridge ? bridge : "bytecode";
  if (bridge_mode == "source") {
    evaluate_script(kBridgeSource, "<venom-typescript-bridge>");
    bootstrap_mode_ = "verified-source+source-bridge";
  } else if (bridge_mode == "bytecode") {
    evaluate_bytecode();
    bootstrap_mode_ = "verified-source+pinned-bridge-bytecode";
  } else {
    raise_error("VENOM-E2000", "unsupported VENOM_TYPESCRIPT_BRIDGE value: " + bridge_mode +
                             " (expected bytecode or source)");
  }

  Value global(context_, JS_GetGlobalObject(context_));
  Value function(context_, JS_GetPropertyStr(context_, global.get(), "__venomTranspileTypeScript"));
  if (!JS_IsFunction(context_, function.get())) raise_error("VENOM-E2000", "embedded TypeScript bridge did not expose __venomTranspileTypeScript");
  transpile_function_opaque_ = new JSValue(JS_DupValue(context_, function.get()));

  Value ts(context_, JS_GetPropertyStr(context_, global.get(), "ts"));
  compiler_version_ = string_property(context_, ts.get(), "version");
  const auto expected_version = venom::generated::typescript::compiler_version();
  if (compiler_version_.rfind(expected_version, 0) != 0)
    raise_error("VENOM-E2000", "embedded TypeScript compiler version mismatch: expected " +
                             std::string(expected_version) + ", loaded " + compiler_version_);
  initialization_milliseconds_ = std::chrono::duration<double, std::milli>(
      std::chrono::steady_clock::now() - started).count();
#endif
}

void EmbeddedRuntime::initialize_from_source() {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  auto compiler_source = venom::generated::typescript::compiler_source();
  if (compiler_source.size() != venom::generated::typescript::compiler_source_size())
    raise_error("VENOM-E2000", "embedded TypeScript compiler asset size mismatch");
  const auto compiler_sha256 = venom::package::sha256_hex(
      reinterpret_cast<const unsigned char*>(compiler_source.data()), compiler_source.size());
  if (compiler_sha256 != venom::generated::typescript::compiler_source_sha256())
    raise_error("VENOM-E2000", "embedded TypeScript compiler asset integrity failure");
  evaluate_script(compiler_source, "<venom-typescript-compiler>");
  compiler_source.clear();
  compiler_source.shrink_to_fit();
#endif
}

void EmbeddedRuntime::evaluate_bytecode() {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  constexpr std::uint32_t kQuickJsBytecodeAbi = 27u;
  const std::string quickjs_version = std::to_string(QJS_VERSION_MAJOR) + "." +
                                      std::to_string(QJS_VERSION_MINOR) + "." +
                                      std::to_string(QJS_VERSION_PATCH);
  if (venom::generated::typescript::bridge_bytecode_quickjs_version() != quickjs_version)
    raise_error("VENOM-E2000", "embedded TypeScript bridge bytecode QuickJS version mismatch");
  if (venom::generated::typescript::bridge_bytecode_abi() != kQuickJsBytecodeAbi)
    raise_error("VENOM-E2000", "embedded TypeScript bridge bytecode ABI mismatch");
  const auto* bytes = venom::generated::typescript::bridge_bytecode_data();
  const auto size = venom::generated::typescript::bridge_bytecode_size();
  if (!bytes || size == 0) raise_error("VENOM-E2000", "embedded TypeScript bridge bytecode is empty");
  const auto digest = venom::package::sha256_hex(bytes, size);
  if (digest != venom::generated::typescript::bridge_bytecode_sha256())
    raise_error("VENOM-E2000", "embedded TypeScript bridge bytecode integrity failure");
  Value object(context_, JS_ReadObject(context_, bytes, size, JS_READ_OBJ_BYTECODE));
  if (JS_IsException(object.get())) raise_error("VENOM-E2000", exception_message());
  Value evaluated(context_, JS_EvalFunction(context_, JS_DupValue(context_, object.get())));
  if (JS_IsException(evaluated.get())) raise_error("VENOM-E2000", exception_message());
#endif
}

void EmbeddedRuntime::evaluate_script(const std::string& source, const char* filename) {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  Value value(context_, JS_Eval(context_, source.data(), source.size(), filename, JS_EVAL_TYPE_GLOBAL));
  if (JS_IsException(value.get())) raise_error("VENOM-E2000", exception_message());
#else
  (void)source; (void)filename;
#endif
}

std::string EmbeddedRuntime::exception_message() const {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  JSValue exception = JS_GetException(context_);
  const char* text = JS_ToCString(context_, exception);
  std::string message = text ? text : "embedded TypeScript QuickJS exception";
  if (text) JS_FreeCString(context_, text);
  JSValue stack = JS_GetPropertyStr(context_, exception, "stack");
  if (!JS_IsUndefined(stack)) {
    const char* stack_text = JS_ToCString(context_, stack);
    if (stack_text && message.find(stack_text) == std::string::npos) message += "\n" + std::string(stack_text);
    if (stack_text) JS_FreeCString(context_, stack_text);
  }
  JS_FreeValue(context_, stack);
  JS_FreeValue(context_, exception);
  return message;
#else
  return "embedded TypeScript frontend unavailable";
#endif
}

TranspileResult EmbeddedRuntime::transpile(const std::string& source, const std::string& filename) {
#ifndef VENOM_ENABLE_FULL_QUICKJS
  (void)source; (void)filename;
  raise_error("VENOM-E2000", "embedded TypeScript frontend requires full QuickJS support");
#else
  if (source.size() > limits_.source_limit_bytes) raise_error("VENOM-E2000", "TypeScript source exceeds embedded frontend input limit");
  auto* function = static_cast<JSValue*>(transpile_function_opaque_);
  JSValue arguments[2] = {
    JS_NewStringLen(context_, source.data(), source.size()),
    JS_NewStringLen(context_, filename.data(), filename.size())
  };
  Value result(context_, JS_Call(context_, *function, JS_UNDEFINED, 2, arguments));
  JS_FreeValue(context_, arguments[0]);
  JS_FreeValue(context_, arguments[1]);
  if (JS_IsException(result.get())) raise_error("VENOM-E2000", exception_message());
  if (!JS_IsObject(result.get())) raise_error("VENOM-E2000", "embedded TypeScript frontend returned a non-object result");

  TranspileResult output;
  output.typescript = true;
  output.javascript = string_property(context_, result.get(), "javascript");
  output.source_map = string_property(context_, result.get(), "sourceMap");
  output.frontend = "embedded-quickjs-typescript";
  output.frontend_version = string_property(context_, result.get(), "version");

  {
    Value tsx(context_, JS_GetPropertyStr(context_, result.get(), "tsx"));
    output.tsx = JS_ToBool(context_, tsx.get()) != 0;
  }
  {
    Value diagnostics(context_, JS_GetPropertyStr(context_, result.get(), "diagnostics"));
    if (!JS_IsArray(diagnostics.get())) raise_error("VENOM-E2000", "embedded TypeScript frontend returned invalid diagnostics");
    output.diagnostics = diagnostic_array(context_, diagnostics.get());
    output.diagnostic_count = output.diagnostics.size();
  }

  ++call_count_;
  if (limits_.garbage_collect_interval && call_count_ % limits_.garbage_collect_interval == 0) JS_RunGC(runtime_);
  return output;
#endif
}

EmbeddedFrontendService& EmbeddedFrontendService::instance() {
  static EmbeddedFrontendService service;
  return service;
}

TranspileResult EmbeddedFrontendService::transpile(const std::string& source, const std::string& filename) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!runtime_) runtime_ = std::make_unique<EmbeddedRuntime>();
  return runtime_->transpile(source, filename);
}

std::string EmbeddedFrontendService::compiler_version() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!runtime_) runtime_ = std::make_unique<EmbeddedRuntime>();
  return runtime_->compiler_version();
}

} // namespace venom::compiler::typescript
