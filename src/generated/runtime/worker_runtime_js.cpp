#include "venom/pipeline/js.hpp"
#include "venom/generated/runtime/worker_runtime_template.hpp"

#include <sstream>

namespace venom::compiler {

std::string make_worker_runtime_js(const std::vector<std::string>& bridge_candidate_ids,
                                   const std::vector<unsigned char>& bridge_registry_bytecode,
                                   const std::vector<WorkerLazyRegistryChunk>& lazy_registry_chunks,
                                   std::uint32_t bridge_invoke_opcode,
                                   std::uint32_t bridge_cancel_opcode,
                                   std::uint32_t bridge_result_opcode,
                                   std::uint32_t bridge_error_opcode) {
  std::ostringstream generated;
  generated << "const BRIDGE_CANDIDATES = Object.freeze([";
  for (std::size_t i = 0; i < bridge_candidate_ids.size(); ++i) {
    if (i) generated << ',';
    generated << '"';
    for (const char c : bridge_candidate_ids[i]) {
      if (c == '\\' || c == '"') generated << '\\';
      generated << c;
    }
    generated << '"';
  }
  generated << "]);\n";
  generated << "const BRIDGE_CAPABILITIES = new Uint32Array([";
  for (std::size_t i = 0; i < bridge_candidate_ids.size(); ++i) {
    if (i) generated << ',';
    const std::uint32_t capability = ((bridge_invoke_opcode ^ 0x9e3779b9u) + static_cast<std::uint32_t>(i + 1u) * 0x85ebca6bu) | 1u;
    generated << capability;
  }
  generated << "]);\n";
  std::uint32_t bridge_integrity_seal = 2166136261u;
  const auto seal_byte = [&bridge_integrity_seal](std::uint8_t value) {
    bridge_integrity_seal ^= value;
    bridge_integrity_seal *= 16777619u;
  };
  for (std::size_t i = 0; i < bridge_candidate_ids.size(); ++i) {
    const std::uint32_t capability = ((bridge_invoke_opcode ^ 0x9e3779b9u) + static_cast<std::uint32_t>(i + 1u) * 0x85ebca6bu) | 1u;
    for (unsigned shift = 0; shift < 32; shift += 8) seal_byte(static_cast<std::uint8_t>((capability >> shift) & 0xffu));
    for (const char raw_c : bridge_candidate_ids[i]) { const auto c = static_cast<unsigned char>(raw_c); seal_byte(c); }
    seal_byte(0xffu);
  }
  for (const char raw_value : bridge_registry_bytecode) { const auto value = static_cast<unsigned char>(raw_value); seal_byte(value); }
  for (const std::uint32_t opcode : {bridge_invoke_opcode, bridge_cancel_opcode, bridge_result_opcode, bridge_error_opcode}) {
    for (unsigned shift = 0; shift < 32; shift += 8) seal_byte(static_cast<std::uint8_t>((opcode >> shift) & 0xffu));
  }

  generated << "const BRIDGE_REGISTRY_BYTECODE = new Uint8Array([";
  for (std::size_t i = 0; i < bridge_registry_bytecode.size(); ++i) {
    if (i) generated << ',';
    generated << static_cast<unsigned int>(bridge_registry_bytecode[i]);
  }
  generated << "]);\n";
  generated << "const LAZY_REGISTRY_CHUNKS=Object.freeze([";
  for (std::size_t i = 0; i < lazy_registry_chunks.size(); ++i) {
    if (i) generated << ',';
    const auto& chunk = lazy_registry_chunks[i];
    generated << "{id:\"" << chunk.id << "\",url:new URL(\"" << chunk.asset
              << "\",import.meta.url).href,sha256:\"" << chunk.sha256 << "\",preload:"
              << (chunk.preload ? "true" : "false") << ",candidates:[";
    for (std::size_t j = 0; j < chunk.candidates.size(); ++j) {
      if (j) generated << ',';
      generated << '"' << chunk.candidates[j] << '"';
    }
    generated << "]}";
  }
  generated << "]);\n";
  generated << "const BRIDGE_INVOKE_OP=" << bridge_invoke_opcode << ";\n";
  generated << "const BRIDGE_CANCEL_OP=" << bridge_cancel_opcode << ";\n";
  generated << "const BRIDGE_RESULT_OP=" << bridge_result_opcode << ";\n";
  generated << "const BRIDGE_ERROR_OP=" << bridge_error_opcode << ";\n";
  generated << "const BRIDGE_INTEGRITY_SEAL=" << bridge_integrity_seal << ";\n";
  generated << generated_runtime::kWorkerRuntimeTemplate;
  return generated.str();
}


} // namespace venom::compiler
