#pragma once

#include "pipeline/js.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace venom::compiler {
std::uint32_t js_envelope_seed(const std::string& salt, const JsChunk& chunk);
std::vector<unsigned char> wrap_turbojs_record_for_bundle(const std::vector<unsigned char>& raw,
                                                          const std::string& diversification_salt,
                                                          const JsChunk& chunk);
// Same per-build VTJSE006 permutation envelope as wrap_turbojs_record_for_bundle,
// specialized for lazy protected registry chunks that have no JsChunk (they are
// identified by domain/id, e.g. domain="registry-chunk", id=chunk.id).
std::vector<unsigned char> wrap_turbojs_record_for_registry_chunk(const std::vector<unsigned char>& raw,
                                                                   const std::string& diversification_salt,
                                                                   const std::string& domain,
                                                                   const std::string& id);
std::string opaque_js_bytecode_label(const std::string& salt,
                                     const std::string& domain,
                                     const std::string& value);
}
