#include "compiler/build.hpp"
#include "compiler/cli.hpp"
#include "compiler/inspect.hpp"
#include "compiler/doctor.hpp"
#include "compiler/compatibility.hpp"
#include "compiler/security.hpp"

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
      case venom::compiler::CommandKind::Contracts:
        venom::compiler::print_contracts(command.contracts.format);
        return 0;
    }
  } catch (const std::exception& ex) {
    std::cerr << "venom: " << ex.what() << '\n';
    return 10;
  }
  return 70;
}
