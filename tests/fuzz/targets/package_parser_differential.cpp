#include "package/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
struct Outcome {
  bool accepted = false;
  std::uint32_t version = 0;
  std::uint32_t runtime_abi = 0;
  std::uint32_t flags = 0;
  std::size_t sections = 0;
};

template <typename F>
Outcome run(F&& fn) {
  try {
    const auto pkg = fn();
    return {true, pkg.version, pkg.runtime_abi, pkg.flags, pkg.sections.size()};
  } catch (...) {
    return {};
  }
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  const std::vector<unsigned char> bytes(data, data + size);
  const auto pointer = run([&] { return venom::package::read_package_bytes(data, size); });
  const auto vector = run([&] { return venom::package::read_package_bytes(bytes); });
  if (pointer.accepted != vector.accepted || pointer.version != vector.version ||
      pointer.runtime_abi != vector.runtime_abi || pointer.flags != vector.flags ||
      pointer.sections != vector.sections) {
    throw std::runtime_error("package parser overload differential mismatch");
  }
  return 0;
}
