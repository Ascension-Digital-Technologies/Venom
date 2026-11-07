#include "ir.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

namespace venom::v4m4 {
namespace {
constexpr std::size_t kMaxBlocks = 32;
constexpr std::size_t kMaxValues = 14; // r15 reserved for parallel phi copies
constexpr std::size_t kMaxNodes = 4096;

void check(bool condition, const char* error) {
  if (!condition) throw std::runtime_error(error);
}
struct Def { int block = -1; int order = -1; };
struct Verified {
  std::vector<std::vector<std::size_t>> preds;
  std::vector<std::uint32_t> dominance;
  std::array<Def, kMaxValues + 1> definitions{};
};

Verified analyze(const Module& mod) {
  const std::size_t count = mod.blocks.size();
  check(count > 0 && count <= kMaxBlocks, "IR block limit exceeded");
  Verified a;
  a.preds.resize(count);
  a.dominance.resize(count);
  std::size_t nodes = 0;
  for (std::size_t bi = 0; bi < count; ++bi) {
    const auto& block = mod.blocks[bi];
    nodes += block.phis.size() + block.values.size();
    check(nodes <= kMaxNodes, "IR node limit exceeded");
    auto add_target = [&](std::size_t to) {
      check(to < count && to != 0, "IR CFG target invalid or entry backedge");
      check(std::find(a.preds[to].begin(), a.preds[to].end(), bi) == a.preds[to].end(),
            "duplicate IR CFG edge");
      a.preds[to].push_back(bi);
    };
    switch (block.end.kind) {
      case Terminator::Kind::Return:
        check(block.end.yes == 0 && block.end.no == 0, "invalid return targets");
        break;
      case Terminator::Kind::Jump:
        check(block.end.value == 0 && block.end.no == 0, "invalid jump fields");
        add_target(block.end.yes);
        break;
      case Terminator::Kind::Branch:
        check(block.end.yes != block.end.no, "duplicate branch targets");
        add_target(block.end.yes);
        add_target(block.end.no);
        break;
      default: throw std::runtime_error("unknown IR terminator");
    }
    auto define = [&](std::uint8_t id, int order) {
      check(id > 0 && id <= kMaxValues, "IR value/register limit exceeded");
      check(a.definitions[id].block == -1, "duplicate SSA value definition");
      a.definitions[id] = {static_cast<int>(bi), order};
    };
    for (const auto& phi : block.phis) define(phi.id, -1);
    for (std::size_t j = 0; j < block.values.size(); ++j)
      define(block.values[j].id, static_cast<int>(j));
  }
  check(a.preds[0].empty(), "IR entry has predecessors");
  // Reject detached cycles: every block must be reachable from entry.
  std::vector<bool> reachable(count, false);
  reachable[0] = true;
  std::vector<std::size_t> queue{0};
  for (std::size_t q = 0; q < queue.size(); ++q) {
    const std::size_t from = queue[q];
    for (std::size_t to = 1; to < count; ++to) {
      if (std::find(a.preds[to].begin(), a.preds[to].end(), from) != a.preds[to].end() && !reachable[to]) {
        reachable[to] = true;
        queue.push_back(to);
      }
    }
  }
  for (const bool yes : reachable) check(yes, "unreachable IR block");

  // Fixed-point dominators work with loop backedges.
  const std::uint32_t all = count == 32 ? 0xffffffffu : (std::uint32_t{1} << count) - 1u;
  a.dominance[0] = 1u;
  for (std::size_t bi = 1; bi < count; ++bi) a.dominance[bi] = all;
  bool changed = true;
  while (changed) {
    changed = false;
    for (std::size_t bi = 1; bi < count; ++bi) {
      std::uint32_t mask = all;
      for (const auto pred : a.preds[bi]) mask &= a.dominance[pred];
      const std::uint32_t next = mask | (std::uint32_t{1} << bi);
      if (a.dominance[bi] != next) {
        a.dominance[bi] = next;
        changed = true;
      }
    }
  }
  auto use = [&](std::uint8_t id, std::size_t at, int order) {
    check(id > 0 && id <= kMaxValues, "invalid IR operand ID");
    const auto def = a.definitions[id];
    check(def.block != -1, "undefined IR SSA value");
    if (static_cast<std::size_t>(def.block) == at) {
      check(def.order < order, "IR use before definition");
    } else {
      check((a.dominance[at] & (1u << static_cast<unsigned>(def.block))) != 0,
            "IR operand does not dominate use");
    }
  };
  for (std::size_t bi = 0; bi < count; ++bi) {
    const auto& block = mod.blocks[bi];
    for (const auto& phi : block.phis) {
      check(!a.preds[bi].empty(), "entry phi has no incoming edges");
      check(phi.incoming.size() == a.preds[bi].size(),
            "IR phi predecessor count mismatch");
      std::vector<std::size_t> seen;
      for (const auto& [pred, id] : phi.incoming) {
        check(std::find(a.preds[bi].begin(), a.preds[bi].end(), pred) !=
              a.preds[bi].end(), "IR phi predecessor invalid");
        check(std::find(seen.begin(), seen.end(), pred) == seen.end(),
              "IR phi predecessor duplicated");
        seen.push_back(pred);
        use(id, pred, static_cast<int>(mod.blocks[pred].values.size()));
      }
    }
    for (std::size_t j = 0; j < block.values.size(); ++j) {
      const auto& value = block.values[j];
      const int order = static_cast<int>(j);
      switch (value.kind) {
        case Kind::Literal:
          check(value.a == 0 && value.b == 0, "invalid literal IR fields");
          break;
        case Kind::Add: case Kind::Sub: case Kind::Xor:
          check(value.immediate == 0, "invalid arithmetic IR fields");
          use(value.a, bi, order);
          use(value.b, bi, order);
          break;
        case Kind::HostCall:
          check(value.immediate != 0 && value.b == 0,
                "invalid effect capability IR fields");
          use(value.a, bi, order);
          break;
        default: throw std::runtime_error("unsupported IR operation");
      }
    }
    if (block.end.kind == Terminator::Kind::Return ||
        block.end.kind == Terminator::Kind::Branch)
      use(block.end.value, bi, static_cast<int>(block.values.size()));
  }
  return a;
}

std::uint8_t phi_source(const Phi& phi, std::size_t from) {
  for (const auto& [pred, id] : phi.incoming) if (pred == from) return id;
  throw std::runtime_error("missing edge phi source");
}
} // namespace

void verify(const Module& module) { (void)analyze(module); }

v4m3::Program lower(const Module& mod) {
  verify(mod);
  using v4m3::Op;
  v4m3::Program program;
  // r0 holds true permanently; user SSA values use distinct r1..r15.
  program.push_back({Op::Const, 0, 0, 1});
  struct Edge { std::size_t from; std::size_t to; std::size_t patch; };
  std::vector<Edge> edges;
  std::vector<std::size_t> block_pc(mod.blocks.size());
  for (std::size_t bi = 0; bi < mod.blocks.size(); ++bi) {
    const auto& block = mod.blocks[bi];
    block_pc[bi] = program.size();
    for (const auto& val : block.values) {
      switch (val.kind) {
        case Kind::Literal:
          program.push_back({Op::Const, val.id, 0, val.immediate}); break;
        case Kind::Add:
          program.push_back({Op::Add, val.id, val.a, val.b}); break;
        case Kind::Sub:
          program.push_back({Op::Sub, val.id, val.a, val.b}); break;
        case Kind::Xor:
          program.push_back({Op::Xor, val.id, val.a, val.b}); break;
        case Kind::HostCall:
          program.push_back({Op::HostCall, val.id, val.a, val.immediate}); break;
        default: throw std::runtime_error("unsupported IR node lowering");
      }
    }
    auto edge = [&](std::size_t to, std::uint8_t cond) {
      edges.push_back({bi, to, program.size()});
      program.push_back({Op::Jnz, 0, cond, 0});
    };
    switch (block.end.kind) {
      case Terminator::Kind::Return:
        program.push_back({Op::Halt, 0, block.end.value, 0}); break;
      case Terminator::Kind::Jump:
        edge(block.end.yes, 0); break;
      case Terminator::Kind::Branch:
        edge(block.end.yes, block.end.value);
        edge(block.end.no, 0);
        break;
      default: throw std::runtime_error("unsupported IR terminator lowering");
    }
  }
  for (const auto& edge : edges) {
    // Execute phi assignments in parallel; a loop backedge may need a
    // two-register swap. Register 15 is reserved as the cycle scratch.
    program[edge.patch].b = static_cast<std::uint32_t>(program.size());
    struct Copy { std::uint8_t dst; std::uint8_t src; };
    std::vector<Copy> pending;
    for (const auto& phi : mod.blocks[edge.to].phis) {
      const auto src = phi_source(phi, edge.from);
      if (phi.id != src) pending.push_back({phi.id, src});
    }
    while (!pending.empty()) {
      bool progressed = false;
      for (std::size_t i = 0; i < pending.size(); ++i) {
        const auto dst = pending[i].dst;
        bool will_read = false;
        for (const auto& other : pending) {
          if (other.src == dst) { will_read = true; break; }
        }
        if (!will_read) {
          program.push_back({Op::Move, dst, pending[i].src, 0});
          pending.erase(pending.begin() + static_cast<std::ptrdiff_t>(i));
          progressed = true;
          break;
        }
      }
      if (progressed) continue;
      // Break a cycle: preserve one old destination and rewrite reads.
      const auto saved = pending.front().dst;
      program.push_back({Op::Move, 15, saved, 0});
      for (auto& copy : pending) if (copy.src == saved) copy.src = 15;
    }
    program.push_back({Op::Jnz, 0, 0,
                       static_cast<std::uint32_t>(block_pc[edge.to])});
  }
  // Unreachable sentinel required by bytecode verifier.
  program.push_back({Op::Halt, 0, 0, 0});
  return program;
}

std::uint32_t interpret(const Module& mod, std::uint64_t fuel,
                        const v4m3::HostPolicy& host) {
  verify(mod);
  std::array<std::uint32_t, kMaxValues + 1> values{};
  std::size_t current = 0;
  std::size_t predecessor = mod.blocks.size();
  std::uint64_t remaining_calls = host.max_calls;
  auto step = [&]() {
    check(fuel != 0, "IR execution fuel exhausted");
    --fuel;
  };
  for (;;) {
    const auto& block = mod.blocks[current];
    if (!block.phis.empty()) {
      std::vector<std::pair<std::uint8_t, std::uint32_t>> incoming;
      for (const auto& phi : block.phis)
        incoming.push_back({phi.id, values[phi_source(phi, predecessor)]});
      for (const auto& [id, value] : incoming) { step(); values[id] = value; }
    }
    for (const auto& val : block.values) {
      step();
      switch (val.kind) {
        case Kind::Literal: values[val.id] = val.immediate; break;
        case Kind::Add: values[val.id] = values[val.a] + values[val.b]; break;
        case Kind::Sub: values[val.id] = values[val.a] - values[val.b]; break;
        case Kind::Xor: values[val.id] = values[val.a] ^ values[val.b]; break;
        case Kind::HostCall:
          check(std::find(host.allowed.begin(), host.allowed.end(), val.immediate)
                != host.allowed.end(), "host capability denied");
          check(static_cast<bool>(host.invoke), "no host callback registered");
          check(remaining_calls != 0, "host call budget exhausted");
          --remaining_calls;
          values[val.id] = host.invoke(val.immediate, values[val.a]);
          break;
        default: throw std::runtime_error("unknown IR operation");
      }
    }
    step();
    switch (block.end.kind) {
      case Terminator::Kind::Return: return values[block.end.value];
      case Terminator::Kind::Jump:
        predecessor = current; current = block.end.yes; break;
      case Terminator::Kind::Branch:
        predecessor = current;
        current = values[block.end.value] ? block.end.yes : block.end.no;
        break;
      default: throw std::runtime_error("unknown IR terminator");
    }
  }
}
} // namespace venom::v4m4
