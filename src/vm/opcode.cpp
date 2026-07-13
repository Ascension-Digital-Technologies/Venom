#include "vm/opcode.hpp"

namespace venom::vm {

std::string opcode_name(LogicalOpcode opcode) {
  switch (opcode) {
    case LogicalOpcode::Nop: return "nop";
    case LogicalOpcode::LoadConst: return "load_const";
    case LogicalOpcode::CreateElement: return "create_element";
    case LogicalOpcode::SetAttribute: return "set_attribute";
    case LogicalOpcode::SetText: return "set_text";
    case LogicalOpcode::AppendChild: return "append_child";
    case LogicalOpcode::EnterElement: return "enter_element";
    case LogicalOpcode::LeaveElement: return "leave_element";
    case LogicalOpcode::CallQuickJs: return "call_quickjs";
    case LogicalOpcode::Halt: return "halt";
  }
  return "unknown";
}

std::uint16_t opcode_index(LogicalOpcode opcode) {
  return static_cast<std::uint16_t>(opcode);
}

} // namespace venom::vm
