#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t*, std::size_t);

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: venom_runtime_contract_replay <input> [input...]\n";
    return 2;
  }
  for (int i = 1; i < argc; ++i) {
    std::ifstream in(argv[i], std::ios::binary);
    if (!in) {
      std::cerr << "failed to open " << argv[i] << "\n";
      return 2;
    }
    const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(in), {});
    (void)LLVMFuzzerTestOneInput(bytes.data(), bytes.size());
  }
  return 0;
}
