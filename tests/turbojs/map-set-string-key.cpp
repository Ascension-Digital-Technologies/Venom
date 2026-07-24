#include "turbojs.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
[[noreturn]] void fail(JSContext* context, const char* message) {
  std::cerr << "TurboJS string collection key regression failed: " << message;
  if (context) {
    JSValue exception = JS_GetException(context);
    const char* text = JS_ToCString(context, exception);
    if (text) { std::cerr << ": " << text; JS_FreeCString(context, text); }
    JS_FreeValue(context, exception);
  }
  std::cerr << '\n';
  std::exit(1);
}
}

int main() {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) fail(nullptr, "runtime allocation");
  JSContext* context = JS_NewContext(runtime);
  if (!context) { JS_FreeRuntime(runtime); fail(nullptr, "context allocation"); }

  const char* source = R"JS(
(() => {
  const map = new Map();
  const keys = [];
  for (let i = 0; i < 4096; i++) {
    const key = "dynamic-key-" + i + "-abcdefghijklmnopqrstuvwxyz";
    keys.push(key);
    map.set(key, i);
  }
  for (let i = 0; i < keys.length; i++) {
    if (map.get(keys[i]) !== i) throw new Error("identity lookup mismatch");
    const equivalent = "dynamic-key-" + i + "-abcdefghijklmnopqrstuvwxyz";
    if (map.get(equivalent) !== i) throw new Error("equivalent string mismatch");
  }
  const key = "later-atomized-" + Date.now();
  map.set(key, 77);
  const object = { [key]: true };
  if (!object[key] || map.get(key) !== 77) throw new Error("atomization changed Map identity");
  const set = new Set(keys);
  for (const key of keys) if (!set.has(key)) throw new Error("Set lookup mismatch");
  return true;
})()
)JS";
  JSValue result = JS_Eval(context, source, std::char_traits<char>::length(source),
                           "map-set-string-key.js", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(result)) fail(context, "script execution");
  if (JS_ToBool(context, result) != 1) fail(context, "unexpected result");
  JS_FreeValue(context, result);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  return 0;
}
