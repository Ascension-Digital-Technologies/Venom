#pragma once

#include <string_view>

namespace venom::compiler::embedded_js_assets {

std::string_view terser_bundle() noexcept;
std::string_view javascript_obfuscator_bundle() noexcept;

} // namespace venom::compiler::embedded_js_assets
