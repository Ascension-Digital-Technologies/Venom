#include "venom/base/error.hpp"

#include <utility>

namespace venom {

Error::Error(std::string code, std::string message, ExitCode exit_code)
    : std::runtime_error(std::move(message)), code_(std::move(code)),
      exit_code_(exit_code) {}

[[noreturn]] void raise_error(std::string code, std::string message,
                              ExitCode exit_code) {
  throw Error(std::move(code), std::move(message), exit_code);
}

} // namespace venom
