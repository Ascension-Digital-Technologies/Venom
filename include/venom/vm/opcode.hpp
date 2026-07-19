#pragma once

#include "venom/vm/bytecode.hpp"

#include <cstdint>
#include <string>

namespace venom::vm {

std::string opcode_name(LogicalOpcode opcode);
std::uint16_t opcode_index(LogicalOpcode opcode);

} // namespace venom::vm
