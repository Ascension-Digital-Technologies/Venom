#include "venom/core/diagnostic.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <vector>

namespace venom::compiler {
namespace {

std::string source_line(const SourceLocation& location) {
  if (!location.source_line.empty()) return location.source_line;
  if (location.file.empty() || location.line == 0) return {};
  std::ifstream input(location.file);
  if (!input) return {};
  std::string line;
  for (std::size_t current = 1; current <= location.line; ++current) {
    if (!std::getline(input, line)) return {};
  }
  return line;
}

std::size_t digits(std::size_t value) {
  std::size_t width = 1;
  while (value >= 10) { value /= 10; ++width; }
  return width;
}

} // namespace

DiagnosticError::DiagnosticError(std::string code, std::string message,
                                 SourceLocation location, std::string detail,
                                 std::string help, std::string docs)
    : Error(std::move(code), std::move(message)),
      location_(std::move(location)), detail_(std::move(detail)),
      help_(std::move(help)), docs_(std::move(docs)) {}

[[noreturn]] void raise_diagnostic(std::string code, std::string message,
                                   SourceLocation location, std::string detail,
                                   std::string help, std::string docs) {
  throw DiagnosticError(std::move(code), std::move(message), std::move(location),
                        std::move(detail), std::move(help), std::move(docs));
}

void print_diagnostic(std::ostream& out, const DiagnosticError& diagnostic) {
  out << "error[" << diagnostic.code() << "]: " << diagnostic.what() << '\n';
  const auto& location = diagnostic.location();
  if (!location.file.empty()) {
    out << "  --> " << location.file;
    if (location.line) out << ':' << location.line << ':' << std::max<std::size_t>(1, location.column);
    out << '\n';
    const auto text = source_line(location);
    if (!text.empty() && location.line) {
      const auto width = digits(location.line);
      out << std::string(width + 1, ' ') << " |\n";
      out << std::setw(static_cast<int>(width)) << location.line << " | " << text << '\n';
      const auto column = std::max<std::size_t>(1, location.column);
      out << std::string(width + 1, ' ') << " | " << std::string(column - 1, ' ') << '^' << '\n';
    }
  }
  if (!diagnostic.detail().empty()) out << "  = detail: " << diagnostic.detail() << '\n';
  if (!diagnostic.help().empty()) out << "  = help: " << diagnostic.help() << '\n';
  if (!diagnostic.docs().empty()) out << "  = docs: " << diagnostic.docs() << '\n';
}

} // namespace venom::compiler
