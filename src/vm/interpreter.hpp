#pragma once

#include "vm/bytecode.hpp"

namespace venom::vm {

class Interpreter {
public:
  int run(const Program& program);
};

} // namespace venom::vm
