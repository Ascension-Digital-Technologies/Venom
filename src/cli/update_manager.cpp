#include "venom/base/error.hpp"
#include "update_manager.hpp"
#include "venom/core/process.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace venom::compiler {
namespace fs = std::filesystem;
namespace {
fs::path find_tool(const fs::path& executable) {
  std::vector<fs::path> roots;
  std::error_code ec;
  auto p = fs::absolute(executable, ec);
  if (!ec) {
    roots.push_back(p.parent_path());
    roots.push_back(p.parent_path().parent_path());
    roots.push_back(p.parent_path().parent_path().parent_path());
  }
  roots.push_back(fs::current_path());
#ifdef VENOM_SOURCE_ROOT
  roots.push_back(fs::path(VENOM_SOURCE_ROOT));
#endif
  for (auto root : roots) {
    for (int i=0;i<5 && !root.empty();++i) {
      const auto candidate = root / "tools" / "update_release.py";
      if (fs::is_regular_file(candidate)) return candidate;
      root = root.parent_path();
    }
  }
  raise_error("VENOM-E1000", "update tool not found; expected tools/update_release.py beside the Venom release or source tree");
}
std::string find_python() {
#ifdef _WIN32
  if (run_process("py", {"-3", "--version"}) == 0) return "py";
  if (run_process("python", {"--version"}) == 0) return "python";
#else
  if (run_process("python3", {"--version"}) == 0) return "python3";
  if (run_process("python", {"--version"}) == 0) return "python";
#endif
  raise_error("VENOM-E1000", "Python 3 is required for secure release updates");
}
}
bool manage_update(const UpdateOptions& o, const fs::path& executable) {
  const auto tool=find_tool(executable);
  const auto python=find_python();
  std::vector<std::string> args;
#ifdef _WIN32
  if (python=="py") args.push_back("-3");
#endif
  args.push_back(tool.string());
  args.push_back(o.action);
  args.push_back("--channel"); args.push_back(o.channel);
  if (!o.manifest.empty()) { args.push_back("--manifest"); args.push_back(o.manifest); }
  if (!o.prefix.empty()) { args.push_back("--prefix"); args.push_back(o.prefix.string()); }
  if (!o.public_key.empty()) { args.push_back("--public-key"); args.push_back(o.public_key.string()); }
  if (o.require_signature) args.push_back("--require-signature");
  if (o.yes) args.push_back("--yes");
  if (o.dry_run) args.push_back("--dry-run");
  args.push_back("--format"); args.push_back(o.format);
  const int code=run_process(python,args);
  if (code < 0) raise_error("VENOM-E1000", "failed to start update helper");
  return code == 0;
}
}
