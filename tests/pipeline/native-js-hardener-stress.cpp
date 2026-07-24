#include "pipeline/native_js_hardener.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace {
struct Case {
  const char* kind;
  const char* source;
};

constexpr std::array<Case, 6> kCases{{
    {"loader", "const config={bindingToken:'stress-token'};globalThis.__stress=config.bindingToken;"},
    {"runtime", "export function add(a,b){return a+b;}globalThis.__runtimeReady=true;"},
    {"worker", "self.onmessage=(event)=>self.postMessage({ok:true,value:event.data});"},
    {"protected-script", "function checksum(values){let total=0;for(const value of values)total=(total+value)|0;return total;}globalThis.__checksum=checksum([1,2,3,4]);"},
    {"browser-module", "export function mount(){document.body.dataset.ready=String(true);}"},
    {"extension-script", "chrome.tabs.query({active:true,currentWindow:true},tabs=>globalThis.__tab=tabs[0]);"},
}};
}

int main(int argc, char** argv) {
  using namespace venom::compiler::native_js_hardener;
  reset_runtime_stats();
  const bool subprocess_mode = argc >= 2;
  const bool retry_mode = argc >= 3;
  if (subprocess_mode) configure_worker_executable(argv[1]);

  for (std::uint32_t cycle = 0; cycle < 2; ++cycle) {
    for (std::size_t index = 0; index < kCases.size(); ++index) {
      const auto& test = kCases[index];
      const auto output = harden(test.kind, test.source, 0x13579bdu + cycle * 17u + static_cast<std::uint32_t>(index));
      if (output.empty()) {
        std::cerr << "hardener returned empty output for " << test.kind << '\n';
        return 1;
      }
      if (std::string_view(test.kind) == "loader" && output.find("vbind:stress-token") == std::string::npos) {
        std::cerr << "loader binding marker was not preserved\n";
        return 2;
      }
    }
    shutdown();
  }

  const auto stats = runtime_stats();
  if (stats.calls != kCases.size() * 2u || stats.fallbacks < 8u ||
      (subprocess_mode && (stats.subprocess_calls < 6u || stats.subprocess_failures != 0u)) ||
      (retry_mode && stats.subprocess_retries < 6u) ||
      (!subprocess_mode && (stats.initializations != 6u || stats.resets != 4u))) {
    std::cerr << "unexpected hardener lifecycle stats: calls=" << stats.calls
              << " initializations=" << stats.initializations
              << " resets=" << stats.resets
              << " fallbacks=" << stats.fallbacks
              << " subprocess_calls=" << stats.subprocess_calls
              << " subprocess_retries=" << stats.subprocess_retries
              << " subprocess_failures=" << stats.subprocess_failures << '\n';
    return 3;
  }

  std::cout << "[venom] native JS hardener stress: PASS (" << stats.calls
            << " calls, " << stats.subprocess_calls << " isolated subprocess calls, "
            << stats.fallbacks << " conservative hardenings)\n";
  return 0;
}
