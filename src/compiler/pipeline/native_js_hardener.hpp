#pragma once

#include <cstdint>
#include <string>

namespace venom::compiler::native_js_hardener {

std::string harden(const std::string& kind, const std::string& source, std::uint32_t seed);
const char* terser_version();
const char* obfuscator_version();

}  // namespace venom::compiler::native_js_hardener
