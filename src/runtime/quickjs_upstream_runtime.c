#include "quickjs_upstream_runtime.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define VENOM_QJS_RUNTIME_ABI_VERSION 12u
#define VENOM_QJS_RUNTIME_PACKAGE_VERSION 83u

static uint32_t venom_fnv1a32(const char* data) {
  uint32_t h = 2166136261u;
  if (!data) return h;
  while (*data) {
    h ^= (unsigned char)(*data++);
    h *= 16777619u;
  }
  return h;
}

static void venom_qjs_report_init(VenomQuickJsNativeReport* report) {
  if (!report) return;
  memset(report, 0, sizeof(*report));
  report->runtime_abi = VENOM_QJS_RUNTIME_ABI_VERSION;
  report->package_version = VENOM_QJS_RUNTIME_PACKAGE_VERSION;
}

uint32_t venom_qjs_native_available(void) {
#ifdef VENOM_ENABLE_FULL_QUICKJS
  return 1u;
#else
  return 0u;
#endif
}

#ifndef VENOM_ENABLE_FULL_QUICKJS
int venom_qjs_native_run_parity_smoke(VenomQuickJsNativeReport* report) {
  venom_qjs_report_init(report);
  if (report) {
    report->available = 0u;
    report->checks_failed = 1u;
    snprintf(report->exception, sizeof(report->exception), "%s", "VENOM_ENABLE_FULL_QUICKJS is OFF; native upstream QuickJS is not linked");
  }
  return 3;
}
#else

#include "quickjs.h"

static void set_exception(VenomQuickJsNativeReport* report, JSContext* ctx, const char* label) {
  if (!report) return;
  if (!ctx) {
    snprintf(report->exception, sizeof(report->exception), "%s", label ? label : "quickjs error");
    return;
  }
  JSValue ex = JS_GetException(ctx);
  const char* msg = JS_ToCString(ctx, ex);
  if (msg) {
    snprintf(report->exception, sizeof(report->exception), "%s: %s", label ? label : "quickjs exception", msg);
    JS_FreeCString(ctx, msg);
  } else {
    snprintf(report->exception, sizeof(report->exception), "%s", label ? label : "quickjs exception");
  }
  JS_FreeValue(ctx, ex);
}

static int eval_to_cstring(JSRuntime* rt,
                           JSContext* ctx,
                           const char* source,
                           const char* expected,
                           VenomQuickJsNativeReport* report) {
  JSValue value = JS_Eval(ctx, source, strlen(source), "<venom-native-parity>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) {
    set_exception(report, ctx, source);
    return 0;
  }

  JSContext* job_ctx = NULL;
  for (;;) {
    const int job_rc = JS_ExecutePendingJob(rt, &job_ctx);
    if (job_rc < 0) {
      set_exception(report, job_ctx ? job_ctx : ctx, "pending job failed");
      JS_FreeValue(ctx, value);
      return 0;
    }
    if (job_rc == 0) break;
  }

  const char* text = JS_ToCString(ctx, value);
  if (!text) {
    set_exception(report, ctx, "failed to stringify result");
    JS_FreeValue(ctx, value);
    return 0;
  }

  const int ok = strcmp(text, expected) == 0;
  if (!ok && report) {
    snprintf(report->exception, sizeof(report->exception), "expected '%s' but got '%s' from %s", expected, text, source);
  }
  JS_FreeCString(ctx, text);
  JS_FreeValue(ctx, value);
  return ok;
}

int venom_qjs_native_run_parity_smoke(VenomQuickJsNativeReport* report) {
  venom_qjs_report_init(report);
  if (!report) return 2;
  report->available = 1u;

  JSRuntime* rt = JS_NewRuntime();
  if (!rt) {
    snprintf(report->exception, sizeof(report->exception), "%s", "JS_NewRuntime failed");
    report->checks_failed = 1u;
    return 1;
  }
  JS_SetMemoryLimit(rt, 64u * 1024u * 1024u);
  JS_SetMaxStackSize(rt, 1024u * 1024u);

  JSContext* ctx = JS_NewContext(rt);
  if (!ctx) {
    snprintf(report->exception, sizeof(report->exception), "%s", "JS_NewContext failed");
    JS_FreeRuntime(rt);
    report->checks_failed = 1u;
    return 1;
  }

  struct Check { const char* source; const char* expected; } checks[] = {
    {"1 + 2 + 39", "42"},
    {"(() => { let x = 7; return (() => x + 5)(); })()", "12"},
    {"JSON.stringify({a:1,b:[2,3]})", "{\"a\":1,\"b\":[2,3]}"},
    {"try { throw new Error('boom') } catch (e) { e.message }", "boom"},
    {"var __venom_async_value = 0; Promise.resolve(5).then(v => { __venom_async_value = v + 1; }); 'queued'", "queued"},
    {"__venom_async_value", "6"},
  };

  const size_t count = sizeof(checks) / sizeof(checks[0]);
  for (size_t i = 0; i < count; ++i) {
    if (eval_to_cstring(rt, ctx, checks[i].source, checks[i].expected, report)) {
      ++report->checks_passed;
    } else {
      ++report->checks_failed;
      break;
    }
  }

  snprintf(report->result_json, sizeof(report->result_json),
           "{\"engine\":\"upstream-quickjs-native\",\"available\":true,\"abi\":%u,\"packageVersion\":%u,\"checksPassed\":%u,\"checksFailed\":%u}",
           report->runtime_abi, report->package_version, report->checks_passed, report->checks_failed);
  report->result_hash32 = venom_fnv1a32(report->result_json);

  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
  return report->checks_failed == 0u ? 0 : 1;
}
#endif
