#include "venom/vm/polymorph.hpp"

#include <array>
#include <cctype>
#include <algorithm>
#include <iostream>

int main() {
  const auto a = venom::vm::make_polymorphic_plan(0x12345678u, true);
  const auto b = venom::vm::make_polymorphic_plan(0x12345678u, true);
  const auto c = venom::vm::make_polymorphic_plan(0x87654321u, true);

  if (a.master_seed != b.master_seed || a.seed_commitment != b.seed_commitment || a.opcode_mask != b.opcode_mask) {
    std::cerr << "deterministic diversification is not reproducible\n";
    return 1;
  }
  if (a.master_seed == c.master_seed || a.seed_commitment == c.seed_commitment) {
    std::cerr << "distinct build seeds produced the same master seed\n";
    return 2;
  }
  if (a.seed_commitment.size() != 64) {
    std::cerr << "seed commitment is not SHA-256 hex\n";
    return 3;
  }
  for (char ch : a.seed_commitment) {
    if (!std::isxdigit(static_cast<unsigned char>(ch))) return 4;
  }

  venom::vm::DiversificationRng opcode(a, "route-opcode-map");
  venom::vm::DiversificationRng strings(a, "string-table-order");
  std::array<std::uint32_t, 8> x{};
  std::array<std::uint32_t, 8> y{};
  for (std::size_t i = 0; i < x.size(); ++i) {
    x[i] = opcode();
    y[i] = strings();
  }
  if (x == y) {
    std::cerr << "domain-separated generators reused a stream\n";
    return 5;
  }

  if (a.dom_commands.logical_to_physical != b.dom_commands.logical_to_physical ||
      a.dom_commands.field_layout != b.dom_commands.field_layout) {
    std::cerr << "deterministic DOM protocol specialization is not reproducible\n";
    return 6;
  }
  if (a.dom_commands.logical_to_physical == c.dom_commands.logical_to_physical &&
      a.dom_commands.field_layout == c.dom_commands.field_layout) {
    std::cerr << "distinct builds produced the same DOM wire protocol\n";
    return 7;
  }
  {
    std::array<bool, 65536> seen{};
    for (const auto physical : a.dom_commands.logical_to_physical) {
      if (!physical || seen[physical]) {
        std::cerr << "DOM command mapping contains zero or duplicate identifiers\n";
        return 8;
      }
      seen[physical] = true;
    }
    std::array<bool, 3> field_seen{};
    for (const auto field : a.dom_commands.field_layout) {
      if (field > 2 || field_seen[field]) {
        std::cerr << "DOM field layout is not a permutation\n";
        return 9;
      }
      field_seen[field] = true;
    }
  }
  const auto encoded = a.encode_binary();
  const std::array<unsigned char, 8> expected_magic{'V','P','O','L','0','0','1','0'};
  if (encoded.size() < 72 || !std::equal(expected_magic.begin(), expected_magic.end(), encoded.begin())) {
    std::cerr << "polymorphic plan did not encode the v10 specialization contract\n";
    return 10;
  }

  std::cout << "cryptographic diversification RNG checks passed\n";
  return 0;
}
