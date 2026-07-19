#include "venom/base/error.hpp"
#include "venom/core/project.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace venom::compiler {
namespace fs = std::filesystem;
namespace {
void write_text(const fs::path& path, const std::string& text, bool force) {
  if (fs::exists(path) && !force) raise_error("VENOM-E1100", "refusing to overwrite existing file: " + path.string() + " (use --force)");
  fs::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) raise_error("VENOM-E1100", "failed to write: " + path.string());
  out << text;
}
std::string project_config() {
  return "[project]\nentry = \".\"\noutput = \"dist\"\n\n[build]\nprofile = \"prod\"\n\n[runtime]\nengine = \"quickjs-wasm\"\nfail_closed = true\n\n[security]\ndeny_host_js_fallback = true\n";
}
}
bool initialize_project(const InitProjectOptions& options) {
  const auto root = fs::absolute(options.directory);
  if (!fs::exists(root)) fs::create_directories(root);
  if (!fs::is_directory(root)) raise_error("VENOM-E1100", "init target is not a directory: " + root.string());
  write_text(root / "venom.toml", project_config(), options.force);
  if (!fs::exists(root / "index.html") || options.force) {
    write_text(root / "index.html", "<!doctype html>\n<html lang=\"en\">\n<head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Venom App</title><link rel=\"stylesheet\" href=\"assets/css/app.css\"></head>\n<body><main><h1>Venom App</h1><p id=\"result\">Loading protected logic…</p></main><script type=\"module\" src=\"assets/js/app.js\"></script></body>\n", options.force);
  }
  write_text(root / "assets/css/app.css", "html{font-family:system-ui,sans-serif;color-scheme:dark}body{max-width:760px;margin:4rem auto;padding:0 1rem}h1{font-size:2.5rem}\n", options.force);
  write_text(root / "protected/engine.js", "// @venom: protected module\n\n// @venom: input value:number\n// @venom: output result:number\nexport function calculate(input) {\n  return { result: input.value * 2 };\n}\n", options.force);
  write_text(root / "assets/js/app.js", "import { calculate } from '../../protected/engine.js';\n\nconst output = await calculate({ value: 21 });\ndocument.querySelector('#result').textContent = `Protected result: ${output.result}`;\n", options.force);
  write_text(root / "README.md", "# Venom App\n\n```powershell\nvenom dev . --open\nvenom build . --profile prod --out dist\n```\n\nProtected code lives in `protected/`; browser code lives in `assets/js/`.\n", options.force);
  std::cout << "[venom] initialized project: " << root.string() << "\n";
  std::cout << "[venom] next: cd \"" << root.string() << "\" && venom dev . --open\n";
  return true;
}
bool create_project(const NewProjectOptions& options) {
  if (options.directory.empty()) raise_error("VENOM-E1100", "new requires a project name or path");
  const auto root = fs::absolute(options.directory);
  if (fs::exists(root) && !fs::is_empty(root) && !options.force) raise_error("VENOM-E1100", "target directory is not empty: " + root.string() + " (use --force)");
  fs::create_directories(root);
  return initialize_project({root, options.force});
}
}
