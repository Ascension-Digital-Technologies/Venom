#include "base/error.hpp"
#include "core/error_reporting.hpp"
#include "pipeline/build.hpp"
#include "cli/cli.hpp"
#include "cli/inspect.hpp"
#include "cli/doctor.hpp"
#include "cli/dev.hpp"
#include "pipeline/compatibility.hpp"
#include "core/diagnostic.hpp"
#include "cli/dist_analyzer.hpp"
#include "pipeline/security.hpp"
#include "core/project.hpp"
#include "runtime/manager.hpp"
#include "cli/update_manager.hpp"
#include "core/config.hpp"
#include "pipeline/planner.hpp"
#include "core/console.hpp"
#include "cli/compile_snippet.hpp"
#include "pipeline/native_js_hardener.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  try {
    const auto executable = std::filesystem::absolute(argv[0]);
#ifdef _WIN32
    const auto hardener_worker = executable.parent_path() / "venom_hardener_worker.exe";
#else
    const auto hardener_worker = executable.parent_path() / "venom_hardener_worker";
#endif
    venom::compiler::native_js_hardener::configure_worker_executable(hardener_worker.string());
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
      case venom::compiler::CommandKind::Verify:
      case venom::compiler::CommandKind::VerifyRuntime:
        return venom::compiler::release_check(command.release_check) ? 0 : 30;
      case venom::compiler::CommandKind::Doctor:
        return venom::compiler::run_doctor(command.doctor) ? 0 : 50;
      case venom::compiler::CommandKind::Compatibility:
        return venom::compiler::run_compatibility_check(command.compatibility) ? 0 : 20;
      case venom::compiler::CommandKind::Plan:
        return venom::compiler::run_protection_plan(command.planner) ? 0 : 22;
      case venom::compiler::CommandKind::CompileSnippet:
        return venom::compiler::compile_snippet(std::cin, std::cout) ? 0 : 23;
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
        if (path.empty()) venom::raise_error("VENOM-E1000", "no venom.toml found");
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
        venom::raise_error("VENOM-E1000", "config action must be validate or print");
      }
    }
  } catch (const venom::compiler::DiagnosticError& diagnostic) {
    venom::compiler::print_diagnostic(std::cerr, diagnostic);
    return static_cast<int>(diagnostic.exit_code());
  } catch (const venom::Error& error) {
    venom::compiler::print_error(std::cerr, error);
    return static_cast<int>(error.exit_code());
  } catch (const std::exception& error) {
    venom::compiler::print_unexpected_error(std::cerr, error);
    return static_cast<int>(venom::ExitCode::user_error);
  }
  return 70;
}
