#include "venom/package/reader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: venom_package_parser_replay <input> [input...]\n";
    return 2;
  }
  for (int i = 1; i < argc; ++i) {
    std::ifstream in(argv[i], std::ios::binary);
    if (!in) {
      std::cerr << "failed to open " << argv[i] << "\n";
      return 2;
    }
    const std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(in), {});
    try {
      (void)venom::package::read_package_bytes(bytes);
    } catch (const std::exception&) {
      // A malformed corpus item must be rejected cleanly.
    }
  }
  return 0;
}
