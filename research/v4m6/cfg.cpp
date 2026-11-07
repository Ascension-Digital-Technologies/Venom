#include "cfg.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace venom::v4m6 {
namespace {
using v4m3::Op;
void check(bool yes, const char* message) {
  if (!yes) throw std::runtime_error(message);
}
}

void verify_compiled_cfg(const v4m3::Program& p) {
  check(!p.empty() && p.size() <= 65536, "compiled CFG instruction count invalid");
  check(p[0].op == Op::Const && p[0].dst == 0 && p[0].a == 0 && p[0].b == 1,
        "compiled CFG missing unconditional-branch sentinel");
  const auto n = p.size();
  std::vector<std::vector<std::size_t>> next(n), prev(n);
  for (std::size_t pc = 0; pc < n; ++pc) {
    const auto& ins = p[pc];
    const bool control = ins.op == Op::Jnz || ins.op == Op::Halt;
    if (!control && pc != 0) check(ins.dst != 0 && ins.dst < 16,
                                   "compiled CFG mutated reserved register zero");
    switch (ins.op) {
      case Op::Const: check(ins.a == 0, "compiled CFG malformed Const"); break;
      case Op::Move:
        check(ins.a < 16 && ins.b == 0, "compiled CFG malformed Move"); break;
      case Op::Add: case Op::Sub: case Op::Xor:
      case Op::AddChecked: case Op::SubChecked: case Op::MulChecked:
      case Op::Equal: case Op::Less: case Op::LessEqual:
      case Op::Greater: case Op::GreaterEqual:
        check(ins.a < 16 && ins.b < 16, "compiled CFG bad arithmetic registers"); break;
      case Op::HostCall:
        check(ins.a < 16 && ins.b != 0, "compiled CFG invalid host operation"); break;
      case Op::Jnz:
        check(ins.dst == 0 && ins.a < 16 && ins.b < n, "compiled CFG invalid jump"); break;
      case Op::Halt:
        check(ins.dst == 0 && ins.a < 16 && ins.b == 0, "compiled CFG invalid return"); break;
      default: throw std::runtime_error("compiled CFG unknown opcode");
    }
    auto edge = [&](std::size_t dest) {
      check(dest < n, "compiled CFG branch falls out of program");
      if (std::find(next[pc].begin(), next[pc].end(), dest) == next[pc].end()) {
        next[pc].push_back(dest);
        prev[dest].push_back(pc);
      }
    };
    if (ins.op == Op::Jnz) {
      edge(ins.b);
      // r0 is the immutable truth sentinel, so a Jnz r0 is unconditional.
      if (ins.a != 0) edge(pc + 1);
    } else if (ins.op != Op::Halt) {
      edge(pc + 1);
    }
  }

  std::vector<bool> reachable(n, false);
  reachable[0] = true;
  std::vector<std::size_t> queue{0};
  for (std::size_t q = 0; q < queue.size(); ++q) {
    for (const auto dest : next[queue[q]]) {
      if (!reachable[dest]) { reachable[dest] = true; queue.push_back(dest); }
    }
  }

  // Definite-assignment dataflow: intersection of predecessor OUT sets.
  // Initialized to top for non-entry nodes, converges through loop cycles.
  std::vector<std::uint16_t> in(n, 0xffffu), out(n, 0xffffu);
  in[0] = 0;
  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto pc : queue) {
      std::uint16_t incoming = 0;
      if (pc != 0) {
        incoming = 0xffffu;
        for (const auto from : prev[pc]) {
          if (reachable[from]) incoming = static_cast<std::uint16_t>(incoming & out[from]);
        }
      }
      const auto& ins = p[pc];
      const std::uint16_t bit = ins.op == Op::Jnz || ins.op == Op::Halt ?
        std::uint16_t{0} : static_cast<std::uint16_t>(1u << ins.dst);
      const std::uint16_t outgoing = static_cast<std::uint16_t>(incoming | bit);
      if (incoming != in[pc] || outgoing != out[pc]) {
        in[pc] = incoming;
        out[pc] = outgoing;
        changed = true;
      }
    }
  }

  for (const auto pc : queue) {
    const auto& ins = p[pc];
    auto read = [&](std::uint32_t reg) {
      check(reg < 16 && (in[pc] & (1u << reg)) != 0,
            "compiled CFG read uninitialized register");
    };
    switch (ins.op) {
      case Op::Const: break;
      case Op::Move: case Op::HostCall: case Op::Halt: case Op::Jnz:
        read(ins.a);
        break;
      default:
        read(ins.a);
        read(ins.b);
        break;
    }
  }
}
} // namespace venom::v4m6
