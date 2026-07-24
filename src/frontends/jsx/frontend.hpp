#pragma once

#include <cstddef>
#include <string>

namespace venom::compiler::jsx {

struct LowerResult {
  std::string javascript;
  std::size_t lowered_elements = 0;
};

LowerResult lower(const std::string& source, const std::string& filename);
const char* frontend_name();
const char* frontend_version();

} // namespace venom::compiler::jsx
