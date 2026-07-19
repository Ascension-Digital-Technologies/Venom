#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace venom::generated::typescript {
const std::uint8_t* bridge_bytecode_data() noexcept;
std::size_t bridge_bytecode_size() noexcept;
std::string_view bridge_bytecode_sha256() noexcept;
std::string_view bridge_bytecode_quickjs_version() noexcept;
std::uint32_t bridge_bytecode_abi() noexcept;
} // namespace venom::generated::typescript
