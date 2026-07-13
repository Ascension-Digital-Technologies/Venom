#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace venom::compiler {

enum class OutputFormat { Text, Json };

enum class CommandKind {
  Help,
  Version,
  Build,
  Inspect,
  Keygen,
  ReleaseCheck,
  VerifyRuntime,
  Doctor,
  Compatibility,
  Analyze,
  Contracts,
};

struct BuildOptions {
  std::filesystem::path input;
  std::filesystem::path output = "dist";
  std::string profile = "browser-protect";
  bool hashed_assets = true;
  std::string package_mode = "external";
  std::string runtime = "wasm";
  std::string quickjs_backend = "wasm-real";
  std::string crypto_provider = "runtime";
  bool allow_host_fallback = false;
  bool deny_host_fallback = true;
  bool strict_release = true;
  bool emit_debug_manifest = false;
  std::filesystem::path key_file;
  bool require_audited_crypto = false;
  std::string security_target = "browser";
  std::filesystem::path vendor_cache;
  std::filesystem::path vendor_lock;
  bool vendor_offline = false;
  bool refresh_vendors = false;
  std::filesystem::path config_file;
  OutputFormat format = OutputFormat::Text;
};

struct InspectOptions {
  std::filesystem::path package;
  std::filesystem::path key_file;
};

struct KeygenOptions {
  std::filesystem::path output;
  bool force = false;
};

struct ReleaseCheckOptions {
  std::filesystem::path target;
  std::filesystem::path key_file;
  std::string security_target = "browser";
  bool require_audited_crypto = false;
  bool runtime_verification = false;
  bool require_real_engine = false;
};

struct DoctorOptions { OutputFormat format = OutputFormat::Text; };
struct CompatibilityOptions { std::filesystem::path input; OutputFormat format = OutputFormat::Text; };
struct ContractsOptions { OutputFormat format = OutputFormat::Text; };

struct Command {
  CommandKind kind = CommandKind::Help;
  BuildOptions build;
  InspectOptions inspect;
  KeygenOptions keygen;
  ReleaseCheckOptions release_check;
  DoctorOptions doctor;
  CompatibilityOptions compatibility;
  ContractsOptions contracts;
};

Command parse_command(int argc, char** argv);
void print_help();
void print_version();

} // namespace venom::compiler
