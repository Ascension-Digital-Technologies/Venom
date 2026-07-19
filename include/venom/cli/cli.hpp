#pragma once

#include "venom/core/options.hpp"

#include <filesystem>

namespace venom::compiler {

enum class CommandKind {
  Help,
  Version,
  Build,
  Dev,
  Inspect,
  Keygen,
  Verify,
  VerifyRuntime,
  Doctor,
  Compatibility,
  AnalyzeDist,
  Contracts,
  NewProject,
  InitProject,
  Runtime,
  Config,
  Update,
  Plan,
  CompileSnippet,
};

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
  int analyze_dist_verbosity = 1;
  ProjectCommandOptions project;
  RuntimeCommandOptions runtime;
  ConfigCommandOptions config;
  UpdateCommandOptions update;
};

Command parse_command(int argc, char** argv);
void print_help();
void print_version();

} // namespace venom::compiler
