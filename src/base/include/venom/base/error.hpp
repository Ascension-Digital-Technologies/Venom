#pragma once

#include <stdexcept>
#include <string>

namespace venom {

enum class ExitCode : int {
  success = 0,
  user_error = 10,
  compatibility = 20,
  analyze = 21,
  plan = 22,
  compile_snippet = 23,
  verify = 30,
  keygen = 40,
  doctor = 50,
  dev = 60,
  build = 70,
  project = 80,
  runtime = 81,
  config = 82,
  update = 83,
};

class Error : public std::runtime_error {
 public:
  Error(std::string code, std::string message,
        ExitCode exit_code = ExitCode::user_error);

  const std::string& code() const noexcept { return code_; }
  ExitCode exit_code() const noexcept { return exit_code_; }

 private:
  std::string code_;
  ExitCode exit_code_;
};

[[noreturn]] void raise_error(std::string code, std::string message,
                              ExitCode exit_code = ExitCode::user_error);

} // namespace venom
