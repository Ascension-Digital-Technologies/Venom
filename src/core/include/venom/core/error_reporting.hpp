#pragma once

#include "venom/base/error.hpp"

#include <iosfwd>

namespace venom::compiler {

void print_error(std::ostream& out, const venom::Error& error);
void print_unexpected_error(std::ostream& out, const std::exception& error);

} // namespace venom::compiler
