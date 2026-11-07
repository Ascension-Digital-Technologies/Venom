#include "ssa.hpp"
#include "../v4m5/frontend.hpp"
#include "../v4m6/cfg.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace vm = venom::v4m3;
namespace js = venom::v4m5;
namespace ssa = venom::v4m7;
void require(bool yes, const char* message) {
  if (!yes) throw std::runtime_error(message);
}
void rejects(const std::function<void()>& callback, const char* message) {
  try { callback(); } catch (const std::runtime_error&) { return; }
  throw std::runtime_error(message);
}
std::uint32_t packaged(const vm::Program& p, std::uint64_t seed = 991,
                       std::uint64_t fuel = 200000,
                       const vm::HostPolicy& host = {}) {
  const auto plan = vm::make_plan(seed);
  return vm::run_package(vm::encode(p, plan), plan, fuel, host);
}
void compare(const std::string& source, std::uint64_t seed) {
  const auto direct = js::compile_source_direct(source);
  const auto graph = ssa::lift(direct);
  ssa::verify(graph);
  const auto output = ssa::lower(graph);
  const auto integrated = js::compile_source(source);
  const auto ref = packaged(direct, seed);
  require(ref == ssa::interpret(graph), "SSA virtual-value interpreter differs");
  require(ref == packaged(output, seed), "SSA canonical backend differs");
  require(ref == packaged(integrated, seed), "integrated frontend differs");
  require(output.size() == integrated.size(), "integrated backend not identical");
  venom::v4m6::verify_compiled_cfg(output);
}
std::string loop_case(std::uint32_t start, std::uint32_t bound,
                      std::uint32_t offset, std::uint32_t seed) {
  return "function main() { let i=" + std::to_string(start) +
         "; let a=" + std::to_string(offset) +
         "; let b=" + std::to_string(seed) +
         "; while (i < " + std::to_string(bound) + ") { "
         "if ((i<3) && (a !== b)) { a=a+3; } else { a=a+1; } "
         "b=b^a; i=i+1; } "
         "return a^b; }";
}
} // namespace

int main() {
  try {
    const std::vector<std::string> cases{
      "function main(){return 42;}",
      "function main(){return false || true;}",
      "function main(){return true && false;}",
      "function main(){let x=0;while(x<7){x=x+1;}return x;}",
      "function main(){let x=7;while(x>0){x=x-1;}return x;}",
      "function main(){let a=7; if(a!==7){return 1;}return 9;}",
      "function main(){let a=9; if(a>3){a=a-1;}else{a=a+1;}return a;}",
      "function main(){let a=4;let b=2;while(a>0){b=b+a; a=a-1;}return b;}",
      "function main(){let a=1;let b=2;let i=0;while(i<5){let t=a;a=b;b=t;i=i+1;}return a;}",
      "function main(){let a=1;while(a<12){if(a<6){a=a+2;}else{a=a+1;}}return a;}",
      "function main(){return (1<2) && (2<3) && !(3<2);}",
      "function main(){const a=7;return (a===7)||(a!==7);}",
      "function main(){let a=0;if(false){a=99;}else{a=42;}return a;}",
      "function main(){let n=0;while(n<10){if(n>7){return n;} n=n+1;}return n;}"
    };
    std::uint64_t seed = 4000;
    for (const auto& source : cases) compare(source, seed++);

    std::mt19937 rng(0x564d3700u);
    for (int i=0;i<1400;++i) {
      const auto a=rng()%12u, b=rng()%22u, c=rng()%200u, d=rng()%200u;
      compare(loop_case(a,b,c,d),static_cast<std::uint64_t>(10000+i));
    }
    auto graph=ssa::lift(js::compile_source_direct(
      "function main(){let a=1;while(a<4){a=a+1;}return a;}"));
    require(graph.blocks.size() > 1, "SSA graph did not split basic blocks");
    require(graph.value_count > graph.blocks.size(), "SSA values not materialized");
    std::size_t phis=0;
    for (const auto& b:graph.blocks) phis+=b.phis.size();
    require(phis>0, "SSA graph has no join phis");

    auto invalid=graph;
    invalid.value_count += 1;
    rejects([&]{ssa::verify(invalid);},"SSA missing value definition accepted");
    invalid=graph;
    invalid.blocks[1].phis[0].incoming[0].second ^= 1u;
    rejects([&]{ssa::verify(invalid);},"SSA wrong incoming definition accepted");
    invalid=graph;
    invalid.blocks[1].phis[0].incoming.clear();
    rejects([&]{ssa::verify(invalid);},"SSA incomplete phi accepted");
    invalid=graph;
    invalid.blocks[1].end.yes=0;
    if(invalid.blocks[1].end.kind != ssa::End::Kind::Return)
      rejects([&]{ssa::verify(invalid);},"SSA branch-to-entry accepted");
    invalid=graph;
    invalid.blocks[0].out[0]=0;
    rejects([&]{ssa::verify(invalid);},"SSA block exit corruption accepted");
    invalid=graph;
    bool changed=false;
    for(auto& block:invalid.blocks) {
      for(auto& step:block.steps) {
        if(step.read_a!=0){step.read_a = step.read_a==1 ? 2 : 1;changed=true;break;}
      }
      if(changed)break;
    }
    require(changed,"SSA graph had no readable inputs");
    rejects([&]{ssa::verify(invalid);},"SSA forged operand accepted");
    invalid=graph;
    invalid.blocks[1].phis[0].result=invalid.blocks[1].phis[1].result;
    rejects([&]{ssa::verify(invalid);},"SSA duplicated value accepted");

    const auto host_source="function main(){return true && (host(7,3)===8);}";
    const auto host_graph=ssa::lift(js::compile_source_direct(host_source));
    int invoked=0;
    vm::HostPolicy policy{{7},[&](std::uint32_t cap,std::uint32_t arg){
      require(cap==7&&arg==3,"host effect arguments");
      ++invoked;
      return std::uint32_t{8};
    },1};
    require(ssa::interpret(host_graph,100000,policy)==1,"SSA permitted host call failed");
    require(packaged(ssa::lower(host_graph),111,100000,policy)==1,
            "SSA lowered host call failed");
    require(invoked==2,"SSA host effect called incorrect number of times");
    rejects([&]{ssa::interpret(host_graph);},"SSA default denied host call allowed");
    auto no_quota=policy;no_quota.max_calls=0;
    rejects([&]{ssa::interpret(host_graph,100000,no_quota);},"SSA host quota bypass");
    require(invoked==2,"SSA denied effect executed callback");

    const auto skip_source="function main(){return false && (host(7,3)===8);}";
    const auto skip_graph=ssa::lift(js::compile_source_direct(skip_source));
    require(ssa::interpret(skip_graph)==0,"SSA short-circuit skipped effect failed");
    require(packaged(ssa::lower(skip_graph))==0,"VM short-circuit skipped effect failed");

    const auto infinite=ssa::lift(js::compile_source_direct(
      "function main(){while(true){}return 2;}"));
    rejects([&]{ssa::interpret(infinite,30);},"SSA fuel exhaustion bypass");
    rejects([&]{packaged(ssa::lower(infinite),111,30);},"lowered loop fuel bypass");
    const auto overflow=ssa::lift(js::compile_source_direct(
      "function main(){return 2147483647+1;}"));
    rejects([&]{ssa::interpret(overflow);},"SSA checked overflow bypass");
    rejects([&]{packaged(ssa::lower(overflow));},"VM checked overflow bypass");

    const auto encoded=vm::encode(ssa::lower(graph),vm::make_plan(1));
    rejects([&]{vm::run_package(encoded,vm::make_plan(2));},
            "SSA backend cross-build envelope accepted");
    std::cout << "V4-M7: 1,400 generated source/SSA/VM differentials, negative SSA and host tests PASS\n";
    return 0;
  } catch(const std::exception& ex) {
    std::cerr << "V4-M7 FAILED: " << ex.what() << '\n';
    return 1;
  }
}
