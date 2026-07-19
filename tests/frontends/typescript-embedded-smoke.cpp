#include "venom/core/diagnostic.hpp"
#include "venom/frontends/typescript/frontend.hpp"
#include "venom/internal/frontends/typescript/typescript_runtime.hpp"
#include "venom/generated/compiler/typescript_bridge_bytecode.hpp"
#include "venom/generated/compiler/typescript_compiler_asset.hpp"

#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void set_bootstrap(const char* value) {
#if defined(_WIN32)
  _putenv_s("VENOM_TYPESCRIPT_BRIDGE", value ? value : "");
#else
  if (value) setenv("VENOM_TYPESCRIPT_BRIDGE", value, 1);
  else unsetenv("VENOM_TYPESCRIPT_BRIDGE");
#endif
}

void clear_frontend_override() {
#if defined(_WIN32)
  _putenv_s("VENOM_TYPESCRIPT_FRONTEND", "");
#else
  unsetenv("VENOM_TYPESCRIPT_FRONTEND");
#endif
}
}

int main() {
  clear_frontend_override();

  if (venom::generated::typescript::compiler_source_size() < 8ULL * 1024ULL * 1024ULL)
    throw std::runtime_error("embedded TypeScript compiler asset is unexpectedly small");
  if (venom::generated::typescript::compiler_source_sha256().size() != 64)
    throw std::runtime_error("embedded TypeScript compiler SHA-256 metadata is invalid");
  if (venom::generated::typescript::compiler_version() != "5.8")
    throw std::runtime_error("embedded TypeScript compiler version metadata is unexpected");

  if (venom::generated::typescript::bridge_bytecode_size() < 1000)
    throw std::runtime_error("embedded TypeScript bridge bytecode asset is unexpectedly small");
  if (venom::generated::typescript::bridge_bytecode_sha256().size() != 64)
    throw std::runtime_error("embedded TypeScript bridge bytecode SHA-256 metadata is invalid");
  if (venom::generated::typescript::bridge_bytecode_quickjs_version() != "0.15.1")
    throw std::runtime_error("embedded TypeScript bridge bytecode QuickJS metadata is unexpected");
  if (venom::generated::typescript::bridge_bytecode_abi() != 27)
    throw std::runtime_error("embedded TypeScript bridge bytecode ABI metadata is unexpected");

  set_bootstrap("bytecode");
  venom::compiler::typescript::EmbeddedRuntime bytecode_runtime;
  if (bytecode_runtime.bootstrap_mode() != "verified-source+pinned-bridge-bytecode")
    throw std::runtime_error("TypeScript runtime did not use pinned bridge bytecode");
  const auto bytecode_result = bytecode_runtime.transpile("export const n: number = 7;", "bytecode.ts");
  if (bytecode_result.javascript.find("const n = 7") == std::string::npos)
    throw std::runtime_error("bytecode bootstrap did not transpile TypeScript");

  set_bootstrap("source");
  venom::compiler::typescript::EmbeddedRuntime source_runtime;
  if (source_runtime.bootstrap_mode() != "verified-source+source-bridge")
    throw std::runtime_error("TypeScript source bridge fallback was not selected");
  const auto source_result = source_runtime.transpile("export const n: number = 7;", "source.ts");
  if (source_result.javascript != bytecode_result.javascript)
    throw std::runtime_error("source bridge output differs from bytecode bridge output");
  set_bootstrap(nullptr);

  const auto cache = std::filesystem::temp_directory_path() / "venom-typescript-pass2-cache";
  std::filesystem::remove_all(cache);
  venom::compiler::typescript::configure_cache(true, cache, false);
  venom::compiler::typescript::reset_cache_stats();

  const std::string source =
      "interface User { name: string }\n"
      "const user: User = { name: 'Venom' };\n"
      "export const greeting: string = `Hello ${user.name}`;\n";

  const auto first = venom::compiler::typescript::transpile(source, "smoke.ts");
  if (!first.typescript) throw std::runtime_error("TypeScript marker was not set");
  if (first.frontend != "embedded-quickjs-typescript") throw std::runtime_error("embedded frontend is not the default");
  if (first.frontend_version.empty()) throw std::runtime_error("TypeScript version was not reported");
  if (first.javascript.find("interface User") != std::string::npos) throw std::runtime_error("interface was not erased");
  if (first.javascript.find("const user") == std::string::npos) throw std::runtime_error("runtime JavaScript was not emitted");
  if (first.source_map.empty()) throw std::runtime_error("source map was not emitted");
  if (!first.diagnostics.empty() || first.diagnostic_count != 0) throw std::runtime_error("unexpected TypeScript diagnostics");

  const auto after_first = venom::compiler::typescript::cache_stats();
  if (after_first.misses != 1 || after_first.writes != 1) throw std::runtime_error("embedded cache miss/write accounting failed");

  const auto second = venom::compiler::typescript::transpile(source, "smoke.ts");
  const auto after_second = venom::compiler::typescript::cache_stats();
  if (after_second.hits != 1) throw std::runtime_error("embedded cache hit accounting failed");
  if (second.javascript != first.javascript || second.source_map != first.source_map)
    throw std::runtime_error("embedded cache changed transpilation output");

  const auto tsx = venom::compiler::typescript::transpile(
      "type Props = { name: string }; export const View = (p: Props) => <section>Hello {p.name}</section>;",
      "view.tsx");
  if (!tsx.tsx || tsx.lowered_jsx_elements == 0) throw std::runtime_error("TSX was not lowered by Venom");
  if (tsx.javascript.find("<section>") != std::string::npos) throw std::runtime_error("raw JSX remained after lowering");

  bool saw_diagnostic = false;
  try {
    (void)venom::compiler::typescript::transpile("const value: = 1;", "broken.ts");
  } catch (const venom::compiler::DiagnosticError& error) {
    saw_diagnostic = error.code() == "VENOM-E2505" && error.location().line == 1 &&
                     error.detail().find("TS") != std::string::npos;
  }
  if (!saw_diagnostic) throw std::runtime_error("structured TypeScript diagnostic was not raised");

  bool rejected_node_frontend = false;
#if defined(_WIN32)
  _putenv_s("VENOM_TYPESCRIPT_FRONTEND", "node");
#else
  setenv("VENOM_TYPESCRIPT_FRONTEND", "node", 1);
#endif
  try {
    (void)venom::compiler::typescript::transpile("export const value: number = 1;", "legacy.ts");
  } catch (const std::runtime_error& error) {
    rejected_node_frontend = std::string(error.what()).find("no longer supported") != std::string::npos;
  }
  clear_frontend_override();
  if (!rejected_node_frontend) throw std::runtime_error("legacy Node frontend selector was not rejected");

  std::filesystem::remove_all(cache);
  std::cout << "embedded TypeScript Pass 5 pinned bridge bytecode smoke passed (TypeScript "
            << first.frontend_version << ", bytecode init=" << bytecode_runtime.initialization_milliseconds()
            << " ms, source init=" << source_runtime.initialization_milliseconds() << " ms)\n";
  return 0;
}
