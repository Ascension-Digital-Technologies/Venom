#include "frontend.hpp"
#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
  try {
    std::uint64_t seed = 37;
    if (argc == 2) seed = std::stoull(argv[1]);
    else if (argc > 2) throw std::runtime_error("usage: venom_v4m5_js [seed]");
    std::string source(std::istreambuf_iterator<char>{std::cin}, {});
    const auto plan = venom::v4m3::make_plan(seed);
    const auto pkg = venom::v4m3::encode(venom::v4m5::compile_source(source), plan);
    const auto result = venom::v4m3::run_package(pkg, plan, 200000);
    std::cout << result << '\n';
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "V4-M5 JS subset rejected: " << ex.what() << '\n';
    return 1;
  }
}
