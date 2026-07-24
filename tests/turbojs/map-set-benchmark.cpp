#include "turbojs.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;

[[noreturn]] void fail(JSContext* context, const std::string& message) {
  std::cerr << "TurboJS collection benchmark failed: " << message;
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

JSValue evaluate(JSContext* context, const std::string& source, const char* filename) {
  JSValue value = JS_Eval(context, source.data(), source.size(), filename, JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(value)) fail(context, filename);
  return value;
}

double run_case(JSContext* context, const char* function_name, int iterations, int seed, unsigned& checksum) {
  const std::string expression = std::string(function_name) + "(" + std::to_string(iterations) + "," + std::to_string(seed) + ")";
  const auto started = Clock::now();
  JSValue value = evaluate(context, expression, "collection-benchmark-run.js");
  const auto ended = Clock::now();
  uint32_t result = 0;
  if (JS_ToUint32(context, &result, value) < 0) {
    JS_FreeValue(context, value);
    fail(context, "result conversion");
  }
  checksum ^= result;
  JS_FreeValue(context, value);
  return std::chrono::duration<double, std::milli>(ended - started).count();
}
}

int main(int argc, char** argv) {
  const int iterations = argc > 1 ? std::max(1, std::atoi(argv[1])) : 500000;
  const int rounds = argc > 2 ? std::max(1, std::atoi(argv[2])) : 5;

  JSRuntime* runtime = JS_NewRuntime();
  if (!runtime) fail(nullptr, "runtime allocation");
  JSContext* context = JS_NewContext(runtime);
  if (!context) {
    JS_FreeRuntime(runtime);
    fail(nullptr, "context allocation");
  }

  const char* source = R"JS(
function mapUpdate(iterations, seed) {
  const map = new Map();
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) {
    const key = i & 1023;
    const value = ((map.get(key) || seed) + i) >>> 0;
    map.set(key, value);
    checksum ^= value;
  }
  return (checksum + map.size) >>> 0;
}
function mapLookup(iterations, seed) {
  const map = new Map();
  for (let i = 0; i < 2048; i++) map.set(i, (seed + i * 17) >>> 0);
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) checksum ^= map.get(i & 2047) || 0;
  return checksum >>> 0;
}
function setChurn(iterations, seed) {
  const set = new Set();
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) {
    const key = (i + checksum) & 2047;
    set.add(key);
    if ((i & 255) === 0) set.delete((key + 17) & 2047);
    checksum ^= set.has(key) ? key : 0;
  }
  return (checksum + set.size) >>> 0;
}
function stringLookup(iterations, seed) {
  const map = new Map();
  const keys = [];
  for (let i = 0; i < 2048; i++) { const key = "key-" + i; keys.push(key); map.set(key, (seed + i * 19) >>> 0); }
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) checksum ^= map.get(keys[i & 2047]) || 0;
  return checksum >>> 0;
}
function objectLookup(iterations, seed) {
  const map = new Map();
  const keys = [];
  for (let i = 0; i < 2048; i++) { const key = {i}; keys.push(key); map.set(key, (seed + i * 23) >>> 0); }
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) checksum ^= map.get(keys[i & 2047]) || 0;
  return checksum >>> 0;
}
function mixedCollections(iterations, seed) {
  const map = new Map();
  const set = new Set();
  let checksum = seed | 0;
  for (let i = 0; i < iterations; i++) {
    const key = i & 1023;
    const value = ((map.get(key) || seed) + i) >>> 0;
    map.set(key, value);
    set.add((key + checksum) & 2047);
    if ((i & 255) === 0) set.delete((key + 17) & 2047);
    checksum ^= value;
  }
  return (checksum + map.size + set.size) >>> 0;
}
)JS";
  JSValue setup = evaluate(context, source, "collection-benchmark-setup.js");
  JS_FreeValue(context, setup);

  struct Case { const char* name; const char* function; };
  const Case cases[] = {
      {"map_update", "mapUpdate"},
      {"map_lookup", "mapLookup"},
      {"set_churn", "setChurn"},
      {"string_lookup", "stringLookup"},
      {"object_lookup", "objectLookup"},
      {"mixed", "mixedCollections"},
  };

  std::cout << std::fixed << std::setprecision(3);
  std::cout << "{\"iterations\":" << iterations << ",\"rounds\":" << rounds << ",\"cases\":[";
  bool first = true;
  for (const auto& item : cases) {
    unsigned checksum = 0;
    run_case(context, item.function, std::max(1000, iterations / 20), 1337, checksum);
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(rounds));
    for (int round = 0; round < rounds; ++round) samples.push_back(run_case(context, item.function, iterations, 1337 + round, checksum));
    double total = 0.0;
    double best = samples.front();
    for (double sample : samples) { total += sample; best = std::min(best, sample); }
    const double mean = total / static_cast<double>(samples.size());
    const double throughput = mean > 0.0 ? iterations / (mean / 1000.0) : 0.0;
    if (!first) std::cout << ',';
    first = false;
    std::cout << "{\"name\":\"" << item.name << "\",\"mean_ms\":" << mean
              << ",\"best_ms\":" << best << ",\"throughput_ops_s\":" << throughput
              << ",\"checksum\":" << checksum << '}';
  }
  std::cout << "]}\n";

  JS_FreeContext(context);
  JS_FreeRuntime(runtime);
  return 0;
}
