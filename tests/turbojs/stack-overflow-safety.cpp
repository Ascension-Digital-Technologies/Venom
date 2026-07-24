#include "turbojs/abi.hpp"
#include "turbojs.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {
[[noreturn]] void fail(const std::string& message) {
  std::cerr << "TurboJS stack-overflow safety test failed: " << message << '\n';
  std::exit(1);
}

std::string take_exception(JSContext* context) {
  JSValue exception = JS_GetException(context);
  const char* text = JS_ToCString(context, exception);
  std::string result = text ? text : "<unprintable exception>";
  if (text) JS_FreeCString(context, text);
  JS_FreeValue(context, exception);
  return result;
}
} // namespace

int main() {
  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) fail("could not create runtime");
  JS_SetMaxStackSize(runtime, venom::turbojs::kDefaultStackLimitBytes);

  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    fail("could not create context");
  }

  // This intentionally matches the recursive stack-depth fingerprint used by
  // fpCollect. The overflow must become a catchable JavaScript RangeError;
  // escaping as a native/WASM stack trap would crash the entire Venom boot.
  const char* source = R"JS(
    (() => {
      let depth = 0;
      let errorMessage = '';
      let errorName = '';
      let errorStacklength = 0;
      function iWillBetrayYouWithMyLongName() {
        try {
          depth++;
          iWillBetrayYouWithMyLongName();
        } catch (e) {
          errorMessage = String(e.message || '');
          errorName = String(e.name || '');
          errorStacklength = String(e.stack || '').length;
        }
      }
      iWillBetrayYouWithMyLongName();
      return JSON.stringify({ depth, errorMessage, errorName, errorStacklength });
    })()
  )JS";

  JSValue value = JS_Eval(context, source, std::strlen(source), "stack-overflow-safety.js", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    const std::string exception = take_exception(context);
    JS_FreeValue(context, value);
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    fail("overflow escaped the JavaScript catch boundary: " + exception);
  }

  const char* text = JS_ToCString(context, value);
  const std::string result = text ? text : "";
  if (text) JS_FreeCString(context, text);
  JS_FreeValue(context, value);
  JS_FreeContext(context);
  JS_FreeRuntime(runtime);

  if (result.find("\"errorName\":\"RangeError\"") == std::string::npos) {
    fail("expected a caught RangeError, got: " + result);
  }
  if (result.find("\"depth\":0") != std::string::npos) {
    fail("recursion did not execute: " + result);
  }

  std::cout << "recursive fingerprint overflow is a catchable TurboJS RangeError: PASS\n";
  return 0;
}
