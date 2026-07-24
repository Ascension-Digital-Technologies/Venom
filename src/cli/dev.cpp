#include "base/error.hpp"
#include "cli/dev.hpp"
#include "core/process.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
namespace venom::compiler {
bool run_dev(const DevOptions& o) {
  auto script = std::filesystem::current_path()/"tools"/"dev_server.py";
  if (!std::filesystem::exists(script)) raise_error("VENOM-E1000", "tools/dev_server.py was not found; run venom dev from the repository root");
#ifdef _WIN32
  const char* py = "python";
#else
  const char* py = "python3";
#endif
  std::vector<std::string> args{script.string(),"--venom",std::filesystem::absolute(o.executable).string(),"--site",o.input.string(),"--dist",o.output.string(),"--host",o.host,"--port",std::to_string(o.port)};
  if (o.open_browser) args.push_back("--open");
  if (!o.watch) args.push_back("--no-watch");
  return run_process(py,args)==0;
}
}
