#include "compiler/pipeline/native_js_hardener.hpp"

#include "quickjs.h"

#include <stdexcept>
#include <string>

namespace venom::compiler::native_js_hardener {
namespace {

#include "compiler/pipeline/embedded_js_hardener_bundles.inc"

std::string value_text(JSContext* ctx, JSValueConst value) {
  const char* text = JS_ToCString(ctx, value);
  if (!text) return "<unprintable QuickJS value>";
  std::string result(text);
  JS_FreeCString(ctx, text);
  return result;
}

std::string exception_text(JSContext* ctx) {
  JSValue exception = JS_GetException(ctx);
  std::string result = value_text(ctx, exception);
  JSValue stack = JS_GetPropertyStr(ctx, exception, "stack");
  if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
    const auto stack_text = value_text(ctx, stack);
    if (!stack_text.empty() && stack_text != result) result += "\n" + stack_text;
  }
  JS_FreeValue(ctx, stack);
  JS_FreeValue(ctx, exception);
  return result;
}

void eval_checked(JSContext* ctx, const char* source, std::size_t size, const char* filename) {
  JSValue value = JS_Eval(ctx, source, size, filename, JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    const auto error = exception_text(ctx);
    JS_FreeValue(ctx, value);
    throw std::runtime_error(std::string("embedded JS hardener failed while loading ") + filename + ": " + error);
  }
  JS_FreeValue(ctx, value);
}

void set_global_string(JSContext* ctx, const char* name, const std::string& value) {
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, name, JS_NewStringLen(ctx, value.data(), value.size()));
  JS_FreeValue(ctx, global);
}

void set_global_u32(JSContext* ctx, const char* name, std::uint32_t value) {
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, name, JS_NewUint32(ctx, value));
  JS_FreeValue(ctx, global);
}

const char kBootstrap[] = R"JS(
globalThis.self = globalThis;
globalThis.window = globalThis;
globalThis.__venom_hardener_done = false;
globalThis.__venom_hardener_output = undefined;
globalThis.__venom_hardener_error = undefined;
)JS";

const char kRunHardener[] = R"JS(
(async () => {
  try {
    const kind = globalThis.__venom_hardener_kind;
    const source = globalThis.__venom_hardener_source;
    const seed = globalThis.__venom_hardener_seed >>> 0;
    const moduleKind = kind === 'loader' || kind === 'engine' || kind === 'runtime';
    const bindingMatch = kind === 'loader' ? source.match(/bindingToken\s*:\s*['\"]([^'\"]+)['\"]/) : null;
    const minified = await globalThis.Terser.minify(source, {
      module: moduleKind,
      compress: {
        passes: 3,
        booleans_as_integers: true,
        drop_console: false,
        keep_fargs: false,
        unsafe_arrows: true,
        unsafe_methods: true,
        unsafe_proto: false,
        unsafe_regexp: false
      },
      mangle: {
        toplevel: true,
        keep_classnames: false,
        keep_fnames: false
      },
      format: {
        comments: false,
        ascii_only: true,
        semicolons: true,
        wrap_iife: true
      }
    });
    if (!minified.code) throw new Error('terser produced no output');
    const hardened = globalThis.JavaScriptObfuscator.obfuscate(minified.code, {
      target: 'browser-no-eval', compact: true, seed,
      identifierNamesGenerator: 'hexadecimal', identifiersPrefix: '_v',
      renameGlobals: false, renameProperties: false,
      stringArray: true, stringArrayEncoding: ['rc4'],
      stringArrayThreshold: kind === 'runtime' ? 0.42 : 0.58,
      stringArrayCallsTransform: kind === 'loader',
      stringArrayCallsTransformThreshold: 0.35,
      stringArrayIndexesType: ['hexadecimal-number'],
      stringArrayRotate: true, stringArrayShuffle: true,
      stringArrayWrappersCount: 1, stringArrayWrappersChainedCalls: true,
      stringArrayWrappersParametersMaxCount: 4, stringArrayWrappersType: 'function',
      splitStrings: kind === 'loader', splitStringsChunkLength: 12,
      numbersToExpressions: kind !== 'worker', simplify: kind !== 'worker',
      transformObjectKeys: kind !== 'worker', unicodeEscapeSequence: false,
      controlFlowFlattening: kind === 'loader',
      controlFlowFlatteningThreshold: kind === 'loader' ? 0.22 : 0.12,
      deadCodeInjection: kind === 'loader',
      deadCodeInjectionThreshold: kind === 'loader' ? 0.035 : 0.02,
      selfDefending: kind !== 'worker', debugProtection: false,
      disableConsoleOutput: false, sourceMap: false
    }).getObfuscatedCode();
    const bindingMarker = bindingMatch ? `;void\"vbind:${bindingMatch[1]}\";` : '';
    globalThis.__venom_hardener_output = hardened + bindingMarker;
  } catch (error) {
    globalThis.__venom_hardener_error = String(error && error.stack ? error.stack : error);
  } finally {
    globalThis.__venom_hardener_done = true;
  }
})();
)JS";

}  // namespace

std::string harden(const std::string& kind, const std::string& source, std::uint32_t seed) {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) throw std::runtime_error("embedded JS hardener could not create QuickJS runtime");
  JS_SetMemoryLimit(runtime, 768ull * 1024ull * 1024ull);
  JS_SetMaxStackSize(runtime, 8ull * 1024ull * 1024ull);
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    throw std::runtime_error("embedded JS hardener could not create QuickJS context");
  }

  try {
    eval_checked(context, kBootstrap, sizeof(kBootstrap) - 1, "<venom-hardener-bootstrap>");
    eval_checked(context, kEmbeddedTerserBundle, sizeof(kEmbeddedTerserBundle) - 1, "<embedded-terser-5.49.0>");
    eval_checked(context, kEmbeddedJavascriptObfuscatorBundle, sizeof(kEmbeddedJavascriptObfuscatorBundle) - 1,
                 "<embedded-javascript-obfuscator-5.4.7>");
    set_global_string(context, "__venom_hardener_kind", kind);
    set_global_string(context, "__venom_hardener_source", source);
    set_global_u32(context, "__venom_hardener_seed", seed);
    eval_checked(context, kRunHardener, sizeof(kRunHardener) - 1, "<venom-hardener-run>");

    for (;;) {
      JSValue global = JS_GetGlobalObject(context);
      JSValue done_value = JS_GetPropertyStr(context, global, "__venom_hardener_done");
      const int done = JS_ToBool(context, done_value);
      JS_FreeValue(context, done_value);
      JS_FreeValue(context, global);
      if (done > 0) break;
      JSContext* job_context = nullptr;
      const int status = JS_ExecutePendingJob(runtime, &job_context);
      if (status < 0) throw std::runtime_error("embedded JS hardener promise job failed: " + exception_text(job_context ? job_context : context));
      if (status == 0) throw std::runtime_error("embedded JS hardener stalled before producing output");
    }

    JSValue global = JS_GetGlobalObject(context);
    JSValue error = JS_GetPropertyStr(context, global, "__venom_hardener_error");
    if (!JS_IsUndefined(error) && !JS_IsNull(error)) {
      const auto message = value_text(context, error);
      JS_FreeValue(context, error);
      JS_FreeValue(context, global);
      throw std::runtime_error("embedded JS hardener failed: " + message);
    }
    JS_FreeValue(context, error);
    JSValue output = JS_GetPropertyStr(context, global, "__venom_hardener_output");
    if (!JS_IsString(output)) {
      JS_FreeValue(context, output);
      JS_FreeValue(context, global);
      throw std::runtime_error("embedded JS hardener produced no string output");
    }
    const auto result = value_text(context, output);
    JS_FreeValue(context, output);
    JS_FreeValue(context, global);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    return result;
  } catch (...) {
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    throw;
  }
}

const char* terser_version() { return "5.49.0"; }
const char* obfuscator_version() { return "5.4.7"; }

}  // namespace venom::compiler::native_js_hardener
