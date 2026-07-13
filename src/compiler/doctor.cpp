#include "compiler/doctor.hpp"
#include "compiler/version.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
struct Check { std::string name, detail; bool ok; };
bool executable_on_path(const char* name) {
#ifdef _WIN32
  const std::string command=std::string("where ")+name+" >nul 2>nul";
#else
  const std::string command=std::string("command -v ")+name+" >/dev/null 2>&1";
#endif
  return std::system(command.c_str())==0;
}
std::string json_escape(const std::string& value){std::string out;for(char c:value){if(c=='"'||c=='\\')out+='\\';out+=c;}return out;}
}
bool run_doctor(const DoctorOptions& options) {
  std::vector<Check> checks;
  checks.push_back({"cmake","required build generator",executable_on_path("cmake")});
  checks.push_back({"python","required release tooling",executable_on_path("python3")||executable_on_path("python")});
  checks.push_back({"git","recommended source and provenance tooling",executable_on_path("git")});
  checks.push_back({"emcc","required for production QuickJS/WASM rebuilds",executable_on_path("emcc")});
  checks.push_back({"repository","CMakeLists.txt and src/ detected",std::filesystem::exists("CMakeLists.txt")&&std::filesystem::exists("src")});
  bool ok=true; for(const auto& c:checks) ok=ok&&c.ok;
  if (options.format==OutputFormat::Json) {
    std::cout << "{\"product\":\"" << json_escape(VENOM_PRODUCT_NAME) << "\",\"version\":\"" << VENOM_VERSION_STRING << "\",\"ok\":" << (ok?"true":"false") << ",\"checks\":[";
    for(std::size_t i=0;i<checks.size();++i){if(i)std::cout<<',';const auto& c=checks[i];std::cout<<"{\"name\":\""<<json_escape(c.name)<<"\",\"ok\":"<<(c.ok?"true":"false")<<",\"detail\":\""<<json_escape(c.detail)<<"\"}";}
    std::cout << "]}\n";
  } else {
    std::cout << VENOM_PRODUCT_NAME << " environment check\n\n";
    for(const auto& c:checks) std::cout << '[' << (c.ok?"PASS":"FAIL") << "] " << c.name << " - " << c.detail << '\n';
    std::cout << "\nResult: " << (ok?"ready":"action required") << '\n';
  }
  return ok;
}
}
