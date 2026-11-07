#pragma once

#include "../v4m3/vm.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace venom::v4m8 {

// This is a source-direct, typed SSA graph for straight-line JS expressions
// and local reassignments. Every node is immutable and has a unique value ID
// (its 1-based index in Module::nodes). No VM bytecode is used as input.
enum class Type : std::uint8_t { Number, Boolean };
enum class Kind : std::uint8_t {
  Literal, Add, Sub, Mul, Xor, Equal, Less, LessEqual, Greater, GreaterEqual,
  HostCall
};
struct Node {
  Kind kind = Kind::Literal;
  Type type = Type::Number;
  std::uint32_t a = 0; // SSA value ID of first operand (0 = none)
  std::uint32_t b = 0; // SSA value ID of second operand (0 = none)
  std::uint32_t immediate = 0; // literal bits or host capability ID
};
struct Module {
  std::vector<Node> nodes;
  std::uint32_t result = 0;
  Type result_type = Type::Number;
};

// Source -> typed SSA; accepts a strict, straight-line subset of JavaScript.
// if/else, while, short-circuit ops, nested functions and full JS Number
// semantics are deliberately out of scope; use V4-M7 for those.
Module parse_typed(const std::string& source);

// Refuse malformed SSA: forward references, incompatible types, invalid
// capability calls and broken result references fail before compilation.
void verify(const Module& module);

// Typed SSA -> VM logical instructions using liveness/use-count based
// physical register allocation. Reuses dead input registers. No spilling yet.
v4m3::Program lower(const Module& module);

// Independent, register-free reference evaluator for differential testing.
std::uint32_t interpret(const Module& module,
                        const v4m3::HostPolicy& host = {});

v4m3::Program compile_source_typed(const std::string& source);
} // namespace venom::v4m8
