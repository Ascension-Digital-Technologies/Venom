#include "typed.hpp"

#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
  try {
    std::uint64_t seed = 37;
    if (argc == 2) seed = std::stoull(argv[1]);
    else if (argc > 2) throw std::runtime_error("usage: venom_v4m8_js [seed]");
    const std::string source(std::istreambuf_iterator<char>{std::cin}, {});
    const auto plan = venom::v4m3::make_plan(seed);
    const auto program = venom::v4m8::compile_source_typed(source);
    const auto bytes = venom::v4m3::encode(program, plan);
    std::cout << venom::v4m3::run_package(bytes, plan) << '\n';
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "V4-M8 typed SSA rejected: " << e.what() << '\n';
    return 1;
  }
}
