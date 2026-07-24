#include "core/error_reporting.hpp"

#include "core/console.hpp"

#include <ostream>

namespace venom::compiler {

void print_error(std::ostream& out, const venom::Error& error) {
  const auto context = current_console_context();
  if (context.phase_index != 0) {
    out << "\nerror[" << error.code() << "]: build failed during step "
        << context.phase_index << '/' << context.phase_total << '\n'
        << "  phase: " << context.phase_message << '\n'
        << "  reason: " << error.what() << '\n';
    return;
  }
  out << "error[" << error.code() << "]: " << error.what() << '\n';
}

void print_unexpected_error(std::ostream& out, const std::exception& error) {
  print_error(out, venom::Error("VENOM-E0001", error.what(), venom::ExitCode::user_error));
}

} // namespace venom::compiler
