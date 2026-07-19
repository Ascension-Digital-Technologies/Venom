#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "venom/vm/bytecode.hpp"

namespace venom::vm {

enum class EncodedWord : std::uint8_t { Opcode = 0, A = 1, B = 2, C = 3 };

struct HostCallPlan { std::uint16_t dom = 0; std::uint16_t event = 0; std::uint16_t fetch = 0; std::uint16_t quickjs = 0; };

struct DomCommandPlan {
  // Logical DOM commands 1..7 map to build-specific physical identifiers.
  std::array<std::uint16_t, 7> logical_to_physical{1, 2, 3, 4, 5, 6, 7};
  // Physical order of the three payload fields a/b/c in each binary record.
  std::array<std::uint8_t, 3> field_layout{0, 1, 2};
};

struct PolymorphicPlan {
  // Public 32-bit commitment tag retained for the package contract. It is not
  // the diversification secret and cannot be used to replay the generator.
  std::uint32_t seed = 0;
  std::array<unsigned char, 32> master_seed{};
  std::string seed_commitment;
  bool enabled = false;
  bool shuffle_strings = false;
  bool shuffle_routes = false;
  std::array<EncodedWord, 4> word_layout{EncodedWord::Opcode, EncodedWord::A, EncodedWord::B, EncodedWord::C};
  std::uint32_t opcode_mask = 0;
  std::array<std::uint32_t, 3> operand_masks{0, 0, 0};
  HostCallPlan host_calls;
  DomCommandPlan dom_commands;
  std::unordered_map<std::uint16_t, std::uint16_t> logical_to_physical;
  std::string describe() const;
  std::vector<unsigned char> encode_binary() const;
};

// SHA-256 counter-mode deterministic generator. Each caller uses an explicit
// domain label so unrelated transformations never reuse one random stream.
class DiversificationRng {
 public:
  using result_type = std::uint32_t;
  DiversificationRng(const PolymorphicPlan& plan, const std::string& domain);
  result_type operator()();
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return 0xffffffffu; }
 private:
  std::array<unsigned char, 32> key_{};
  std::uint64_t counter_ = 0;
  std::array<unsigned char, 32> block_{};
  std::size_t offset_ = 32;
  void refill();
};

PolymorphicPlan make_polymorphic_plan(std::uint32_t deterministic_seed, bool enabled = true);
std::uint32_t derive_diversification_u32(const PolymorphicPlan& plan, const std::string& domain);
const char* encoded_word_name(EncodedWord word);

} // namespace venom::vm
