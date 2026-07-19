#include "venom/base/error.hpp"
#include "venom/internal/pipeline/quickjs_wasm_contract.hpp"
#include "venom/pipeline/js.hpp"
#include "venom/generated/contracts/quickjs_wasm_abi.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace venom::compiler {
namespace {
std::uint32_t read_u32(const std::vector<unsigned char>& bytes, std::size_t& pos) {
  std::uint32_t value = 0;
  unsigned shift = 0;
  while (pos < bytes.size()) {
    const auto byte = bytes[pos++];
    value |= static_cast<std::uint32_t>(byte & 0x7fu) << shift;
    if ((byte & 0x80u) == 0u) return value;
    shift += 7u;
    if (shift > 35u) raise_error("VENOM-E5000", "invalid WebAssembly LEB128 value");
  }
  raise_error("VENOM-E5000", "truncated WebAssembly LEB128 value");
}

std::string read_name(const std::vector<unsigned char>& bytes, std::size_t& pos) {
  const auto size = read_u32(bytes, pos);
  if (pos + size > bytes.size()) raise_error("VENOM-E5000", "truncated WebAssembly export name");
  std::string value(reinterpret_cast<const char*>(bytes.data() + pos), size);
  pos += size;
  return value;
}

std::vector<std::string> inspect_exports(const std::vector<unsigned char>& bytes) {
  if (bytes.size() < 8u || bytes[0] != 0x00u || bytes[1] != 0x61u || bytes[2] != 0x73u || bytes[3] != 0x6du) {
    raise_error("VENOM-E5000", "embedded QuickJS runtime is not a WebAssembly module");
  }
  std::size_t pos = 8u;
  std::vector<std::string> exports;
  while (pos < bytes.size()) {
    const auto id = bytes[pos++];
    const auto size = read_u32(bytes, pos);
    const auto end = pos + size;
    if (end > bytes.size()) raise_error("VENOM-E5000", "truncated WebAssembly section");
    if (id == 7u) {
      const auto count = read_u32(bytes, pos);
      exports.reserve(count);
      for (std::uint32_t i = 0; i < count; ++i) {
        exports.push_back(read_name(bytes, pos));
        if (pos >= end) raise_error("VENOM-E5000", "truncated WebAssembly export entry");
        ++pos;
        (void)read_u32(bytes, pos);
      }
      break;
    }
    pos = end;
  }
  return exports;
}
} // namespace

void verify_embedded_quickjs_wasm_contract() {
  const auto exports = inspect_exports(make_quickjs_runtime_wasm_module());
  std::vector<std::string> missing;
  for (const auto required : generated::quickjs_wasm_abi::required_exports) {
    if (std::find(exports.begin(), exports.end(), required) == exports.end()) missing.emplace_back(required);
  }
  if (!missing.empty()) {
    std::ostringstream message;
    message << "embedded QuickJS/WASM ABI mismatch; missing required exports:";
    for (const auto& name : missing) message << ' ' << name;
    message << "; regenerate the runtime from contracts/quickjs-wasm-abi.json";
    raise_error("VENOM-E5000", message.str());
  }
}
} // namespace venom::compiler
