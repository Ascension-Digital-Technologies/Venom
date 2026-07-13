#include "vm/interpreter.hpp"

namespace venom::vm {

int Interpreter::run(const Program& program) {
  for (const auto& ins : program) {
    if (ins.opcode == LogicalOpcode::Halt) {
      return 0;
    }
  }
  return 0;
}

} // namespace venom::vm
