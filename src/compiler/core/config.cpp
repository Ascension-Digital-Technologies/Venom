#include "compiler/core/config.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>

namespace venom::compiler {
namespace {
std::string trim(std::string s) {
  auto not_space=[](unsigned char c){return !std::isspace(c);};
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}
std::string unquote(std::string v) {
  v=trim(v);
  if (v.size()>=2 && ((v.front()=='"'&&v.back()=='"')||(v.front()=='\''&&v.back()=='\''))) return v.substr(1,v.size()-2);
  return v;
}
bool parse_bool(const std::string& value, const std::string& key) {
  const auto v=unquote(value);
  if (v=="true") return true;
  if (v=="false") return false;
  throw std::runtime_error("venom.toml: " + key + " must be true or false");
}
}
std::filesystem::path discover_project_config(const std::filesystem::path& start) {
  auto current=std::filesystem::absolute(start.empty()?std::filesystem::current_path():start);
  if (!std::filesystem::is_directory(current)) current=current.parent_path();
  for (;;) {
    const auto candidate=current/"venom.toml";
    if (std::filesystem::exists(candidate)) return candidate;
    if (current==current.root_path()) break;
    current=current.parent_path();
  }
  return {};
}
void apply_project_config(const std::filesystem::path& path, BuildOptions& options) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("failed to open config: " + path.string());
  std::string section,line;
  std::size_t line_no=0;
  while (std::getline(in,line)) {
    ++line_no;
    const auto comment=line.find('#'); if (comment!=std::string::npos) line.erase(comment);
    line=trim(line); if (line.empty()) continue;
    if (line.front()=='[' && line.back()==']') { section=trim(line.substr(1,line.size()-2)); continue; }
    const auto eq=line.find('='); if (eq==std::string::npos) throw std::runtime_error("venom.toml:"+std::to_string(line_no)+": expected key = value");
    const auto key=trim(line.substr(0,eq)); const auto value=trim(line.substr(eq+1)); const auto full=section+"."+key;
    if (full=="project.entry" || full=="project.input") options.input=unquote(value);
    else if (full=="project.output") options.output=unquote(value);
    else if (full=="build.profile") options.profile=unquote(value);
    else if (full=="build.vendor_cache") options.vendor_cache=unquote(value);
    else if (full=="build.vendor_lock") options.vendor_lock=unquote(value);
    else if (full=="build.offline") options.vendor_offline=parse_bool(value,full);
    else if (full=="build.refresh_vendors") options.refresh_vendors=parse_bool(value,full);
    else if (full=="security.require_audited_crypto") options.require_audited_crypto=parse_bool(value,full);
    else if (full=="security.deny_host_js_fallback" && !parse_bool(value,full)) throw std::runtime_error("Venom production policy requires security.deny_host_js_fallback=true");
    else if (full=="runtime.engine" && unquote(value)!="quickjs-wasm") throw std::runtime_error("Venom production policy requires runtime.engine=\"quickjs-wasm\"");
    else if (full=="runtime.fail_closed" && !parse_bool(value,full)) throw std::runtime_error("Venom production policy requires runtime.fail_closed=true");
    else if (section=="project" || section=="build" || section=="runtime" || section=="security" || section=="compatibility") {
      // Forward-compatible: recognized sections may contain fields consumed by newer runtimes.
    } else throw std::runtime_error("venom.toml:"+std::to_string(line_no)+": unknown section or key: "+full);
  }
  options.config_file=std::filesystem::absolute(path);
  const auto config_dir = options.config_file.parent_path();
  // Project paths are relative to the configuration file, not the caller's
  // current working directory. Absolute paths remain unchanged.
  if (!options.input.empty() && options.input.is_relative()) options.input = config_dir / options.input;
  if (!options.output.empty() && options.output.is_relative()) options.output = config_dir / options.output;
  if (!options.vendor_cache.empty() && options.vendor_cache.is_relative()) options.vendor_cache = config_dir / options.vendor_cache;
  if (!options.vendor_lock.empty() && options.vendor_lock.is_relative()) options.vendor_lock = config_dir / options.vendor_lock;
}
}


namespace venom::compiler {
bool validate_project_config(const std::filesystem::path& path, std::string* error) {
  try {
    BuildOptions options;
    apply_project_config(path, options);
    if (options.input.empty()) throw std::runtime_error("venom.toml: project.entry or project.input is required");
    if (options.profile != "dev" && options.profile != "prod") throw std::runtime_error("venom.toml: build.profile must be dev or prod");
    return true;
  } catch (const std::exception& ex) {
    if (error) *error = ex.what();
    return false;
  }
}
void print_project_config(const std::filesystem::path& path, OutputFormat format) {
  BuildOptions options;
  apply_project_config(path, options);
  if (options.input.empty()) throw std::runtime_error("venom.toml: project.entry or project.input is required");
  if (options.profile != "dev" && options.profile != "prod") throw std::runtime_error("venom.toml: build.profile must be dev or prod");
  if (format == OutputFormat::Json) {
    std::cout << "{\"config\":\"" << std::filesystem::absolute(path).generic_string()
      << "\",\"entry\":\"" << options.input.generic_string()
      << "\",\"output\":\"" << options.output.generic_string()
      << "\",\"profile\":\"" << options.profile
      << "\",\"runtime\":\"quickjs-wasm\",\"fail_closed\":true}\n";
  } else {
    std::cout << "Venom project configuration\n"
      << "  file: " << std::filesystem::absolute(path).string() << "\n"
      << "  entry: " << options.input.string() << "\n"
      << "  output: " << options.output.string() << "\n"
      << "  profile: " << options.profile << "\n"
      << "  runtime: quickjs-wasm\n"
      << "  host fallback: denied\n";
  }
}
}
