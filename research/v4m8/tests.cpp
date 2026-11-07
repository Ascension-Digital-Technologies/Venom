#include "typed.hpp"
#include "../v4m5/frontend.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace typed = venom::v4m8;
namespace vm = venom::v4m3;
namespace baseline = venom::v4m5;
void require(bool yes, const char* message) {
  if (!yes) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& fn, const char* message) {
  try { fn(); } catch (const std::runtime_error&) { return; }
  throw std::runtime_error(message);
}
std::uint32_t package_run(const vm::Program& p, std::uint64_t seed = 71,
                          const vm::HostPolicy& host = {}) {
  const auto plan = vm::make_plan(seed);
  return vm::run_package(vm::encode(p, plan), plan, 200000, host);
}
void compare(const std::string& src, std::uint64_t seed) {
  const auto module = typed::parse_typed(src);
  typed::verify(module);
  const auto expected = typed::interpret(module);
  require(package_run(typed::lower(module), seed) == expected,
          "typed SSA compiler disagrees with independent interpreter");
  require(package_run(typed::compile_source_typed(src), seed + 1) == expected,
          "typed source public entry point differs");
  require(package_run(baseline::compile_source(src), seed + 2) == expected,
          "V4-M7 reference compiler differs");
}
}
int main() {
  try {
    const std::vector<std::string> smoke{
      "function main(){return 42;}",
      "function main(){const x=7;let y=3;y=y+x;return y*2;}",
      "function main(){let a=11;let b=22;let t=a;a=b;b=t;return a^b;}",
      "function main(){return 4 < 5;}",
      "function main(){return (4+5) >= 9;}",
      "function main(){return 3 !== 2;}",
      "function main(){return true === !false;}",
      "function main(){return !true;}",
      "function main(){let n=2;n=n+1;n=n*3;n=n-4;return n;}",
      "function main(){return -7 + 9;}",
      "function main(){return (3*4 - 5) ^ (7*2);}",
      "function main(){const a=7;return (a === 7) ^ 1;}" // invalid: boolean xor
    };
    for (std::size_t i = 0; i + 1 < smoke.size(); ++i)
      compare(smoke[i], 101 + i);
    rejects([&]{typed::parse_typed(smoke.back());},
            "boolean XOR was accepted as implicit coercion");
    std::mt19937 rng(0x560008u);
    for (int i = 0; i < 2100; ++i) {
      const auto a = static_cast<int>(rng()%201u) - 100;
      const auto b = static_cast<int>(rng()%201u) - 100;
      const auto c = static_cast<int>(rng()%60u);
      const auto d = static_cast<int>(rng()%60u);
      const auto s = [](int v) { return v < 0 ? "(" + std::to_string(v) + ")" : std::to_string(v); };
      const auto op = i % 8 == 0 ? "+" : i % 8 == 1 ? "-" :
                      i % 8 == 2 ? "*" : i % 8 == 3 ? "^" :
                      i % 8 == 4 ? "<" : i % 8 == 5 ? "<=" :
                      i % 8 == 6 ? ">=" : "===";
      const std::string src =
        "function main(){let a=" + s(a) + ";let b=" + s(b) +
        ";let c=" + s(c) + ";c=c+" + s(d) +
        ";return (a " + op + " b);}";
      compare(src, 1000 + static_cast<std::uint64_t>(i));
    }

    // More than the 15 physical registers worth of individual SSA values,
    // but only a few are live at any instant: proves last-use reuse.
    std::string long_source="function main(){let v=0;";
    for (int i = 0; i < 400; ++i) long_source += "v=v+1;";
    long_source += "return v;}";
    const auto many = typed::parse_typed(long_source);
    require(many.nodes.size() >= 800, "source did not produce many virtual values");
    require(package_run(typed::lower(many)) == 400, "long chain register reuse");
    require(typed::interpret(many) == 400, "long chain interpreter");
    rejects([&]{typed::parse_typed("function main(){return 1.5;}");}, "float accepted");
    rejects([&]{typed::parse_typed("function main(){return fetch(1);}");}, "external call accepted");
    rejects([&]{typed::parse_typed("function main(){return eval(1);}");}, "eval accepted");
    rejects([&]{typed::parse_typed("function main(){return 1 + true;}");}, "coercion accepted");
    rejects([&]{typed::parse_typed("function main(){return 3 === false;}");}, "equality coercion accepted");
    rejects([&]{typed::parse_typed("function main(){let x=1;x=true;return x;}");}, "assignment type mismatch accepted");
    rejects([&]{typed::parse_typed("function main(){const x=1;x=2;return x;}");}, "const write accepted");
    rejects([&]{typed::parse_typed("function main(){return missing;}");}, "unknown variable accepted");
    rejects([&]{typed::parse_typed("function main(){return 2147483648;}");}, "large literal accepted");
    rejects([&]{typed::parse_typed("function main(){return 9999999999999999999999999999999999999999999999;}");},
            "oversized decimal accepted");
    rejects([&]{typed::parse_typed("function main(){let if=5;return if;}");}, "reserved name accepted");
    rejects([&]{typed::parse_typed("function main(){return 1;return 2;}");}, "unreachable code accepted");
    rejects([&]{typed::parse_typed("function main(){return 1;/* bad");}, "unterminated comment accepted");
    rejects([&]{typed::parse_typed("function main(){while(true){return 1;}}");}, "loop accepted in straight-line frontend");
    rejects([&]{typed::parse_typed("function main(){return true && false;}");}, "short-circuit accepted by eager frontend");
    rejects([&]{typed::parse_typed("function main(){return host(0,3);}");}, "zero capability accepted");
    rejects([&]{typed::parse_typed("function main(){return host(7,true);}");}, "invalid host argument accepted");

    auto invalid = typed::parse_typed("function main(){return 1+2;}");
    invalid.nodes[2].a = 3;
    rejects([&]{typed::verify(invalid);}, "forward value reference accepted");
    invalid = typed::parse_typed("function main(){return 1+2;}");
    invalid.result = 9000;
    rejects([&]{typed::verify(invalid);}, "forged return accepted");
    invalid = typed::parse_typed("function main(){return true;}");
    invalid.nodes[0].immediate = 5;
    rejects([&]{typed::verify(invalid);}, "invalid Boolean literal accepted");
    invalid = typed::parse_typed("function main(){return 1+2;}");
    invalid.nodes[2].type = typed::Type::Boolean;
    rejects([&]{typed::verify(invalid);}, "wrong arithmetic result type accepted");
    const auto over = typed::parse_typed("function main(){return 2147483647+1;}");
    rejects([&]{typed::interpret(over);}, "SSA checked overflow bypass");
    rejects([&]{package_run(typed::lower(over));}, "VM checked overflow bypass");

    int calls = 0;
    const auto host_module = typed::parse_typed("function main(){return host(7,3)+2;}");
    vm::HostPolicy policy{{7},[&](std::uint32_t cap, std::uint32_t arg) {
      require(cap == 7 && arg == 3, "invalid host arguments");
      ++calls; return std::uint32_t{4};
    },1};
    require(typed::interpret(host_module,policy) == 6, "typed SSA host call");
    require(package_run(typed::lower(host_module),61,policy) == 6, "typed VM host call");
    require(calls==2, "host callback count");
    rejects([&]{typed::interpret(host_module);}, "unauthorized host callback");
    rejects([&]{package_run(typed::lower(host_module));}, "unauthorized VM host callback");
    auto deny=policy;deny.max_calls=0;
    rejects([&]{typed::interpret(host_module,deny);},"SSA callback budget bypass");
    require(calls==2,"denied host callback side effect");
    const auto plan1=vm::make_plan(551),plan2=vm::make_plan(552);
    const auto bytes=vm::encode(typed::lower(host_module),plan1);
    rejects([&]{vm::run_package(bytes,plan2,2000,policy);}, "wrong-build bytecode accepted");
    std::cout<<"V4-M8: 2100 generated typed source/SSA/compiler comparisons PASS\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr << "V4-M8 FAILED: " << e.what() << '\n'; return 1;
  }
}
