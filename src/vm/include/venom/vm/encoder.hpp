#pragma once

#include "venom/vm/bytecode.hpp"
#include "venom/vm/polymorph.hpp"

#include <vector>

namespace venom::vm {

std::vector<unsigned char> encode_program(const Program& program, const PolymorphicPlan& plan);

} // namespace venom::vm
