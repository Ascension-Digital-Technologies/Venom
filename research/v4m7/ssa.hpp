#pragma once
#include "../v4m3/vm.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace venom::v4m7 {

// Register-constrained SSA bridge. Every write has a unique virtual value ID;
// each non-entry CFG block has 16 explicit entry phis. Physical slots remain
// r0..r15, so phis are coalesced by slot and need no edge moves in the backend.
// This is not an AST-level SSA compiler, general register allocator or spill pass.
struct Phi {
  std::uint32_t result = 0;
  std::uint8_t slot = 0;
  std::vector<std::pair<std::size_t, std::uint32_t>> incoming;
};
struct Step {
  v4m3::Instruction ins{};
  std::uint32_t result = 0;
  std::uint32_t read_a = 0;
  std::uint32_t read_b = 0;
};
struct End {
  enum class Kind : std::uint8_t { Return, Jump, Branch } kind = Kind::Return;
  std::size_t yes = 0;
  std::size_t no = 0;
  std::uint8_t source_reg = 0;
  std::uint32_t source_value = 0;
};
struct Block {
  std::size_t original_pc = 0;
  std::vector<Phi> phis;
  std::vector<Step> steps;
  End end{};
  std::array<std::uint32_t, 16> out{};
};
struct Module {
  std::vector<Block> blocks;
  std::uint32_t value_count = 0;
};

Module lift(const v4m3::Program& logical_program);
void verify(const Module& module);
v4m3::Program lower(const Module& module);
std::uint32_t interpret(const Module& module, std::uint64_t fuel = 200000,
                        const v4m3::HostPolicy& host = {});

} // namespace venom::v4m7
