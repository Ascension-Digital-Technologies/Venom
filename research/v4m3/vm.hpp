#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace venom::v4m3 {

// Research ISA. These opcodes are semantic identities, never stored on wire.
enum class Op : std::uint8_t { Const, Move, Add, Sub, Xor, Jnz, Halt, HostCall, Count };

// Const: dst <- b; Move: dst <- reg[a]; Add/Sub/Xor: dst <- reg[a] op reg[b]
// Jnz: if (reg[a] != 0) pc <- b; Halt: return reg[a].
// HostCall: reg[dst] <- capability(b, reg[a]); b must be nonzero.
// All unused fields must be 0.
struct Instruction {
  Op op;
  std::uint32_t dst = 0;
  std::uint32_t a = 0;
  std::uint32_t b = 0;
};
using Program = std::vector<Instruction>;

struct Plan {
  std::array<std::uint8_t, 16> build_id{};
  std::array<std::uint32_t, static_cast<std::size_t>(Op::Count)> opcodes{};
  // Stored field order (0: opcode, 1: dst, 2: a, 3: b).
  std::array<std::uint8_t, 4> field_order{};
  std::array<std::uint32_t, 4> masks{};
};

// One seed per build. Do not embed the seed in the package. No security claim
// relies on this seed remaining secret on an attacker-controlled client.
Plan make_plan(std::uint64_t build_seed);

// V4-M4 format revision 4 is incompatible with V4-M3: HostCall adds an ISA opcode.
// Versioned, build-bound, SHA-256 checked bytecode envelope.
// Both encoder and decoder validate all register, opcode and branch operands.
std::vector<std::uint8_t> encode(const Program& program, const Plan& plan);
Program decode(const std::vector<std::uint8_t>& bytes, const Plan& plan);

// Host access is opt-in. No capability is allowed by default.
// Callback receives (capability ID, uint32 argument); 0 calls means denied.
struct HostPolicy {
  std::vector<std::uint32_t> allowed;
  std::function<std::uint32_t(std::uint32_t, std::uint32_t)> invoke;
  std::uint64_t max_calls = 0;
};

// 16-register uint32 reference VM with modulo arithmetic and fuel.
std::uint32_t run(const Program& program, std::uint64_t fuel = 100000,
                  const HostPolicy& host = {});
std::uint32_t run_package(const std::vector<std::uint8_t>& bytes, const Plan& plan,
                          std::uint64_t fuel = 100000, const HostPolicy& host = {});

} // namespace venom::v4m3
