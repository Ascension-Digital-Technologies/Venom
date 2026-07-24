#include "package/reader.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
  try {
    (void)venom::package::read_package_bytes(data, size);
  } catch (const std::exception&) {
    // Rejection is the expected outcome for malformed inputs. Sanitizers and
    // the fuzzer still observe crashes, memory errors, hangs, and OOMs.
  } catch (...) {
  }
  return 0;
}
