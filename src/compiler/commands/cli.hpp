#pragma once

#include <filesystem>
#include <string>
#include <cstdint>
#include <vector>

namespace venom::compiler {

enum class OutputFormat { Text, Json };

enum class CommandKind {
  Help,
  Version,
  Build,
  Dev,
  Inspect,
  Keygen,
  ReleaseCheck,
  VerifyRuntime,
  Doctor,
  Compatibility,
  Analyze,
  AnalyzeDist,
  Contracts,
  NewProject,
  InitProject,
  Runtime,
  Config,
  Update,
  Plan,
};

struct BuildOptions {
  std::filesystem::path input;
  std::filesystem::path output = "dist";
  std::string profile = "prod";
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
  std::uint32_t diversification_seed = 0;
  std::string protection_level = "strong";
  std::string planner_mode = "manual";
  int planner_minimum_confidence = 70;
  std::vector<std::string> force_protected;
  std::vector<std::string> force_browser;
  std::filesystem::path config_file;
  OutputFormat format = OutputFormat::Text;
  int verbosity = 1; // 0=quiet, 1=phases, 2=verbose internals
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

struct DevOptions { std::filesystem::path executable; std::filesystem::path input; std::filesystem::path output = "dist-dev"; std::string host = "127.0.0.1"; std::uint16_t port = 8080; bool open_browser = false; bool watch = true; };
struct DoctorOptions { OutputFormat format = OutputFormat::Text; std::string profile = "development"; };
struct CompatibilityOptions { std::filesystem::path input; OutputFormat format = OutputFormat::Text; };
struct PlannerOptions {
  std::filesystem::path input;
  OutputFormat format = OutputFormat::Text;
  std::string mode = "recommend";
  int minimum_confidence = 70;
  std::vector<std::string> protected_patterns;
  std::vector<std::string> browser_patterns;
  std::vector<std::string> config_protected_patterns;
  std::vector<std::string> config_browser_patterns;
  std::filesystem::path config_file;
  std::filesystem::path report_file;
};
struct ContractsOptions { OutputFormat format = OutputFormat::Text; };
struct DistAnalyzeOptions;
struct ProjectCommandOptions { std::filesystem::path directory; bool force = false; };
struct RuntimeCommandOptions { std::string action; std::filesystem::path directory; bool force = false; };
struct ConfigCommandOptions { std::string action; std::filesystem::path file; OutputFormat format = OutputFormat::Text; };
struct UpdateCommandOptions { std::string action; std::string channel = "stable"; std::string manifest; std::filesystem::path prefix; std::filesystem::path public_key; bool require_signature = false; bool yes = false; bool dry_run = false; OutputFormat format = OutputFormat::Text; };

struct Command {
  CommandKind kind = CommandKind::Help;
  BuildOptions build;
  DevOptions dev;
  InspectOptions inspect;
  KeygenOptions keygen;
  ReleaseCheckOptions release_check;
  DoctorOptions doctor;
  CompatibilityOptions compatibility;
  PlannerOptions planner;
  ContractsOptions contracts;
  std::filesystem::path analyze_dist_input;
  OutputFormat analyze_dist_format = OutputFormat::Text;
  ProjectCommandOptions project;
  RuntimeCommandOptions runtime;
  ConfigCommandOptions config;
  UpdateCommandOptions update;
};

Command parse_command(int argc, char** argv);
void print_help();
void print_version();

} // namespace venom::compiler
