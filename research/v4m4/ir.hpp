#pragma once
#include "../v4m3/vm.hpp"
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace venom::v4m4 {

// Research subset: explicit uint32 effects, not full JavaScript semantics.
// Each SSA value 1..14 is globally single-assignment and directly maps to a
// VM register (r15 is reserved for loop phi copy cycles). There is no register allocator.
enum class Kind : std::uint8_t { Literal, Add, Sub, Xor, HostCall };

struct Value {
  std::uint8_t id = 0;
  Kind kind = Kind::Literal;
  std::uint8_t a = 0;
  std::uint8_t b = 0;
  std::uint32_t immediate = 0; // Literal or HostCall capability
};
struct Phi {
  std::uint8_t id = 0;
  std::vector<std::pair<std::size_t, std::uint8_t>> incoming;
};
struct Terminator {
  enum class Kind : std::uint8_t { Return, Jump, Branch } kind = Kind::Return;
  std::uint8_t value = 0; // return value or branch predicate; zero for Jump
  std::size_t yes = 0;
  std::size_t no = 0;
};
struct Block {
  std::vector<Phi> phis;
  std::vector<Value> values;
  Terminator end;
};
struct Module { std::vector<Block> blocks; };

// Reachability, cyclic CFG fixed-point dominance, predecessor-complete phi, definition,
// dominance, and operand/effect validation. Rejects before code generation.
void verify(const Module& module);

// Lower to build-agnostic logical bytecode. Join phi moves go into edge stubs.
v4m3::Program lower(const Module& module);

// Independent reference evaluator for differential execution testing.
std::uint32_t interpret(const Module& module, std::uint64_t fuel = 100000,
                        const v4m3::HostPolicy& host = {});
} // namespace venom::v4m4
