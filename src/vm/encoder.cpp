#include "vm/encoder.hpp"

#include "vm/opcode.hpp"

#include <array>
#include <stdexcept>

namespace venom::vm {

namespace {
void push_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

std::uint32_t field_value(EncodedWord word,
                          std::uint32_t physical_opcode,
                          const Instruction& ins,
                          const PolymorphicPlan& plan) {
  switch (word) {
    case EncodedWord::Opcode: return physical_opcode ^ plan.opcode_mask;
    case EncodedWord::A: return ins.a ^ plan.operand_masks[0];
    case EncodedWord::B: return ins.b ^ plan.operand_masks[1];
    case EncodedWord::C: return ins.c ^ plan.operand_masks[2];
  }
  throw std::runtime_error("unknown encoded word in polymorphic layout");
}
} // namespace

std::vector<unsigned char> encode_program(const Program& program, const PolymorphicPlan& plan) {
  std::vector<unsigned char> out;
  out.reserve(program.size() * 16u);
  for (const auto& ins : program) {
    const auto logical = opcode_index(ins.opcode);
    const auto found = plan.logical_to_physical.find(logical);
    if (found == plan.logical_to_physical.end()) {
      throw std::runtime_error("missing physical opcode mapping");
    }
    const auto physical = static_cast<std::uint32_t>(found->second);
    for (const auto word : plan.word_layout) {
      push_u32(out, field_value(word, physical, ins, plan));
    }
  }
  return out;
}

} // namespace venom::vm
