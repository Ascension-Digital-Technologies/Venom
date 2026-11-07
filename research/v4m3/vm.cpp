#include "vm.hpp"
#include "package/hash.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

namespace venom::v4m3 {
namespace {
constexpr std::size_t kHeaderSize = 64;
constexpr std::uint32_t kVersion = 3;
constexpr std::size_t kMaxInstructions = 65536;
constexpr std::uint32_t kRegisterCount = 16;
constexpr std::array<std::uint8_t, 8> kMagic{{'V','4','V','M','B','C','0','3'}};

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
void put32(std::vector<std::uint8_t>& out, std::uint32_t x) {
  for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(x >> (i * 8)));
}
std::uint32_t get32(const std::uint8_t* ptr) {
  return std::uint32_t(ptr[0]) | (std::uint32_t(ptr[1]) << 8) |
         (std::uint32_t(ptr[2]) << 16) | (std::uint32_t(ptr[3]) << 24);
}
std::uint32_t next(std::uint32_t& state) {
  state ^= state << 13; state ^= state >> 17; state ^= state << 5;
  return state;
}
std::vector<std::uint8_t> domain_seed(const char* label, std::uint64_t seed) {
  std::vector<std::uint8_t> bytes;
  while (*label) bytes.push_back(static_cast<std::uint8_t>(*label++));
  for (unsigned i = 0; i < 8; ++i) bytes.push_back(static_cast<std::uint8_t>(seed >> (i * 8)));
  return bytes;
}
void validate(const Program& p) {
  check(!p.empty() && p.size() <= kMaxInstructions, "invalid instruction count");
  check(p.back().op == Op::Halt, "program must end in Halt");
  for (const auto& ins : p) {
    switch (ins.op) {
      case Op::Const:
        check(ins.dst < kRegisterCount && ins.a == 0, "invalid Const operands"); break;
      case Op::Move:
        check(ins.dst < kRegisterCount && ins.a < kRegisterCount && ins.b == 0, "invalid Move operands"); break;
      case Op::Add: case Op::Sub: case Op::Xor:
        check(ins.dst < kRegisterCount && ins.a < kRegisterCount && ins.b < kRegisterCount, "invalid arithmetic operands"); break;
      case Op::Jnz:
        check(ins.dst == 0 && ins.a < kRegisterCount && ins.b < p.size(), "invalid branch operands"); break;
      case Op::Halt:
        check(ins.dst == 0 && ins.a < kRegisterCount && ins.b == 0, "invalid Halt operands"); break;
      default: throw std::runtime_error("unsupported opcode");
    }
  }
}
void validate_plan(const Plan& p) {
  auto order = p.field_order;
  std::sort(order.begin(), order.end());
  check(order == std::array<std::uint8_t, 4>{{0,1,2,3}}, "invalid field layout");
  auto ops = p.opcodes;
  std::sort(ops.begin(), ops.end());
  check(std::adjacent_find(ops.begin(), ops.end()) == ops.end(), "duplicate physical opcode");
}
} // namespace

Plan make_plan(std::uint64_t seed) {
  Plan plan;
  const auto id = package::sha256(domain_seed("venom-v4m3-build-id-v1|", seed));
  std::copy_n(id.begin(), 16, plan.build_id.begin());
  const auto material = package::sha256(domain_seed("venom-v4m3-mapping-v1|", seed));
  std::uint32_t state = get32(material.data());
  if (state == 0) state = 0x6d2b79f5u;
  for (std::size_t i = 0; i < plan.opcodes.size(); ++i) plan.opcodes[i] = 0x101u + static_cast<std::uint32_t>(i) * 0x102u;
  for (std::size_t i = plan.opcodes.size() - 1; i > 0; --i) {
    std::swap(plan.opcodes[i], plan.opcodes[next(state) % (i + 1)]);
  }
  plan.field_order = {{0,1,2,3}};
  for (std::size_t i = 3; i > 0; --i) std::swap(plan.field_order[i], plan.field_order[next(state) % (i + 1)]);
  for (auto& mask : plan.masks) mask = next(state);
  validate_plan(plan);
  return plan;
}

std::vector<std::uint8_t> encode(const Program& p, const Plan& plan) {
  validate_plan(plan);
  validate(p);
  std::vector<std::uint8_t> bytes;
  bytes.reserve(kHeaderSize + p.size() * 16);
  bytes.insert(bytes.end(), kMagic.begin(), kMagic.end());
  put32(bytes, kVersion);
  put32(bytes, static_cast<std::uint32_t>(p.size()));
  bytes.insert(bytes.end(), plan.build_id.begin(), plan.build_id.end());
  bytes.resize(kHeaderSize, 0); // digest at bytes 32..63
  for (const auto& ins : p) {
    const std::array<std::uint32_t,4> words{{plan.opcodes[static_cast<std::size_t>(ins.op)], ins.dst, ins.a, ins.b}};
    for (const auto id : plan.field_order) put32(bytes, words[id] ^ plan.masks[id]);
  }
  // Hash header identity, version, count and payload, excluding digest field.
  std::vector<std::uint8_t> signed_bytes(bytes.begin(), bytes.begin() + 32);
  signed_bytes.insert(signed_bytes.end(), bytes.begin() + kHeaderSize, bytes.end());
  const auto digest = package::sha256(signed_bytes);
  std::copy(digest.begin(), digest.end(), bytes.begin() + 32);
  return bytes;
}

Program decode(const std::vector<std::uint8_t>& bytes, const Plan& plan) {
  validate_plan(plan);
  check(bytes.size() >= kHeaderSize, "truncated bytecode header");
  check(std::equal(kMagic.begin(), kMagic.end(), bytes.begin()), "invalid bytecode magic");
  check(get32(bytes.data() + 8) == kVersion, "unsupported bytecode version");
  const auto count = get32(bytes.data() + 12);
  check(count > 0 && count <= kMaxInstructions, "invalid instruction count");
  check(bytes.size() == kHeaderSize + std::size_t(count) * 16, "invalid bytecode length");
  check(std::equal(plan.build_id.begin(), plan.build_id.end(), bytes.begin() + 16), "wrong runtime build");
  std::vector<std::uint8_t> signed_bytes(bytes.begin(), bytes.begin() + 32);
  signed_bytes.insert(signed_bytes.end(), bytes.begin() + kHeaderSize, bytes.end());
  const auto digest = package::sha256(signed_bytes);
  check(std::equal(digest.begin(), digest.end(), bytes.begin() + 32), "bytecode checksum mismatch");
  Program p;
  p.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    std::array<std::uint32_t,4> fields{};
    const auto* ins = bytes.data() + kHeaderSize + i * 16;
    for (std::size_t j = 0; j < 4; ++j) {
      const auto logical_field = plan.field_order[j];
      fields[logical_field] = get32(ins + j * 4) ^ plan.masks[logical_field];
    }
    std::size_t opcode = 0;
    for (; opcode < plan.opcodes.size(); ++opcode) if (plan.opcodes[opcode] == fields[0]) break;
    check(opcode < plan.opcodes.size(), "unknown physical opcode");
    p.push_back({static_cast<Op>(opcode), fields[1], fields[2], fields[3]});
  }
  validate(p);
  return p;
}

std::uint32_t run(const Program& p, std::uint64_t fuel) {
  validate(p);
  std::array<std::uint32_t, kRegisterCount> regs{};
  std::size_t pc = 0;
  for (;;) {
    check(fuel-- != 0, "execution fuel exhausted");
    check(pc < p.size(), "program counter out of range");
    const auto& ins = p[pc];
    switch (ins.op) {
      case Op::Const: regs[ins.dst] = ins.b; break;
      case Op::Move: regs[ins.dst] = regs[ins.a]; break;
      case Op::Add: regs[ins.dst] = regs[ins.a] + regs[ins.b]; break;
      case Op::Sub: regs[ins.dst] = regs[ins.a] - regs[ins.b]; break;
      case Op::Xor: regs[ins.dst] = regs[ins.a] ^ regs[ins.b]; break;
      case Op::Jnz:
        if (regs[ins.a] != 0) { pc = ins.b; continue; }
        break;
      case Op::Halt: return regs[ins.a];
      default: throw std::runtime_error("unsupported opcode");
    }
    ++pc;
  }
}

std::uint32_t run_package(const std::vector<std::uint8_t>& bytes, const Plan& plan, std::uint64_t fuel) {
  return run(decode(bytes, plan), fuel);
}
} // namespace venom::v4m3
