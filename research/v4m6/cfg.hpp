#pragma once
#include "../v4m3/vm.hpp"

namespace venom::v4m6 {

// Source-compiler only: reject uninitialized registers, malformed edges and
// writes to reserved r0 on all reachable paths, including loop backedges.
// The bytecode decoder independently checks structural operand bounds.
void verify_compiled_cfg(const v4m3::Program& program);

} // namespace venom::v4m6
