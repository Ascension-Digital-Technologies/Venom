#include "cfg.hpp"
#include "../v4m4/ir.hpp"
#include "../v4m5/frontend.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

namespace {
namespace vm = venom::v4m3;
namespace ir = venom::v4m4;
namespace js = venom::v4m5;
namespace cfg = venom::v4m6;
using T = ir::Terminator;
void require(bool yes, const char* why) {
  if (!yes) throw std::runtime_error(why);
}
void rejects(const std::function<void()>& fn, const char* why) {
  try { fn(); } catch (const std::runtime_error&) { return; }
  throw std::runtime_error(why);
}
std::uint32_t compiled_ir(const ir::Module& m, std::uint64_t seed) {
  const auto plan = vm::make_plan(seed);
  return vm::run_package(vm::encode(ir::lower(m),plan),plan);
}
std::uint32_t compiled_js(const std::string& source, const vm::HostPolicy& host = {}) {
  const auto plan = vm::make_plan(37);
  return vm::run_package(vm::encode(js::compile_source(source),plan),plan,100000,host);
}
ir::Module countdown(std::uint32_t n) {
  ir::Module m;
  m.blocks.resize(4);
  m.blocks[0].values = {{1,ir::Kind::Literal,0,0,0},
                        {2,ir::Kind::Literal,0,0,n},
                        {3,ir::Kind::Literal,0,0,1}};
  m.blocks[0].end = {T::Kind::Jump,0,1,0};
  m.blocks[1].phis = {
    {4,{{std::size_t{0},std::uint8_t{2}},{std::size_t{2},std::uint8_t{7}}}},
    {5,{{std::size_t{0},std::uint8_t{1}},{std::size_t{2},std::uint8_t{8}}}}
  };
  m.blocks[1].end = {T::Kind::Branch,4,2,3};
  m.blocks[2].values = {{7,ir::Kind::Sub,4,3,0},
                        {8,ir::Kind::Add,5,4,0}};
  m.blocks[2].end = {T::Kind::Jump,0,1,0};
  m.blocks[3].end = {T::Kind::Return,5,0,0};
  return m;
}
ir::Module swap_loop(std::uint32_t steps) {
  ir::Module m;
  m.blocks.resize(4);
  m.blocks[0].values = {{1,ir::Kind::Literal,0,0,11},
                        {2,ir::Kind::Literal,0,0,29},
                        {3,ir::Kind::Literal,0,0,1},
                        {4,ir::Kind::Literal,0,0,steps}};
  m.blocks[0].end = {T::Kind::Jump,0,1,0};
  m.blocks[1].phis = {
    {5,{{std::size_t{0},std::uint8_t{1}},{std::size_t{2},std::uint8_t{6}}}},
    {6,{{std::size_t{0},std::uint8_t{2}},{std::size_t{2},std::uint8_t{5}}}},
    {7,{{std::size_t{0},std::uint8_t{4}},{std::size_t{2},std::uint8_t{8}}}}
  };
  m.blocks[1].end = {T::Kind::Branch,7,2,3};
  m.blocks[2].values = {{8,ir::Kind::Sub,7,3,0}};
  m.blocks[2].end = {T::Kind::Jump,0,1,0};
  m.blocks[3].end = {T::Kind::Return,5,0,0};
  return m;
}
}

int main() {
  try {
    for (std::uint32_t n = 0; n < 256; ++n) {
      const auto m = countdown(n);
      const auto expected = n * (n + 1u) / 2u;
      require(ir::interpret(m) == expected, "loop reference mismatch");
      require(compiled_ir(m, 3000 + n) == expected, "loop lowering mismatch");
    }
    for (std::uint32_t n = 0; n < 60; ++n) {
      const auto m = swap_loop(n);
      const auto expected = n % 2u ? 29u : 11u;
      require(ir::interpret(m) == expected, "parallel phi reference mismatch");
      require(compiled_ir(m, 1000 + n) == expected, "parallel phi lowering mismatch");
    }
    auto broken = countdown(3);
    broken.blocks[1].phis[0].incoming.pop_back();
    rejects([&] { ir::verify(broken); }, "incomplete loop phi accepted");
    broken = countdown(3);
    broken.blocks[1].phis[0].incoming[1].second = 9;
    rejects([&] { ir::verify(broken); }, "undefined loop phi accepted");
    broken = countdown(3);
    broken.blocks[2].values[0].a = 8;
    rejects([&] { ir::verify(broken); }, "loop body use-before-define accepted");
    broken = countdown(3);
    broken.blocks[0].end = {T::Kind::Return,2,0,0};
    rejects([&] { ir::verify(broken); }, "detached loop accepted");
    rejects([&] { ir::interpret(countdown(30),3); }, "IR fuel bypass");
    rejects([&] { vm::run(ir::lower(countdown(30)),4); }, "VM fuel bypass");

    using vm::Op;
    const vm::Program undefined{{Op::Const,0,0,1},{Op::Move,1,4,0},{Op::Halt,0,1,0}};
    rejects([&] { cfg::verify_compiled_cfg(undefined); }, "uninitialized read accepted");
    const vm::Program partial{{Op::Const,0,0,1},{Op::Const,1,0,1},
      {Op::Jnz,0,1,4},{Op::Jnz,0,0,5},{Op::Const,2,0,7},{Op::Halt,0,2,0}};
    rejects([&] { cfg::verify_compiled_cfg(partial); }, "partially defined merge accepted");
    const vm::Program backedge{{Op::Const,0,0,1},
      {Op::Jnz,0,0,3},{Op::Const,4,0,7},{Op::Jnz,0,4,2},{Op::Halt,0,4,0}};
    rejects([&] { cfg::verify_compiled_cfg(backedge); }, "backedge uninitialized read accepted");
    const vm::Program sentinel{{Op::Const,0,0,1},{Op::Const,0,0,2},{Op::Halt,0,0,0}};
    rejects([&] { cfg::verify_compiled_cfg(sentinel); }, "reserved register mutation accepted");
    const vm::Program invalid_branch{{Op::Const,0,0,1},{Op::Jnz,0,0,30},{Op::Halt,0,0,0}};
    rejects([&] { cfg::verify_compiled_cfg(invalid_branch); }, "bad jump accepted");

    require(compiled_js("function main(){return false && true;}") == 0, "and false");
    require(compiled_js("function main(){return true && false;}") == 0, "and true");
    require(compiled_js("function main(){return true || false;}") == 1, "or true");
    require(compiled_js("function main(){return false || true;}") == 1, "or false");
    require(compiled_js("function main(){return true || false && false;}") == 1, "precedence");
    require(compiled_js("function main(){return false || (1 < 2 && 2 < 3);}") == 1, "nested comparisons");
    rejects([&] { compiled_js("function main(){return 1 && true;}"); }, "implicit coercion accepted");
    rejects([&] { compiled_js("function main(){return true || 2;}"); }, "implicit coercion accepted");
    int calls = 0;
    vm::HostPolicy host{{7}, [&](std::uint32_t cap, std::uint32_t arg) {
      require(cap == 7 && arg == 3, "host arguments invalid");
      ++calls;
      return std::uint32_t{8};
    }, 1};
    require(compiled_js("function main(){return false && (host(7,3)===8);}") == 0,
            "and short-circuit failed");
    require(compiled_js("function main(){return true || (host(7,3)===8);}") == 1,
            "or short-circuit failed");
    require(calls == 0, "skipped host call had side effect");
    require(compiled_js("function main(){return true && (host(7,3)===8);}", host) == 1,
            "allowed host call failed");
    require(compiled_js("function main(){return false || (host(7,3)===8);}", host) == 1,
            "allowed host call on or failed");
    require(calls == 2, "allowed host call count mismatch");

    std::mt19937 rng(0x564d3600u);
    for (int i = 0; i < 2000; ++i) {
      const std::uint32_t n = rng() % 128u;
      const auto candidate = i % 2 ? countdown(n) : swap_loop(n % 48u);
      require(compiled_ir(candidate, 90000 + static_cast<std::uint64_t>(i)) ==
              ir::interpret(candidate), "random loop and phi differential mismatch");
    }
    std::cout << "V4-M6: 2,000 loop/phi differentials, CFG and short-circuit tests PASS\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "V4-M6 FAILED: " << e.what() << '\n';
    return 1;
  }
}
