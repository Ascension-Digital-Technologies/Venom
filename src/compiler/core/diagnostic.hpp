#pragma once

#include <cstddef>
#include <iosfwd>
#include <stdexcept>
#include <string>

namespace venom::compiler {

struct SourceLocation {
  std::string file;
  std::size_t line = 0;
  std::size_t column = 0;
  std::string source_line;
};

class DiagnosticError final : public std::runtime_error {
 public:
  DiagnosticError(std::string code, std::string message,
                  SourceLocation location = {}, std::string detail = {},
                  std::string help = {}, std::string docs = {});

  const std::string& code() const noexcept { return code_; }
  const SourceLocation& location() const noexcept { return location_; }
  const std::string& detail() const noexcept { return detail_; }
  const std::string& help() const noexcept { return help_; }
  const std::string& docs() const noexcept { return docs_; }

 private:
  std::string code_;
  SourceLocation location_;
  std::string detail_;
  std::string help_;
  std::string docs_;
};

[[noreturn]] void raise_diagnostic(std::string code, std::string message,
                                   SourceLocation location = {}, std::string detail = {},
                                   std::string help = {}, std::string docs = {});
void print_diagnostic(std::ostream& out, const DiagnosticError& diagnostic);

} // namespace venom::compiler
