#include "pipeline/native_js_hardener.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

namespace {
std::string read_file(const fs::path& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("unable to open flaky hardener input");
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_result(const fs::path& path, const std::string& payload, std::size_t fallbacks) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output << "VENOM-HARDENER-WORKER-1\n" << fallbacks << '\n' << payload.size() << '\n';
  output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  if (!output) throw std::runtime_error("unable to write flaky hardener output");
}
}

int main(int argc, char** argv) {
  if (argc != 5) return 64;
  try {
    const fs::path output_path = argv[4];
    const fs::path marker_path = output_path.string() + ".fail-once";
    if (!fs::exists(marker_path)) {
      std::ofstream marker(marker_path, std::ios::binary | std::ios::trunc);
      marker << "retry";
      return 99;
    }
    std::error_code ignored;
    fs::remove(marker_path, ignored);
    const auto source = read_file(argv[3]);
    venom::compiler::native_js_hardener::reset_runtime_stats();
    const auto result = venom::compiler::native_js_hardener::harden_in_process(
        argv[1], source, static_cast<std::uint32_t>(std::stoul(argv[2])));
    write_result(output_path, result, venom::compiler::native_js_hardener::runtime_stats().fallbacks);
    venom::compiler::native_js_hardener::shutdown();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 70;
  }
}
