#include "venom/base/error.hpp"
#include "venom/internal/pipeline/native_js_hardener.hpp"
#include "venom/frontends/javascript/embedded_bundles.hpp"

#include "quickjs.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace venom::compiler::native_js_hardener {
namespace {


using Clock = std::chrono::steady_clock;

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
    raise_error("VENOM-E5000", std::string("embedded JS hardener failed while loading ") + filename + ": " + error);
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

void clear_global(JSContext* ctx, const char* name) {
  JSValue global = JS_GetGlobalObject(ctx);
  JS_SetPropertyStr(ctx, global, name, JS_UNDEFINED);
  JS_FreeValue(ctx, global);
}

const char kBootstrap[] = R"JS(
globalThis.self = globalThis;
globalThis.window = globalThis;
globalThis.__venom_hardener_done = false;
globalThis.__venom_hardener_output = undefined;
globalThis.__venom_hardener_error = undefined;
)JS";

const char kResetInvocation[] = R"JS(
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
    const moduleKind = kind === 'loader' || kind === 'engine' || kind === 'runtime' || kind === 'protected-module' || kind === 'extension-module';
    const protectedKind = kind === 'protected-script' || kind === 'protected-module';
    const extensionKind = kind.startsWith('extension-');
    const bytecodeKind = kind.startsWith('protected-');
    const largeAsset = source.length >= 32768;
    const hugeAsset = source.length >= 196608;
    const bindingMatch = kind === 'loader' ? source.match(/bindingToken\s*:\s*['\"]([^'\"]+)['\"]/) : null;
    const minified = await globalThis.Terser.minify(source, {
      module: moduleKind,
      compress: {
        passes: hugeAsset ? 1 : (largeAsset ? 2 : 3),
        booleans_as_integers: true,
        drop_console: false,
        keep_fargs: false,
        unsafe_arrows: true,
        unsafe_methods: true,
        unsafe_proto: false,
        unsafe_regexp: false
      },
      mangle: { toplevel: true, keep_classnames: false, keep_fnames: false },
      format: { comments: false, ascii_only: true, semicolons: true, wrap_iife: true }
    });
    if (!minified.code) throw new Error('terser produced no output');
    let hardened = minified.code;
    if (!extensionKind) hardened = globalThis.JavaScriptObfuscator.obfuscate(minified.code, {
      target: 'browser-no-eval', compact: true, seed,
      identifierNamesGenerator: 'hexadecimal', identifiersPrefix: '_v' + seed.toString(36) + '_',
      renameGlobals: false, renameProperties: false,
      stringArray: !bytecodeKind && !extensionKind, stringArrayEncoding: ['rc4'],
      stringArrayThreshold: protectedKind ? 0.86 : (kind === 'runtime' ? 0.42 : 0.58),
      stringArrayCallsTransform: !bytecodeKind && !extensionKind && (protectedKind || (kind === 'loader' && !hugeAsset)),
      stringArrayCallsTransformThreshold: protectedKind ? 0.72 : 0.35,
      stringArrayIndexesType: ['hexadecimal-number'],
      stringArrayRotate: !bytecodeKind && !extensionKind, stringArrayShuffle: !bytecodeKind && !extensionKind,
      stringArrayWrappersCount: 1, stringArrayWrappersChainedCalls: true,
      stringArrayWrappersParametersMaxCount: 4, stringArrayWrappersType: 'function',
      splitStrings: !bytecodeKind && !extensionKind && (protectedKind || (kind === 'loader' && !hugeAsset)), splitStringsChunkLength: protectedKind ? 7 : 12,
      numbersToExpressions: !bytecodeKind && kind !== 'worker' && !extensionKind, simplify: kind !== 'worker' && !extensionKind,
      transformObjectKeys: kind !== 'worker' && !extensionKind, unicodeEscapeSequence: false,
      controlFlowFlattening: !bytecodeKind && !extensionKind && (protectedKind || ((kind === 'loader' || kind === 'runtime') && !hugeAsset)),
      controlFlowFlatteningThreshold: protectedKind ? 0.34 : (largeAsset ? 0.06 : (kind === 'loader' ? 0.22 : 0.12)),
      deadCodeInjection: !bytecodeKind && !extensionKind && (protectedKind || (kind === 'loader' && !largeAsset)),
      deadCodeInjectionThreshold: protectedKind ? 0.045 : (kind === 'loader' ? 0.035 : 0.02),
      selfDefending: !bytecodeKind && kind !== 'worker' && !extensionKind, debugProtection: false,
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

class PersistentHardener {
 public:
  PersistentHardener() { initialize(); }
  ~PersistentHardener() { destroy(); }

  std::string run(const std::string& kind, const std::string& source, std::uint32_t seed) {
    eval_checked(context_, kResetInvocation, sizeof(kResetInvocation) - 1, "<venom-hardener-reset>");
    set_global_string(context_, "__venom_hardener_kind", kind);
    set_global_string(context_, "__venom_hardener_source", source);
    set_global_u32(context_, "__venom_hardener_seed", seed);
    eval_checked(context_, kRunHardener, sizeof(kRunHardener) - 1, "<venom-hardener-run>");

    for (;;) {
      JSValue global = JS_GetGlobalObject(context_);
      JSValue done_value = JS_GetPropertyStr(context_, global, "__venom_hardener_done");
      const int done = JS_ToBool(context_, done_value);
      JS_FreeValue(context_, done_value);
      JS_FreeValue(context_, global);
      if (done > 0) break;
      JSContext* job_context = nullptr;
      const int status = JS_ExecutePendingJob(runtime_, &job_context);
      if (status < 0) raise_error("VENOM-E5000", "embedded JS hardener promise job failed: " + exception_text(job_context ? job_context : context_));
      if (status == 0) raise_error("VENOM-E5000", "embedded JS hardener stalled before producing output");
    }

    JSValue global = JS_GetGlobalObject(context_);
    JSValue error = JS_GetPropertyStr(context_, global, "__venom_hardener_error");
    if (!JS_IsUndefined(error) && !JS_IsNull(error)) {
      const auto message = value_text(context_, error);
      JS_FreeValue(context_, error);
      JS_FreeValue(context_, global);
      raise_error("VENOM-E5000", "embedded JS hardener failed: " + message);
    }
    JS_FreeValue(context_, error);
    JSValue output = JS_GetPropertyStr(context_, global, "__venom_hardener_output");
    if (!JS_IsString(output)) {
      JS_FreeValue(context_, output);
      JS_FreeValue(context_, global);
      raise_error("VENOM-E5000", "embedded JS hardener produced no string output");
    }
    auto result = value_text(context_, output);
    JS_FreeValue(context_, output);
    JS_FreeValue(context_, global);

    clear_global(context_, "__venom_hardener_kind");
    clear_global(context_, "__venom_hardener_source");
    clear_global(context_, "__venom_hardener_output");
    ++calls_since_gc_;
    if (calls_since_gc_ >= 4) {
      JS_RunGC(runtime_);
      calls_since_gc_ = 0;
    }
    return result;
  }

 private:
  void initialize() {
    runtime_ = JS_NewRuntime();
    if (!runtime_) raise_error("VENOM-E5000", "embedded JS hardener could not create QuickJS runtime");
    JS_SetMemoryLimit(runtime_, 768ull * 1024ull * 1024ull);
    JS_SetMaxStackSize(runtime_, 8ull * 1024ull * 1024ull);
    context_ = JS_NewContext(runtime_);
    if (!context_) {
      JS_FreeRuntime(runtime_);
      runtime_ = nullptr;
      raise_error("VENOM-E5000", "embedded JS hardener could not create QuickJS context");
    }
    eval_checked(context_, kBootstrap, sizeof(kBootstrap) - 1, "<venom-hardener-bootstrap>");
    eval_checked(context_, embedded_js_assets::terser_bundle().data(), embedded_js_assets::terser_bundle().size(), "<embedded-terser-5.49.0>");
    eval_checked(context_, embedded_js_assets::javascript_obfuscator_bundle().data(), embedded_js_assets::javascript_obfuscator_bundle().size(),
                 "<embedded-javascript-obfuscator-5.4.7>");
  }

  void destroy() {
    if (context_) JS_FreeContext(context_);
    if (runtime_) JS_FreeRuntime(runtime_);
    context_ = nullptr;
    runtime_ = nullptr;
  }

  JSRuntime* runtime_ = nullptr;
  JSContext* context_ = nullptr;
  std::size_t calls_since_gc_ = 0;
};

std::mutex g_mutex;
std::unique_ptr<PersistentHardener> g_hardener;
HardenerRuntimeStats g_stats;

}  // namespace

std::string harden(const std::string& kind, const std::string& source, std::uint32_t seed) {
  std::lock_guard<std::mutex> lock(g_mutex);
  const auto total_start = Clock::now();
  if (!g_hardener) {
    const auto init_start = Clock::now();
    g_hardener = std::make_unique<PersistentHardener>();
    g_stats.initialization_ms += std::chrono::duration<double, std::milli>(Clock::now() - init_start).count();
    ++g_stats.initializations;
  }
  try {
    const auto run_start = Clock::now();
    auto result = g_hardener->run(kind, source, seed);
    g_stats.execution_ms += std::chrono::duration<double, std::milli>(Clock::now() - run_start).count();
    g_stats.total_ms += std::chrono::duration<double, std::milli>(Clock::now() - total_start).count();
    ++g_stats.calls;
    g_stats.input_bytes += source.size();
    g_stats.output_bytes += result.size();
    return result;
  } catch (...) {
    // A failed promise job or OOM can leave the realm in an unknown state. Recreate
    // it for the next request rather than carrying damaged compiler state forward.
    g_hardener.reset();
    ++g_stats.resets;
    throw;
  }
}

HardenerRuntimeStats runtime_stats() {
  std::lock_guard<std::mutex> lock(g_mutex);
  return g_stats;
}

void reset_runtime_stats() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_stats = {};
}

void shutdown() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_hardener.reset();
}

const char* terser_version() { return "5.49.0"; }
const char* obfuscator_version() { return "5.4.7"; }

}  // namespace venom::compiler::native_js_hardener
