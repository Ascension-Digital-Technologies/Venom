#include "ssa.hpp"
#include "../v4m6/cfg.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace venom::v4m7 {
namespace {
using v4m3::Instruction;
using v4m3::Op;
using v4m3::Program;

constexpr std::size_t kMaxBlocks = 512;
constexpr std::size_t kMaxInstructions = 65536;
constexpr std::uint32_t kMaxValues = 80000;

void require(bool yes, const char* message) {
  if (!yes) throw std::runtime_error(message);
}
bool reads_a(Op op) {
  switch (op) {
    case Op::Move: case Op::HostCall: case Op::Jnz: case Op::Halt:
    case Op::Add: case Op::Sub: case Op::Xor:
    case Op::AddChecked: case Op::SubChecked: case Op::MulChecked:
    case Op::Equal: case Op::Less: case Op::LessEqual:
    case Op::Greater: case Op::GreaterEqual: return true;
    case Op::Const: return false;
    default: throw std::runtime_error("SSA bridge: unknown operation");
  }
}
bool reads_b(Op op) {
  switch (op) {
    case Op::Add: case Op::Sub: case Op::Xor:
    case Op::AddChecked: case Op::SubChecked: case Op::MulChecked:
    case Op::Equal: case Op::Less: case Op::LessEqual:
    case Op::Greater: case Op::GreaterEqual: return true;
    default: return false;
  }
}
bool writes(Op op) { return op != Op::Jnz && op != Op::Halt; }

std::int64_t signed32(std::uint32_t raw) {
  return raw <= 0x7fffffffu ? static_cast<std::int64_t>(raw) :
         static_cast<std::int64_t>(raw) - 0x100000000ll;
}
std::uint32_t checked(std::int64_t value) {
  require(value >= -0x80000000ll && value <= 0x7fffffffll,
          "SSA interpreter int32 overflow");
  return static_cast<std::uint32_t>(value);
}
void successors(const End& e, const std::size_t count,
                std::vector<std::size_t>& next) {
  if (e.kind == End::Kind::Return) return;
  require(e.yes < count && e.yes != 0, "SSA invalid jump target");
  next.push_back(e.yes);
  if (e.kind == End::Kind::Branch) {
    require(e.no < count && e.no != 0 && e.no != e.yes,
            "SSA invalid branch targets");
    next.push_back(e.no);
  } else {
    require(e.kind == End::Kind::Jump, "SSA unknown terminator");
  }
}
struct Segment {
  std::size_t begin = 0;
  std::size_t end = 0;
  End terminator{};
};
} // namespace

Module lift(const Program& code) {
  v4m6::verify_compiled_cfg(code);
  require(!code.empty() && code.size() <= kMaxInstructions,
          "SSA source instruction limit");
  const std::size_t n = code.size();
  std::vector<bool> leader(n, false);
  leader[0] = true;
  for (std::size_t pc = 0; pc < n; ++pc) {
    if (code[pc].op == Op::Jnz) {
      require(code[pc].b < n, "SSA jump out of bounds");
      leader[code[pc].b] = true;
      if (pc + 1 < n) leader[pc + 1] = true;
    } else if (code[pc].op == Op::Halt && pc + 1 < n) {
      leader[pc + 1] = true;
    }
  }
  std::vector<Segment> segments;
  std::vector<std::size_t> starts;
  for (std::size_t pc = 0; pc < n; ++pc) if (leader[pc]) starts.push_back(pc);
  require(starts.size() <= kMaxBlocks, "SSA basic-block budget exceeded");
  std::vector<std::size_t> pc_block(n);
  for (std::size_t bi = 0; bi < starts.size(); ++bi) {
    const std::size_t stop = bi + 1 < starts.size() ? starts[bi + 1] : n;
    require(stop > starts[bi], "SSA empty segment");
    segments.push_back({starts[bi], stop, {}});
    for (std::size_t pc = starts[bi]; pc < stop; ++pc) pc_block[pc] = bi;
  }
  for (std::size_t bi = 0; bi < segments.size(); ++bi) {
    auto& seg = segments[bi];
    const std::size_t tail_pc = seg.end - 1;
    const auto& tail = code[tail_pc];
    if (tail.op == Op::Halt) {
      seg.terminator = {End::Kind::Return, 0, 0,
                        static_cast<std::uint8_t>(tail.a), 0};
    } else if (tail.op == Op::Jnz) {
      const std::size_t target = pc_block[tail.b];
      if (tail.a == 0 || (tail_pc + 1 < n && target == pc_block[tail_pc + 1])) {
        seg.terminator = {End::Kind::Jump, target, 0, 0, 0};
      } else {
        require(tail_pc + 1 < n, "SSA branch missing false edge");
        seg.terminator = {End::Kind::Branch, target, pc_block[tail_pc + 1],
                          static_cast<std::uint8_t>(tail.a), 0};
      }
    } else {
      require(seg.end < n, "SSA trailing block missing terminator");
      seg.terminator = {End::Kind::Jump, pc_block[seg.end], 0, 0, 0};
    }
  }
  // Drop unreachable sentinel blocks and out-of-path dead segments.
  std::vector<bool> reachable(segments.size(), false);
  std::vector<std::size_t> visit{0};
  reachable[0] = true;
  for (std::size_t i = 0; i < visit.size(); ++i) {
    std::vector<std::size_t> next;
    successors(segments[visit[i]].terminator, segments.size(), next);
    for (const auto dest : next) {
      if (!reachable[dest]) { reachable[dest] = true; visit.push_back(dest); }
    }
  }
  std::vector<std::size_t> old_to_new(segments.size(), segments.size());
  Module module;
  for (std::size_t old = 0; old < segments.size(); ++old) {
    if (reachable[old]) {
      old_to_new[old] = module.blocks.size();
      module.blocks.push_back({});
    }
  }
  require(module.blocks.size() <= kMaxBlocks, "SSA reachable block budget exceeded");
  auto id = std::uint32_t{16}; // 1..16 are implicit zero-initialized input register values.
  for (std::size_t old = 0; old < segments.size(); ++old) {
    if (!reachable[old]) continue;
    auto& block = module.blocks[old_to_new[old]];
    const auto& seg = segments[old];
    block.original_pc = seg.begin;
    std::array<std::uint32_t, 16> state{};
    for (std::size_t r = 0; r < state.size(); ++r) state[r] = static_cast<std::uint32_t>(r + 1);
    if (old != 0) {
      for (std::size_t r = 0; r < state.size(); ++r) {
        const auto result = ++id;
        block.phis.push_back({result, static_cast<std::uint8_t>(r), {}});
        state[r] = result;
      }
    }
    const auto& tail = code[seg.end - 1];
    const std::size_t statement_end = (tail.op == Op::Jnz || tail.op == Op::Halt)
      ? seg.end - 1 : seg.end;
    for (std::size_t pc = seg.begin; pc < statement_end; ++pc) {
      const auto ins = code[pc];
      require(writes(ins.op), "SSA unexpected control instruction inside basic block");
      Step step{};
      step.ins = ins;
      if (reads_a(ins.op)) step.read_a = state[ins.a];
      if (reads_b(ins.op)) step.read_b = state[ins.b];
      step.result = ++id;
      require(id <= kMaxValues, "SSA virtual value budget exceeded");
      state[ins.dst] = step.result;
      block.steps.push_back(step);
    }
    block.out = state;
    block.end = seg.terminator;
    if (block.end.kind != End::Kind::Return) {
      block.end.yes = old_to_new[block.end.yes];
      if (block.end.kind == End::Kind::Branch)
        block.end.no = old_to_new[block.end.no];
    }
    if (block.end.kind != End::Kind::Jump)
      block.end.source_value = state[block.end.source_reg];
  }
  std::vector<std::vector<std::size_t>> preds(module.blocks.size());
  for (std::size_t bi = 0; bi < module.blocks.size(); ++bi) {
    std::vector<std::size_t> next;
    successors(module.blocks[bi].end, module.blocks.size(), next);
    for (const auto dest : next) preds[dest].push_back(bi);
  }
  for (std::size_t bi = 1; bi < module.blocks.size(); ++bi) {
    for (auto& phi : module.blocks[bi].phis) {
      for (const auto pred : preds[bi])
        phi.incoming.push_back({pred, module.blocks[pred].out[phi.slot]});
    }
  }
  module.value_count = id;
  verify(module);
  return module;
}

void verify(const Module& module) {
  const std::size_t n = module.blocks.size();
  require(n > 0 && n <= kMaxBlocks, "SSA invalid block count");
  require(module.value_count >= 16 && module.value_count <= kMaxValues,
          "SSA invalid value count");
  require(module.blocks[0].original_pc == 0 && module.blocks[0].phis.empty(),
          "SSA invalid entry block");
  std::vector<std::vector<std::size_t>> preds(n);
  std::vector<std::uint32_t> definitions(module.value_count + 1, 0);
  std::vector<std::uint8_t> value_slot(module.value_count + 1, 0);
  for (std::uint32_t v = 1; v <= 16; ++v) {
    definitions[v] = 1;
    value_slot[v] = static_cast<std::uint8_t>(v - 1);
  }
  std::size_t steps = 0;
  for (std::size_t bi = 0; bi < n; ++bi) {
    const auto& block = module.blocks[bi];
    if (bi != 0) {
      require(block.original_pc > module.blocks[bi-1].original_pc,
              "SSA block ordering invalid");
      require(block.phis.size() == 16, "SSA incomplete block register phis");
    }
    for (std::size_t reg = 0; reg < block.phis.size(); ++reg) {
      const auto& phi = block.phis[reg];
      require(phi.slot == reg && phi.result > 16 &&
              phi.result <= module.value_count, "SSA malformed phi");
      require(++definitions[phi.result] == 1, "SSA duplicate value definition");
      value_slot[phi.result] = phi.slot;
    }
    for (const auto& step : block.steps) {
      ++steps;
      require(steps <= kMaxInstructions, "SSA instruction budget exceeded");
      const auto& ins = step.ins;
      require(writes(ins.op) && ins.dst < 16, "SSA invalid step");
      require(ins.op != Op::Const || ins.a == 0, "SSA malformed constant");
      require(!reads_a(ins.op) || ins.a < 16, "SSA invalid operand a");
      require(!reads_b(ins.op) || ins.b < 16, "SSA invalid operand b");
      if (ins.op == Op::HostCall) require(ins.b != 0, "SSA host capability zero");
      if (ins.op == Op::Move) require(ins.b == 0, "SSA malformed move");
      if (ins.op == Op::Const || ins.op == Op::Move) {
        require(ins.dst != 0 || (bi == 0 && &step == &block.steps.front() &&
                ins.op == Op::Const && ins.b == 1),
                "SSA unauthorized write to sentinel register");
      } else {
        require(ins.dst != 0, "SSA unauthorized write to sentinel register");
      }
      require(step.result > 16 && step.result <= module.value_count,
              "SSA invalid value definition ID");
      require(++definitions[step.result] == 1, "SSA duplicate value definition");
      value_slot[step.result] = static_cast<std::uint8_t>(ins.dst);
    }
    std::vector<std::size_t> next;
    successors(block.end, n, next);
    for (const auto dest : next) preds[dest].push_back(bi);
  }
  for (std::uint32_t v = 1; v <= module.value_count; ++v)
    require(definitions[v] == 1, "SSA missing or duplicated virtual value");
  require(preds[0].empty(), "SSA backedge into entry");
  // Enforce all blocks reachable, including when an attacker inserts a
  // detached cycle whose predecessor list is nonempty.
  std::vector<bool> reachable(n, false);
  std::vector<std::size_t> queue{0};
  reachable[0] = true;
  for (std::size_t q = 0; q < queue.size(); ++q) {
    std::vector<std::size_t> next;
    successors(module.blocks[queue[q]].end, n, next);
    for (const auto dest : next) if (!reachable[dest]) {
      reachable[dest] = true;
      queue.push_back(dest);
    }
  }
  for (const auto yes : reachable) require(yes, "SSA detached CFG block");

  for (std::size_t bi = 0; bi < n; ++bi) {
    const auto& block = module.blocks[bi];
    std::array<std::uint32_t, 16> state{};
    for (std::size_t reg = 0; reg < state.size(); ++reg)
      state[reg] = bi == 0 ? static_cast<std::uint32_t>(reg + 1) :
                           block.phis[reg].result;
    if (bi != 0) {
      for (const auto& phi : block.phis) {
        require(phi.incoming.size() == preds[bi].size(),
                "SSA phi predecessor count mismatch");
        for (std::size_t idx = 0; idx < preds[bi].size(); ++idx) {
          const auto& incoming = phi.incoming[idx];
          require(incoming.first == preds[bi][idx], "SSA wrong phi predecessor");
          const auto expected = module.blocks[incoming.first].out[phi.slot];
          require(incoming.second == expected &&
                  incoming.second <= module.value_count &&
                  value_slot[incoming.second] == phi.slot,
                  "SSA wrong phi input");
        }
      }
    }
    for (const auto& step : block.steps) {
      const auto& ins = step.ins;
      const auto expected_a = reads_a(ins.op) ? state[ins.a] : 0u;
      const auto expected_b = reads_b(ins.op) ? state[ins.b] : 0u;
      require(step.read_a == expected_a && step.read_b == expected_b,
              "SSA step reads do not match reaching definitions");
      state[ins.dst] = step.result;
    }
    require(state == block.out, "SSA block output state mismatch");
    const auto& end = block.end;
    if (end.kind != End::Kind::Jump) {
      require(end.source_reg < 16 &&
              end.source_value == state[end.source_reg],
              "SSA terminator reads wrong value");
    }
  }
}

v4m3::Program lower(const Module& module) {
  verify(module);
  Program result;
  struct Patch { std::size_t at; std::size_t target; };
  std::vector<Patch> patches;
  std::vector<std::size_t> starts(module.blocks.size());
  for (std::size_t bi = 0; bi < module.blocks.size(); ++bi) {
    const auto& block = module.blocks[bi];
    starts[bi] = result.size();
    for (const auto& step : block.steps) result.push_back(step.ins);
    switch (block.end.kind) {
      case End::Kind::Return:
        result.push_back({Op::Halt, 0, block.end.source_reg, 0});
        break;
      case End::Kind::Jump:
        patches.push_back({result.size(),block.end.yes});
        result.push_back({Op::Jnz,0,0,0});
        break;
      case End::Kind::Branch:
        patches.push_back({result.size(),block.end.yes});
        result.push_back({Op::Jnz,0,block.end.source_reg,0});
        patches.push_back({result.size(),block.end.no});
        result.push_back({Op::Jnz,0,0,0});
        break;
      default: throw std::runtime_error("SSA lowering unknown terminator");
    }
  }
  result.push_back({Op::Halt,0,0,0});
  require(result.size() <= kMaxInstructions, "SSA backend instruction limit");
  for (const auto& patch : patches) {
    require(patch.target < starts.size(), "SSA backend bad jump");
    result[patch.at].b = static_cast<std::uint32_t>(starts[patch.target]);
  }
  v4m6::verify_compiled_cfg(result);
  return result;
}

std::uint32_t interpret(const Module& module, std::uint64_t fuel,
                        const v4m3::HostPolicy& host) {
  // Re-lowering also applies the existing register-level definite-assignment
  // verifier to prevent interpreting a manually corrupted SSA module.
  (void)lower(module);
  std::vector<std::uint32_t> values(module.value_count + 1, 0);
  std::size_t current = 0;
  std::size_t previous = module.blocks.size();
  std::uint64_t remaining_calls = host.max_calls;
  auto consume = [&]() {
    require(fuel != 0, "SSA interpreter fuel exhausted");
    --fuel;
  };
  for (;;) {
    const auto& block = module.blocks[current];
    if (current != 0) {
      // Phi reads must observe the predecessor's *entire* register state.
      // Stage results first: loop-carried phis can swap values across an edge.
      std::vector<std::pair<std::uint32_t, std::uint32_t>> staged;
      staged.reserve(block.phis.size());
      for (const auto& phi : block.phis) {
        bool found = false;
        for (const auto& incoming : phi.incoming) {
          if (incoming.first == previous) {
            staged.push_back({phi.result, values[incoming.second]});
            found = true;
            break;
          }
        }
        require(found, "SSA runtime missing incoming edge");
      }
      for (const auto& item : staged) values[item.first] = item.second;
    }
    for (const auto& step : block.steps) {
      consume();
      const auto& ins = step.ins;
      const auto a = values[step.read_a];
      const auto b = values[step.read_b];
      std::uint32_t output = 0;
      switch (ins.op) {
        case Op::Const: output = ins.b; break;
        case Op::Move: output = a; break;
        case Op::Add: output = a+b; break;
        case Op::Sub: output = a-b; break;
        case Op::Xor: output = a^b; break;
        case Op::AddChecked: output = checked(signed32(a)+signed32(b)); break;
        case Op::SubChecked: output = checked(signed32(a)-signed32(b)); break;
        case Op::MulChecked: output = checked(signed32(a)*signed32(b)); break;
        case Op::Equal: output = a==b ? 1u : 0u; break;
        case Op::Less: output = signed32(a)<signed32(b) ? 1u : 0u; break;
        case Op::LessEqual: output = signed32(a)<=signed32(b) ? 1u : 0u; break;
        case Op::Greater: output = signed32(a)>signed32(b) ? 1u : 0u; break;
        case Op::GreaterEqual: output = signed32(a)>=signed32(b) ? 1u : 0u; break;
        case Op::HostCall:
          require(std::find(host.allowed.begin(),host.allowed.end(),ins.b) != host.allowed.end(),
                  "SSA host capability denied");
          require(static_cast<bool>(host.invoke), "SSA host callback unavailable");
          require(remaining_calls > 0, "SSA host budget exhausted");
          --remaining_calls;
          output = host.invoke(ins.b,a);
          break;
        default: throw std::runtime_error("SSA interpreter unknown opcode");
      }
      values[step.result] = output;
    }
    consume();
    const auto& end = block.end;
    if (end.kind == End::Kind::Return) return values[end.source_value];
    previous = current;
    if (end.kind == End::Kind::Jump) current = end.yes;
    else if (end.kind == End::Kind::Branch)
      current = values[end.source_value] ? end.yes : end.no;
    else throw std::runtime_error("SSA interpreter unknown terminator");
  }
}

} // namespace venom::v4m7
