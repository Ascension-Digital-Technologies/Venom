#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace venom::generated::typescript {

std::string compiler_source();
std::size_t compiler_source_size() noexcept;
std::string_view compiler_source_sha256() noexcept;
std::string_view compiler_version() noexcept;

} // namespace venom::generated::typescript
