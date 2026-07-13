#pragma once

#include "vm/bytecode.hpp"
#include "vm/polymorph.hpp"

#include <vector>

namespace venom::vm {

std::vector<unsigned char> encode_program(const Program& program, const PolymorphicPlan& plan);

} // namespace venom::vm
