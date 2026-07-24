#include "base/error.hpp"
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
  if (!input) venom::raise_error("VENOM-E5000", "unable to open hardener worker input", venom::ExitCode::build);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const fs::path& path, const std::string& payload, std::size_t fallbacks) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) venom::raise_error("VENOM-E5000", "unable to open hardener worker output", venom::ExitCode::build);
  output << "VENOM-HARDENER-WORKER-1\n" << fallbacks << '\n' << payload.size() << '\n';
  output.write(payload.data(), static_cast<std::streamsize>(payload.size()));
  output.flush();
  if (!output) venom::raise_error("VENOM-E5000", "unable to publish hardener worker output", venom::ExitCode::build);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5) {
    std::cerr << "usage: venom_hardener_worker <kind> <seed> <input> <output>\n";
    return 64;
  }
  try {
    const std::string kind = argv[1];
    const auto seed = static_cast<std::uint32_t>(std::stoul(argv[2]));
    const fs::path input_path = argv[3];
    const fs::path output_path = argv[4];
    const auto source = read_file(input_path);
    venom::compiler::native_js_hardener::reset_runtime_stats();
    const auto hardened = venom::compiler::native_js_hardener::harden_in_process(kind, source, seed);
    const auto stats = venom::compiler::native_js_hardener::runtime_stats();
    write_file(output_path, hardened, stats.fallbacks);
    venom::compiler::native_js_hardener::shutdown();
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "hardener worker failed: " << error.what() << '\n';
    return 70;
  }
}
