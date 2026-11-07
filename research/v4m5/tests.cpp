#include "frontend.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using venom::v4m5::compile_source;
using venom::v4m3::encode;
using venom::v4m3::make_plan;
using venom::v4m3::run_package;
namespace {
void check(bool condition, const char* reason) {
  if (!condition) throw std::runtime_error(reason);
}
void rejects(const std::function<void()>& call, const char* reason) {
  try { call(); } catch (const std::runtime_error&) { return; }
  throw std::runtime_error(reason);
}
std::uint32_t run(const std::string& source, std::uint64_t seed = 37,
                  std::uint64_t fuel = 200000,
                  const venom::v4m3::HostPolicy& policy = {}) {
  const auto plan = make_plan(seed);
  return run_package(encode(compile_source(source), plan), plan, fuel, policy);
}
} // namespace
int main() {
  try {
    check(run("function main(){ return 42; }") == 42, "literal return");
    check(run("function main() { const n=7; let b=5; b=n+b; return b*3; }") == 36,
          "variables and arithmetic");
    check(run("function main(){let x=3; while(x>0){x=x-1;} return x;}") == 0,
          "bounded while loop");
    check(run("function main(){let x=5; let total=0; while(x>0){total=total+x;x=x-1;} return total;}") == 15,
          "loop sums");
    check(run("function main(){ let a=7; if(a===7){a=a^3;}else{a=99;}return a;}") == (7^3),
          "if true");
    check(run("function main(){ let a=7; if(a!==7){a=100;}else{a=a+3;}return a;}") == 10,
          "if false");
    check(run("function main(){ if (true){return 1;}else{return 2;} }") == 1,
          "both branches return");
    check(run("function main(){let a=7; if (a<0) {return 12;} return -7;}") == static_cast<std::uint32_t>(-7),
          "signed negative and fallthrough");
    check(run("function main(){return !false;}") == 1, "boolean negation");
    check(run("function main(){return (3>2) === true;}") == 1,
          "typed equality boolean");
    check(run("function main(){return (5 + 3 * 2) ^ 7;}") == (11u ^ 7u),
          "JavaScript operator precedence");
    check(run("function main(){return 0-7;}") == static_cast<std::uint32_t>(-7),
          "two's complement value encoding");
    const auto source = "function main(){let x=3;while(x>0){x=x-1;}return x;}";
    const auto a = make_plan(101), b = make_plan(102);
    const auto p1 = encode(compile_source(source), a);
    const auto p2 = encode(compile_source(source), b);
    check(p1 != p2, "cross-build polymorphism missing");
    check(run_package(p1, a) == 0 && run_package(p2, b) == 0, "cross-build execution");
    rejects([&] { run_package(p1,b); }, "cross-build accepted");
    auto modified = p1; modified[modified.size()-1] ^= 0x20;
    rejects([&] { run_package(modified,a); }, "checksum tamper accepted");
    rejects([&] { run("function main(){while(true){}return 0;}",37,30); },
            "fuel limit bypass");
    rejects([&] { run("function main(){return 2147483647+1;}"); },
            "signed int32 overflow accepted");
    rejects([&] { run("function main(){return 0-2147483647-2;}"); },
            "signed int32 underflow accepted");
    rejects([&] { run("function main(){return 100000*100000;}"); },
            "signed multiply overflow accepted");
    rejects([&] { run("function main(){return 1.5;}"); }, "float accepted");
    rejects([&] { run("function main(){return " + std::string(180, '(') +
                       "1" + std::string(180, ')') + ";}"); },
            "deeply nested expression accepted");
    rejects([&] { run("function main(){return " + std::string(180, '!') + "true;}"); },
            "deeply nested unary expression accepted");
    rejects([&] { run("function main(){return 1; /* unterminated"); },
            "unterminated comment accepted");
    rejects([&] { run("function main(){return 'secret';}"); }, "string accepted");
    rejects([&] { run("function main(){return eval(123);}"); }, "eval accepted");
    rejects([&] { run("function main(){return fetch(3);}"); }, "fetch accepted");
    rejects([&] { run("function main(){return window;}"); }, "global accepted");
    rejects([&] { run("function main(){return true+1;}"); }, "type coercion accepted");
    rejects([&] { run("function main(){let x=1;return x===true;}"); },
            "mismatched equality accepted");
    rejects([&] { run("function main(){const x=1;x=2;return x;}"); },
            "const mutation accepted");
    rejects([&] { run("function main(){let x=x;return x;}"); },
            "uninitialized use accepted");
    rejects([&] { run("function main(){while(0){let y=3;}return y;}"); },
            "escaped lexical scope accepted");
    rejects([&] { run("function main(){if(1){return 1;}}"); },
            "missing return accepted");
    rejects([&] { run("function main(){return 1;return 2;}"); },
            "unreachable statement accepted");
    rejects([&] { run("function main(){return 1;}function other(){return 2;}"); },
            "second function accepted");
    rejects([&] { run("function main(){return host(7,1);}"); },
            "unauthorized host call accepted");
    int calls = 0;
    venom::v4m3::HostPolicy host{{7},[&](std::uint32_t cap, std::uint32_t n) {
      ++calls; check(cap == 7 && n == 3,"host arguments incorrect");return n+5;
    },1};
    check(run("function main(){return host(7,3);}",37,100,host) == 8,
          "allowlisted host call failed");
    check(calls == 1,"host callback count");
    auto none=host;none.allowed.clear();
    rejects([&] {run("function main(){return host(7,3);}",37,100,none);},
            "unapproved host accepted");
    auto noquota=host;noquota.max_calls=0;
    rejects([&] {run("function main(){return host(7,3);}",37,100,noquota);},
            "host budget ignored");
    check(calls==1,"denied host call produced side effects");
    std::mt19937 rng(0x58565635u);
    for(int i=0;i<1000;++i) {
      const int initial=static_cast<int>(rng()%15);
      const int amount=static_cast<int>(rng()%50);
      const int bound=static_cast<int>(rng()%30);
      const std::string loop="function main(){let x="+std::to_string(initial)+
        ";let r="+std::to_string(amount)+
        ";while(x<"+std::to_string(bound)+") {r=r+3; x=x+1;} return r;}";
      const int steps = initial<bound?bound-initial:0;
      check(run(loop,static_cast<std::uint64_t>(i+10)) ==
        static_cast<std::uint32_t>(amount + 3 * steps),"randomized loop differential");
    }
    std::cout << "V4-M5: 1,000 structured-source cases and negative security cases PASS\n";
    return 0;
  } catch(const std::exception& e) {
    std::cerr << "V4-M5 FAILED: " << e.what() << '\n';return 1;
  }
}
