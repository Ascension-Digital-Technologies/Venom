#include "vm.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

using namespace venom::v4m3;
namespace {
void require(bool test, const char* message) { if (!test) throw std::runtime_error(message); }
void rejects(const std::function<void()>& f, const char* message) {
  try { f(); } catch (const std::runtime_error&) { return; }
  throw std::runtime_error(message);
}
void set32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t x) {
  for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = static_cast<std::uint8_t>(x >> (i * 8));
}
} // namespace
int main() {
  try {
    const Plan a = make_plan(1), b = make_plan(2), a2 = make_plan(1);
    require(a.build_id == a2.build_id && a.opcodes == a2.opcodes && a.field_order == a2.field_order, "unstable plan");
    require(a.build_id != b.build_id && a.opcodes != b.opcodes, "build diversity missing");
    const Program p{{Op::Const,0,0,7},{Op::Const,1,0,5},{Op::Add,2,0,1},
                    {Op::Sub,3,2,1},{Op::Xor,4,3,1},{Op::Move,5,4,0},{Op::Halt,0,5,0}};
    const auto bytes_a = encode(p, a), bytes_b = encode(p, b);
    require(bytes_a != bytes_b, "identical bytecode across builds");
    require(decode(bytes_a, a).size() == p.size(), "roundtrip size mismatch");
    require(run_package(bytes_a, a) == 2 && run_package(bytes_b, b) == 2, "execution mismatch");
    rejects([&] { decode(bytes_a, b); }, "cross-build bytes accepted");
    auto corrupted = bytes_a; corrupted[65] ^= 0x80;
    rejects([&] { decode(corrupted, a); }, "tampered payload accepted");
    corrupted = bytes_a; corrupted[0] ^= 0x01;
    rejects([&] { decode(corrupted, a); }, "invalid magic accepted");
    corrupted = bytes_a; set32(corrupted, 8, 5);
    rejects([&] { decode(corrupted, a); }, "wrong version accepted");
    corrupted = bytes_a; set32(corrupted, 12, 0xffffffffu);
    rejects([&] { decode(corrupted, a); }, "oversized program accepted");
    corrupted = bytes_a; corrupted.pop_back();
    rejects([&] { decode(corrupted, a); }, "truncated payload accepted");
    corrupted = bytes_a; corrupted.push_back(0);
    rejects([&] { decode(corrupted, a); }, "trailing bytes accepted");
    const Program invalid{{Op::Move,0,16,0},{Op::Halt,0,0,0}};
    rejects([&] { encode(invalid, a); }, "invalid register accepted");
    const Program loop{{Op::Const,0,0,1},{Op::Jnz,0,0,1},{Op::Halt,0,0,0}};
    rejects([&] { run_package(encode(loop,a), a, 20); }, "infinite loop ignored fuel");
    // Differentially compare encoded execution with the semantic reference.
    std::mt19937 generator(0x42524245u);
    for (int n = 0; n < 2000; ++n) {
      Program generated;
      for (int i = 0; i < 25; ++i) {
        const auto op = static_cast<Op>(generator() % 5);
        const std::uint32_t dst = generator() % 16, ra = generator() % 16;
        std::uint32_t rb = generator() % 16;
        if (op == Op::Const) { rb = generator(); generated.push_back({op,dst,0,rb}); }
        else if (op == Op::Move) generated.push_back({op,dst,ra,0});
        else generated.push_back({op,dst,ra,rb});
      }
      generated.push_back({Op::Halt,0,static_cast<std::uint32_t>(n % 16),0});
      const Plan plan = make_plan(static_cast<std::uint64_t>(n) + 100);
      require(run(generated) == run_package(encode(generated, plan), plan), "differential mismatch");
    }
    std::cout << "Venom V4-M3: 2,000 differential cases and negative cases PASS\n";
  } catch (const std::exception& e) {
    std::cerr << "Venom V4-M3 test FAILED: " << e.what() << '\n'; return 1;
  }
}
