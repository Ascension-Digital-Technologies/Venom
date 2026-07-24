#include "turbojs.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
[[noreturn]] void fail(JSContext* context, const std::string& message) {
  std::cerr << "TurboJS numeric Map/Set key regression failed: " << message;
  if (context) {
    JSValue exception = JS_GetException(context);
    const char* text = JS_ToCString(context, exception);
    if (text) {
      std::cerr << ": " << text;
      JS_FreeCString(context, text);
    }
    JS_FreeValue(context, exception);
  }
  std::cerr << '\n';
  std::exit(1);
}

void evaluate_assertions(JSContext* context) {
  static constexpr const char kSource[] = R"JS(
(() => {
  const map = new Map();
  for (let i = -4096; i <= 4096; i++) map.set(i, (i * 17) | 0);
  for (let i = -4096; i <= 4096; i++) {
    if (map.get(i) !== ((i * 17) | 0)) throw new Error(`integer key mismatch: ${i}`);
  }

  map.set(-0, "zero");
  if (map.get(+0) !== "zero" || map.get(0.0) !== "zero") {
    throw new Error("signed-zero SameValueZero mismatch");
  }

  map.set(NaN, "nan");
  if (map.get(Number("not-a-number")) !== "nan") {
    throw new Error("NaN SameValueZero mismatch");
  }

  map.set(42, "integer");
  if (map.get(42.0) !== "integer") throw new Error("integral double mismatch");

  map.set(42.5, "fraction");
  if (map.get(42.5) !== "fraction" || map.has(42) !== true) {
    throw new Error("fractional numeric key mismatch");
  }

  const set = new Set();
  for (let i = 0; i < 8192; i++) set.add(i);
  for (let i = 0; i < 8192; i++) {
    if (!set.has(i)) throw new Error(`integer Set key mismatch: ${i}`);
  }
  set.add(-0);
  if (!set.has(+0)) throw new Error("Set signed-zero mismatch");
  set.add(NaN);
  if (!set.has(Number("nan"))) throw new Error("Set NaN mismatch");

  return true;
})()
)JS";

  JSValue result = JS_Eval(context, kSource, sizeof(kSource) - 1,
                           "map-set-numeric-key-regression.js", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(result)) fail(context, "JavaScript assertions");
  if (JS_ToBool(context, result) != 1) {
    JS_FreeValue(context, result);
    fail(context, "unexpected false result");
  }
  JS_FreeValue(context, result);
}
}  // namespace

int main() {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) fail(nullptr, "runtime allocation");
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    fail(nullptr, "context allocation");
  }

  evaluate_assertions(context);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  return 0;
}
