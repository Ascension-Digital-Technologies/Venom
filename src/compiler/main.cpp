#include "compiler/pipeline/build.hpp"
#include "compiler/commands/cli.hpp"
#include "compiler/commands/inspect.hpp"
#include "compiler/commands/doctor.hpp"
#include "compiler/commands/dev.hpp"
#include "compiler/core/compatibility.hpp"
#include "compiler/commands/dist_analyzer.hpp"
#include "compiler/pipeline/security.hpp"
#include "compiler/core/project.hpp"
#include "compiler/services/runtime_manager.hpp"
#include "compiler/services/update_manager.hpp"
#include "compiler/core/config.hpp"
#include "compiler/core/planner.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
  try {
    const auto command = venom::compiler::parse_command(argc, argv);
    switch (command.kind) {
      case venom::compiler::CommandKind::Help:
        venom::compiler::print_help();
        return 0;
      case venom::compiler::CommandKind::Version:
        venom::compiler::print_version();
        return 0;
      case venom::compiler::CommandKind::Build:
        return venom::compiler::build_site(command.build) ? 0 : 70;
      case venom::compiler::CommandKind::Dev:
        return venom::compiler::run_dev(command.dev) ? 0 : 60;
      case venom::compiler::CommandKind::Inspect:
        if (!command.inspect.key_file.empty()) {
          venom::compiler::load_package_key_file_for_process(command.inspect.key_file);
        }
        return venom::compiler::inspect_package(command.inspect.package) ? 0 : 30;
      case venom::compiler::CommandKind::Keygen:
        return venom::compiler::generate_key_file(command.keygen) ? 0 : 40;
      case venom::compiler::CommandKind::ReleaseCheck:
      case venom::compiler::CommandKind::VerifyRuntime:
        return venom::compiler::release_check(command.release_check) ? 0 : 30;
      case venom::compiler::CommandKind::Doctor:
        return venom::compiler::run_doctor(command.doctor) ? 0 : 50;
      case venom::compiler::CommandKind::Compatibility:
        return venom::compiler::run_compatibility_check(command.compatibility) ? 0 : 20;
      case venom::compiler::CommandKind::Analyze:
        venom::compiler::run_capability_analysis(command.compatibility);
        return 0;
      case venom::compiler::CommandKind::Plan:
        return venom::compiler::run_protection_plan(command.planner) ? 0 : 22;
      case venom::compiler::CommandKind::AnalyzeDist:
        return venom::compiler::analyze_distribution({command.analyze_dist_input, command.analyze_dist_format}) ? 0 : 21;
      case venom::compiler::CommandKind::Contracts:
        venom::compiler::print_contracts(command.contracts.format);
        return 0;
      case venom::compiler::CommandKind::NewProject:
        return venom::compiler::create_project({command.project.directory, command.project.force}) ? 0 : 80;
      case venom::compiler::CommandKind::InitProject:
        return venom::compiler::initialize_project({command.project.directory, command.project.force}) ? 0 : 80;
      case venom::compiler::CommandKind::Runtime:
        return venom::compiler::manage_runtime({command.runtime.action, command.runtime.directory, command.runtime.force}) ? 0 : 81;
      case venom::compiler::CommandKind::Update:
        return venom::compiler::manage_update({command.update.action, command.update.channel, command.update.manifest, command.update.prefix, command.update.public_key, command.update.require_signature, command.update.yes, command.update.dry_run, command.update.format == venom::compiler::OutputFormat::Json ? "json" : "text"}, argv[0]) ? 0 : 83;
      case venom::compiler::CommandKind::Config: {
        auto path = command.config.file;
        if (path.empty()) path = venom::compiler::discover_project_config(std::filesystem::current_path());
        if (path.empty()) throw std::runtime_error("no venom.toml found");
        if (command.config.action == "validate") {
          std::string error;
          const bool ok = venom::compiler::validate_project_config(path, &error);
          if (command.config.format == venom::compiler::OutputFormat::Json) {
            std::cout << "{\"valid\":" << (ok ? "true" : "false") << ",\"file\":\"" << std::filesystem::absolute(path).generic_string() << "\"";
            if (!ok) std::cout << ",\"error\":\"" << error << "\"";
            std::cout << "}\n";
          } else {
            std::cout << (ok ? "[PASS] " : "[FAIL] ") << std::filesystem::absolute(path).string() << "\n";
            if (!ok) std::cout << "       " << error << "\n";
          }
          return ok ? 0 : 82;
        }
        if (command.config.action == "print") { venom::compiler::print_project_config(path, command.config.format); return 0; }
        throw std::runtime_error("config action must be validate or print");
      }
    }
  } catch (const std::exception& ex) {
    std::cerr << "venom: " << ex.what() << '\n';
    return 10;
  }
  return 70;
}
