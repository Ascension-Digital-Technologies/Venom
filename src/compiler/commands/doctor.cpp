#include "compiler/commands/doctor.hpp"
#include "compiler/core/version.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
struct Check { std::string name, detail, remedy; enum class Status { Pass, Warn, Fail } status; };

struct Probe { bool ok = false; std::string output; };
Probe probe(const std::string& command) {
#ifdef _WIN32
  FILE* pipe = _popen((command + " 2>&1").c_str(), "r");
#else
  FILE* pipe = popen((command + " 2>&1").c_str(), "r");
#endif
  if (!pipe) return {};
  std::array<char, 512> buffer{};
  std::string output;
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) output += buffer.data();
#ifdef _WIN32
  const int status = _pclose(pipe);
#else
  const int status = pclose(pipe);
#endif
  while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) output.pop_back();
  return {status == 0, output};
}

bool executable_on_path(const char* name) {
#ifdef _WIN32
  return probe(std::string("where ") + name).ok;
#else
  return probe(std::string("command -v ") + name).ok;
#endif
}

std::string first_line(std::string value) {
  const auto pos = value.find_first_of("\r\n");
  if (pos != std::string::npos) value.resize(pos);
  return value;
}

std::string json_escape(const std::string& value) { std::string out; for(char c:value){if(c=='"'||c=='\\')out+='\\'; if(c=='\n'){out+="\\n";continue;} out+=c;} return out; }
void add(std::vector<Check>& c, std::string n, std::string d, bool ok, std::string remedy={}, bool required=true) { c.push_back({std::move(n),std::move(d),std::move(remedy),ok?Check::Status::Pass:(required?Check::Status::Fail:Check::Status::Warn)}); }

bool required_for(const std::string& profile, const std::string& component) {
  if (component == "emscripten") return profile == "runtime-contributor";
  if (component == "js-hardener") return profile == "production" || profile == "runtime-contributor";
  return true;
}
}

bool run_doctor(const DoctorOptions& options) {
  namespace fs = std::filesystem;
  std::vector<Check> checks;
  const auto required = [&](const std::string& component){ return required_for(options.profile, component); };

  add(checks, "repository", "CMakeLists.txt and src/ detected", fs::exists("CMakeLists.txt") && fs::exists("src"), "Run venom doctor from the repository root.");

  const auto cmake = probe("cmake --version");
  add(checks, "cmake", cmake.ok ? first_line(cmake.output) : "not available", cmake.ok, "Install CMake 3.20 or newer.");

#ifdef _WIN32
  const bool compiler_found = executable_on_path("cl") || executable_on_path("clang++") || executable_on_path("g++");
#else
  const bool compiler_found = executable_on_path("c++") || executable_on_path("clang++") || executable_on_path("g++");
#endif
  add(checks, "compiler", "C++17 compiler executable", compiler_found, "Install a C++17-capable Clang, GCC, or Visual Studio toolchain.");

  std::string python_command = executable_on_path("python3") ? "python3" : "python";
  const auto python = probe(python_command + " -c \"import sys; assert sys.version_info >= (3,10); print(sys.version.split()[0])\"");
  add(checks, "python", python.ok ? "Python " + first_line(python.output) : "Python 3.10+ probe failed", python.ok, "Install Python 3.10 or newer.");


  const bool embedded_hardener =
      fs::exists("src/compiler/pipeline/native_js_hardener.cpp") &&
      fs::exists("src/compiler/pipeline/embedded_js_hardener_bundles.inc");

  add(checks, "js-hardener", embedded_hardener ? "embedded QuickJS hardener: Terser 5.49.0 + javascript-obfuscator 5.4.7" : "embedded hardener payload missing",
      embedded_hardener, "Restore the native hardener source and embedded bundle payload.", required("js-hardener"));

  const bool blob_present = fs::exists("src/generated/runtime/quickjs_runtime_wasm_blob.hpp");
  add(checks, "quickjs-wasm-source", blob_present ? "embedded runtime source present" : "embedded runtime source missing", blob_present, "Run the QuickJS/WASM setup and build scripts.");
  Probe quickjs;
  if (blob_present && fs::exists("tools/quickjs_wasm_cutover.py")) {
    quickjs = probe(python_command + " tools/quickjs_wasm_cutover.py --repo-root . --verify-embedded --require-real");
  }
  add(checks, "quickjs-wasm-verification", quickjs.ok ? "strict embedded provenance verified" : "strict embedded provenance verification failed", quickjs.ok, "Rebuild and verify the canonical QuickJS/WASM runtime.");

  const auto emcc = probe("emcc --version");
  add(checks, "emscripten", emcc.ok ? first_line(emcc.output) : "emcc not available", emcc.ok, "Run scripts/setup-emscripten.* before rebuilding the runtime.", required("emscripten"));

  const auto git = probe("git --version");
  add(checks, "git", git.ok ? first_line(git.output) : "git not available", git.ok, "Install Git for release provenance.", false);

  bool ok = true;
  for (const auto& c : checks) if (c.status == Check::Status::Fail) ok = false;
  if (options.format == OutputFormat::Json) {
    std::cout << "{\"product\":\"" << json_escape(VENOM_PRODUCT_NAME) << "\",\"version\":\"" << VENOM_VERSION_STRING << "\",\"profile\":\"" << json_escape(options.profile) << "\",\"ok\":" << (ok?"true":"false") << ",\"checks\":[";
    for(std::size_t i=0;i<checks.size();++i){if(i)std::cout<<',';const auto& c=checks[i];const char* st=c.status==Check::Status::Pass?"pass":c.status==Check::Status::Warn?"warn":"fail";std::cout<<"{\"name\":\""<<json_escape(c.name)<<"\",\"status\":\""<<st<<"\",\"detail\":\""<<json_escape(c.detail)<<"\",\"remedy\":\""<<json_escape(c.remedy)<<"\"}";}
    std::cout << "]}\n";
  } else {
    std::cout << VENOM_PRODUCT_NAME << " " << VENOM_VERSION_STRING << " environment check (" << options.profile << ")\n\n";
    for(const auto& c:checks){const char* st=c.status==Check::Status::Pass?"PASS":c.status==Check::Status::Warn?"WARN":"FAIL";std::cout<<'['<<st<<"] "<<c.name<<" - "<<c.detail<<'\n';if(c.status!=Check::Status::Pass&&!c.remedy.empty())std::cout<<"       "<<c.remedy<<'\n';}
    std::cout << "\nResult: " << (ok?"ready":"action required") << '\n';
  }
  return ok;
}
}
