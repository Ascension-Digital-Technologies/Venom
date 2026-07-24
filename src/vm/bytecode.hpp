#pragma once

#include <cstdint>
#include <vector>

namespace venom::vm {

// Logical opcodes are stable compiler concepts. The physical opcode values
// written into a protected package are produced by the polymorphic plan and can
// change per build/profile.
enum class LogicalOpcode : std::uint16_t {
  Nop,
  LoadConst,
  CreateElement,
  SetAttribute,
  SetText,
  AppendChild,
  EnterElement,
  LeaveElement,
  CallTurboJs,
  Halt,
};

struct Instruction {
  LogicalOpcode opcode = LogicalOpcode::Nop;
  std::uint32_t a = 0;
  std::uint32_t b = 0;
  std::uint32_t c = 0;
};

using Program = std::vector<Instruction>;

} // namespace venom::vm
