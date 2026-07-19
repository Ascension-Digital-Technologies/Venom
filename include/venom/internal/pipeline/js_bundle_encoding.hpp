#pragma once

#include "venom/pipeline/js.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler {
std::uint32_t js_envelope_seed(const std::string& salt, const JsChunk& chunk);
std::vector<unsigned char> wrap_quickjs_record_for_bundle(const std::vector<unsigned char>& raw,
                                                          const std::string& diversification_salt,
                                                          const JsChunk& chunk);
std::string opaque_js_bytecode_label(const std::string& salt,
                                     const std::string& domain,
                                     const std::string& value);
}
