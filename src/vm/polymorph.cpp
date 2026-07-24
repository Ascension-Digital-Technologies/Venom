#include "vm/polymorph.hpp"

#include "vm/opcode.hpp"
#include "package/hash.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <random>
#include <chrono>
#include <cstring>
#include <sstream>

namespace venom::vm {

const char* encoded_word_name(EncodedWord word) {
  switch (word) {
    case EncodedWord::Opcode: return "opcode";
    case EncodedWord::A: return "a";
    case EncodedWord::B: return "b";
    case EncodedWord::C: return "c";
  }
  return "unknown";
}

namespace {
std::string hex32(std::uint32_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
  return out.str();
}

std::string hex16(std::uint16_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setw(4) << std::setfill('0') << value;
  return out.str();
}
} // namespace


std::vector<unsigned char> PolymorphicPlan::encode_binary() const {
  std::vector<unsigned char> out;
  out.reserve(112u);
  const auto push_u16 = [&](std::uint16_t value) {
    out.push_back(static_cast<unsigned char>(value & 0xffu));
    out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  };
  const auto push_u32 = [&](std::uint32_t value) {
    out.push_back(static_cast<unsigned char>(value & 0xffu));
    out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
    out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
    out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
  };
  out.insert(out.end(), {'V','P','O','L','0','0','1','0'});
  push_u32(2u);
  push_u32(seed);
  std::uint32_t flags = enabled ? 1u : 0u;
  if (shuffle_strings) flags |= 2u;
  if (shuffle_routes) flags |= 4u;
  push_u32(flags);
  push_u32(opcode_mask);
  push_u32(operand_masks[0]);
  push_u32(operand_masks[1]);
  push_u32(operand_masks[2]);
  for (const auto word : word_layout) out.push_back(static_cast<unsigned char>(word));
  push_u16(host_calls.dom); push_u16(host_calls.event);
  push_u16(host_calls.fetch); push_u16(host_calls.turbojs);
  push_u32(static_cast<std::uint32_t>(logical_to_physical.size()));
  for (const auto physical : dom_commands.logical_to_physical) push_u16(physical);
  for (const auto field : dom_commands.field_layout) out.push_back(field);
  out.push_back(0u);
  out.push_back(0u);
  out.push_back(0u);
  std::vector<std::pair<std::uint16_t, std::uint16_t>> entries(logical_to_physical.begin(), logical_to_physical.end());
  std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  for (const auto& [logical, physical] : entries) { push_u16(logical); push_u16(physical); }
  return out;
}

std::string PolymorphicPlan::describe() const {
  std::ostringstream out;
  out << "VENOM_POLYMORPH 10\n";
  out << "seed_commitment=" << seed_commitment << "\n";
  out << "enabled=" << (enabled ? 1 : 0) << "\n";
  out << "instruction_size=16\n";
  out << "word_layout="
      << encoded_word_name(word_layout[0]) << ","
      << encoded_word_name(word_layout[1]) << ","
      << encoded_word_name(word_layout[2]) << ","
      << encoded_word_name(word_layout[3]) << "\n";
  out << "opcode_xor=" << hex32(opcode_mask) << "\n";
  out << "operand_xor=" << hex32(operand_masks[0]) << ","
      << hex32(operand_masks[1]) << ","
      << hex32(operand_masks[2]) << "\n";
  out << "shuffle_strings=" << (shuffle_strings ? 1 : 0) << "\n";
  out << "shuffle_routes=" << (shuffle_routes ? 1 : 0) << "\n";
  out << "host_call dom=" << hex16(host_calls.dom)
      << " event=" << hex16(host_calls.event)
      << " fetch=" << hex16(host_calls.fetch)
      << " turbojs=" << hex16(host_calls.turbojs) << "\n";
  out << "dom_command_map=";
  for (std::size_t i = 0; i < dom_commands.logical_to_physical.size(); ++i) {
    if (i) out << ",";
    out << (i + 1u) << ":" << hex16(dom_commands.logical_to_physical[i]);
  }
  out << "\n";
  out << "dom_field_layout=" << static_cast<unsigned>(dom_commands.field_layout[0]) << ","
      << static_cast<unsigned>(dom_commands.field_layout[1]) << ","
      << static_cast<unsigned>(dom_commands.field_layout[2]) << "\n";
  out << "opcode_map:\n";
  for (const auto& pair : logical_to_physical) {
    out << pair.first << " => " << pair.second << "\n";
  }
  return out.str();
}

namespace {
std::array<unsigned char, 32> make_master_seed(std::uint32_t deterministic_seed) {
  std::vector<unsigned char> material;
  if (deterministic_seed != 0) {
    const std::string prefix = "venom-diversification-deterministic-v1|" + std::to_string(deterministic_seed);
    material.assign(prefix.begin(), prefix.end());
  } else {
    // Collect a full 256 bits from the implementation's entropy source. The
    // hash also mixes timing/address material as defense-in-depth for unusual
    // standard-library implementations with narrow random_device output.
    std::random_device rd;
    material.reserve(96);
    for (int i = 0; i < 16; ++i) {
      const std::uint32_t v = static_cast<std::uint32_t>(rd());
      for (int j = 0; j < 4; ++j) material.push_back(static_cast<unsigned char>((v >> (j * 8)) & 0xffu));
    }
    const auto now = static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const auto addr = reinterpret_cast<std::uintptr_t>(&material);
    for (int j = 0; j < 8; ++j) material.push_back(static_cast<unsigned char>((now >> (j * 8)) & 0xffu));
    for (std::size_t j = 0; j < sizeof(addr); ++j) material.push_back(static_cast<unsigned char>((addr >> (j * 8)) & 0xffu));
  }
  return venom::package::sha256(material);
}

std::array<unsigned char, 32> derive_key(const std::array<unsigned char, 32>& master, const std::string& domain) {
  std::vector<unsigned char> input(master.begin(), master.end());
  static constexpr char kPrefix[] = "venom-diversification-domain-v1|";
  input.insert(input.end(), kPrefix, kPrefix + sizeof(kPrefix) - 1);
  input.insert(input.end(), domain.begin(), domain.end());
  return venom::package::sha256(input);
}
}

DiversificationRng::DiversificationRng(const PolymorphicPlan& plan, const std::string& domain)
    : key_(derive_key(plan.master_seed, domain)) {}

void DiversificationRng::refill() {
  std::vector<unsigned char> input(key_.begin(), key_.end());
  for (int i = 0; i < 8; ++i) input.push_back(static_cast<unsigned char>((counter_ >> (i * 8)) & 0xffu));
  block_ = venom::package::sha256(input);
  ++counter_;
  offset_ = 0;
}

DiversificationRng::result_type DiversificationRng::operator()() {
  if (offset_ + 4 > block_.size()) refill();
  const std::uint32_t value = static_cast<std::uint32_t>(block_[offset_]) |
      (static_cast<std::uint32_t>(block_[offset_ + 1]) << 8u) |
      (static_cast<std::uint32_t>(block_[offset_ + 2]) << 16u) |
      (static_cast<std::uint32_t>(block_[offset_ + 3]) << 24u);
  offset_ += 4;
  return value;
}

std::uint32_t derive_diversification_u32(const PolymorphicPlan& plan, const std::string& domain) {
  DiversificationRng rng(plan, domain);
  return rng();
}

PolymorphicPlan make_polymorphic_plan(std::uint32_t deterministic_seed, bool enabled) {
  std::array<LogicalOpcode, 10> ops{
    LogicalOpcode::Nop, LogicalOpcode::LoadConst, LogicalOpcode::CreateElement,
    LogicalOpcode::SetAttribute, LogicalOpcode::SetText, LogicalOpcode::AppendChild,
    LogicalOpcode::EnterElement, LogicalOpcode::LeaveElement,
    LogicalOpcode::CallTurboJs, LogicalOpcode::Halt,
  };

  PolymorphicPlan plan;
  plan.master_seed = make_master_seed(deterministic_seed);
  plan.seed_commitment = venom::package::sha256_hex(
      std::vector<unsigned char>(plan.master_seed.begin(), plan.master_seed.end()));
  const auto public_tag = derive_key(plan.master_seed, "public-seed-tag");
  plan.seed = static_cast<std::uint32_t>(public_tag[0]) |
      (static_cast<std::uint32_t>(public_tag[1]) << 8u) |
      (static_cast<std::uint32_t>(public_tag[2]) << 16u) |
      (static_cast<std::uint32_t>(public_tag[3]) << 24u);
  plan.enabled = enabled;

  if (!enabled) {
    for (const auto op : ops) plan.logical_to_physical.emplace(opcode_index(op), opcode_index(op));
    return plan;
  }

  DiversificationRng opcode_rng(plan, "route-opcode-map");
  std::array<std::uint16_t, 10> physical{0x11, 0x27, 0x39, 0x4d, 0x52, 0x62, 0x75, 0x7c, 0x81, 0xa4};
  std::shuffle(physical.begin(), physical.end(), opcode_rng);
  for (std::size_t i = 0; i < ops.size(); ++i) plan.logical_to_physical.emplace(opcode_index(ops[i]), physical[i]);

  DiversificationRng layout_rng(plan, "route-word-layout");
  std::array<EncodedWord, 4> layout{EncodedWord::Opcode, EncodedWord::A, EncodedWord::B, EncodedWord::C};
  std::shuffle(layout.begin(), layout.end(), layout_rng);
  plan.word_layout = layout;

  DiversificationRng mask_rng(plan, "route-masks");
  plan.opcode_mask = mask_rng();
  plan.operand_masks = {mask_rng(), mask_rng(), mask_rng()};
  plan.shuffle_strings = true;
  plan.shuffle_routes = true;
  DiversificationRng host_rng(plan, "route-host-call-classes");
  plan.host_calls.dom = static_cast<std::uint16_t>(0x100u + (host_rng() & 0x3ffu));
  plan.host_calls.event = static_cast<std::uint16_t>(0x500u + (host_rng() & 0x3ffu));
  plan.host_calls.fetch = static_cast<std::uint16_t>(0x900u + (host_rng() & 0x3ffu));
  plan.host_calls.turbojs = static_cast<std::uint16_t>(0xd00u + (host_rng() & 0x3ffu));

  DiversificationRng dom_opcode_rng(plan, "dom-command-opcode-map");
  for (std::size_t i = 0; i < plan.dom_commands.logical_to_physical.size(); ++i) {
    std::uint16_t candidate = 0u;
    bool unique = false;
    while (!unique) {
      candidate = static_cast<std::uint16_t>((dom_opcode_rng() & 0xfffeu) | 1u);
      unique = candidate != 0u;
      for (std::size_t j = 0; unique && j < i; ++j) {
        if (plan.dom_commands.logical_to_physical[j] == candidate) unique = false;
      }
    }
    plan.dom_commands.logical_to_physical[i] = candidate;
  }
  DiversificationRng dom_layout_rng(plan, "dom-command-field-layout");
  std::array<std::uint8_t, 3> dom_layout{0u, 1u, 2u};
  std::shuffle(dom_layout.begin(), dom_layout.end(), dom_layout_rng);
  plan.dom_commands.field_layout = dom_layout;
  return plan;
}

} // namespace venom::vm
