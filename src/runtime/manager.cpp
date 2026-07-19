#include "venom/base/error.hpp"
#include "venom/runtime/manager.hpp"
#include "venom/generated/runtime/quickjs_runtime_wasm_blob.hpp"
#include "venom/core/version.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace venom::compiler {
namespace fs = std::filesystem;
fs::path default_runtime_directory() {
#ifdef _WIN32
  if (const char* p = std::getenv("LOCALAPPDATA")) return fs::path(p) / "Venom" / "runtime" / VENOM_VERSION_STRING;
  if (const char* p = std::getenv("USERPROFILE")) return fs::path(p) / ".venom" / "runtime" / VENOM_VERSION_STRING;
#else
  if (const char* p = std::getenv("XDG_CACHE_HOME")) return fs::path(p) / "venom" / "runtime" / VENOM_VERSION_STRING;
  if (const char* p = std::getenv("HOME")) return fs::path(p) / ".cache" / "venom" / "runtime" / VENOM_VERSION_STRING;
#endif
  return fs::current_path() / ".venom-runtime" / VENOM_VERSION_STRING;
}
namespace {
fs::path wasm_path(const RuntimeOptions& o) { return (o.directory.empty() ? default_runtime_directory() : fs::absolute(o.directory)) / "quickjs-runtime.wasm"; }
bool valid_wasm(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  unsigned char h[8]{}; in.read(reinterpret_cast<char*>(h), 8);
  return in.gcount()==8 && h[0]==0x00 && h[1]==0x61 && h[2]==0x73 && h[3]==0x6d && fs::file_size(path)==sizeof(kQuickJsRuntimeWasmBlob);
}
void install(const fs::path& path, bool force) {
  if (fs::exists(path) && valid_wasm(path) && !force) { std::cout << "[venom] runtime already installed: " << path.string() << "\n"; return; }
  fs::create_directories(path.parent_path());
  const auto tmp=path.string()+".tmp";
  { std::ofstream out(tmp, std::ios::binary|std::ios::trunc); if(!out) raise_error("VENOM-E8000", "failed to create runtime: "+tmp); out.write(reinterpret_cast<const char*>(kQuickJsRuntimeWasmBlob), sizeof(kQuickJsRuntimeWasmBlob)); }
  if (!valid_wasm(tmp)) { fs::remove(tmp); raise_error("VENOM-E8000", "embedded runtime verification failed"); }
  if (fs::exists(path)) fs::remove(path);
  fs::rename(tmp,path);
  std::ofstream manifest(path.parent_path()/"runtime.json", std::ios::trunc);
  manifest << "{\n  \"product\": \"Venom QuickJS/WASM\",\n  \"version\": \"" << VENOM_VERSION_STRING << "\",\n  \"bytes\": " << sizeof(kQuickJsRuntimeWasmBlob) << "\n}\n";
  std::cout << "[venom] installed runtime: " << path.string() << "\n";
}
}
bool manage_runtime(const RuntimeOptions& options) {
  const auto path=wasm_path(options);
  if (options.action=="install" || options.action=="update") { install(path, options.force || options.action=="update"); return true; }
  if (options.action=="verify") {
    const bool ok=fs::exists(path)&&valid_wasm(path);
    std::cout << (ok?"[PASS] ":"[FAIL] ") << "QuickJS/WASM runtime: " << path.string() << "\n";
    return ok;
  }
  if (options.action=="path") { std::cout << path.string() << "\n"; return true; }
  raise_error("VENOM-E8000", "runtime action must be install, verify, update, or path");
}
}
