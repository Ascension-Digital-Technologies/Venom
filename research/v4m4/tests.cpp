#include "ir.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>

using namespace venom::v4m4;
namespace {
using T = Terminator;
void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& fn, const char* message) {
  try { fn(); } catch (const std::runtime_error&) { return; }
  throw std::runtime_error(message);
}
Module diamond(std::uint32_t cond, std::uint32_t yes,
               std::uint32_t no, std::uint32_t bonus) {
  Module m;
  m.blocks.resize(4);
  m.blocks[0].values.push_back({1, Kind::Literal, 0, 0, cond});
  m.blocks[0].end = {T::Kind::Branch, 1, 1, 2};
  m.blocks[1].values.push_back({2, Kind::Literal, 0, 0, yes});
  m.blocks[1].end = {T::Kind::Jump, 0, 3, 0};
  m.blocks[2].values.push_back({3, Kind::Literal, 0, 0, no});
  m.blocks[2].end = {T::Kind::Jump, 0, 3, 0};
  m.blocks[3].phis.push_back({4, {{1,2},{2,3}}});
  m.blocks[3].values.push_back({5, Kind::Literal, 0, 0, bonus});
  m.blocks[3].values.push_back({6, Kind::Add, 4, 5, 0});
  m.blocks[3].end = {T::Kind::Return, 6, 0, 0};
  return m;
}
Module masked_promotion(std::uint32_t user_id, std::uint32_t flags) {
  // Mocked uint32-only application kernel; not a full JS application.
  Module m;
  m.blocks.resize(4);
  m.blocks[0].values = {{1,Kind::Literal,0,0,user_id},
                        {2,Kind::Literal,0,0,flags},
                        {3,Kind::Xor,1,2,0}};
  m.blocks[0].end = {T::Kind::Branch, 3, 1, 2};
  m.blocks[1].values = {{4,Kind::Literal,0,0,10},
                        {5,Kind::Add,1,4,0}};
  m.blocks[1].end = {T::Kind::Jump, 0, 3, 0};
  m.blocks[2].values = {{6,Kind::Literal,0,0,20},
                        {7,Kind::Sub,1,6,0}};
  m.blocks[2].end = {T::Kind::Jump, 0, 3, 0};
  m.blocks[3].phis = {{8,{{1,5},{2,7}}}};
  m.blocks[3].end = {T::Kind::Return, 8, 0, 0};
  return m;
}
Module host_kernel() {
  Module m;
  m.blocks.resize(1);
  m.blocks[0].values = {{1,Kind::Literal,0,0,17},
                        {2,Kind::HostCall,1,0,7}};
  m.blocks[0].end = {T::Kind::Return, 2, 0, 0};
  return m;
}
} // namespace

int main() {
  try {
    const auto plan = venom::v4m3::make_plan(90831);
    const auto m = diamond(1, 41, 7, 1);
    require(interpret(m) == 42, "IR reference join incorrect");
    require(venom::v4m3::run_package(venom::v4m3::encode(lower(m), plan),
                                    plan) == 42, "bytecode phi lowering incorrect");
    require(interpret(diamond(0,41,7,1)) == 8, "false IR edge incorrect");
    require(venom::v4m3::run_package(
      venom::v4m3::encode(lower(diamond(0,41,7,1)),plan),plan) == 8,
      "false bytecode edge incorrect");

    const auto effects = host_kernel();
    const auto effect_package = venom::v4m3::encode(lower(effects), plan);
    int invoked = 0;
    venom::v4m3::HostPolicy allowed{{7}, [&](std::uint32_t cap, std::uint32_t arg) {
      ++invoked;
      require(cap == 7 && arg == 17, "incorrect host invocation");
      return arg + 9;
    }, 1};
    require(interpret(effects, 100, allowed) == 26, "IR host incorrect");
    require(venom::v4m3::run_package(effect_package, plan, 100, allowed) == 26,
            "protected host incorrect");
    require(invoked == 2, "incorrect host invocation count");
    rejects([&] { venom::v4m3::run_package(effect_package,plan); },
            "default host policy allowed call");
    rejects([&] { interpret(effects); },
            "default IR host policy allowed call");
    auto no_budget = allowed; no_budget.max_calls = 0;
    rejects([&] { venom::v4m3::run_package(effect_package,plan,100,no_budget); },
            "host call budget bypass");
    auto no_allow = allowed; no_allow.allowed = {8};
    rejects([&] { venom::v4m3::run_package(effect_package,plan,100,no_allow); },
            "host capability bypass");
    auto no_callback = allowed; no_callback.invoke = {};
    rejects([&] { venom::v4m3::run_package(effect_package,plan,100,no_callback); },
            "host callback bypass");
    rejects([&] { venom::v4m3::run_package(effect_package,plan,1,allowed); },
            "VM fuel bypass");
    rejects([&] { interpret(effects,1,allowed); }, "IR fuel bypass");
    require(invoked == 2, "denied host call had a side effect");

    auto bad = diamond(1,41,7,1);
    bad.blocks[3].phis[0].incoming.pop_back();
    rejects([&] { lower(bad); }, "incomplete phi accepted");
    bad = diamond(1,41,7,1);
    bad.blocks[3].phis[0].incoming[1].first = 1;
    rejects([&] { verify(bad); }, "duplicate phi predecessor accepted");
    bad = diamond(1,41,7,1);
    bad.blocks[0].values.push_back({1,Kind::Literal,0,0,7});
    rejects([&] { verify(bad); }, "duplicate SSA definition accepted");
    bad = diamond(1,41,7,1);
    bad.blocks[3].values[1].a = 14;
    rejects([&] { verify(bad); }, "undefined SSA value accepted");
    bad = diamond(1,41,7,1);
    bad.blocks[0].end.no = 1;
    rejects([&] { verify(bad); }, "duplicate CFG branch accepted");
    bad = diamond(1,41,7,1);
    bad.blocks[1].end.yes = 1;
    rejects([&] { verify(bad); }, "cyclic CFG accepted");
    bad = diamond(1,41,7,1);
    bad.blocks[3].values.push_back({7,Kind::Add,2,5,0});
    rejects([&] { verify(bad); }, "non-dominating value accepted");
    bad = host_kernel();
    bad.blocks[0].values[1].immediate = 0;
    rejects([&] { verify(bad); }, "zero host capability accepted");
    bad = host_kernel();
    bad.blocks[0].phis.push_back({3,{}});
    rejects([&] { interpret(bad); }, "entry phi accepted");

    std::mt19937 rng(0x4d345345u);
    for (int i = 0; i < 2000; ++i) {
      const std::uint32_t a = rng(), b = rng(), c = rng();
      const auto candidate = i % 2 ? diamond(a & 1, b, c, a)
                                    : masked_promotion(a, b);
      const auto mapping = venom::v4m3::make_plan(
        0x123456789ull + static_cast<std::uint64_t>(i));
      const auto expected = interpret(candidate);
      const auto code = lower(candidate);
      const auto package = venom::v4m3::encode(code, mapping);
      require(venom::v4m3::run_package(package,mapping) == expected,
              "IR and encoded runtime differ");
      require(venom::v4m3::decode(package,mapping).size() == code.size(),
              "bytecode decoder roundtrip mismatch");
    }
    std::cout << "Venom V4-M4: 2,000 IR/compiler/runtime differential cases PASS\n";
  } catch (const std::exception& err) {
    std::cerr << "Venom V4-M4 FAILED: " << err.what() << '\n';
    return 1;
  }
}
