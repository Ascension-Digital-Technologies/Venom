#include "venom/base/error.hpp"
#include "js_bundle_encoding.hpp"
#include "venom/graph/module_graph.hpp"
#include "js_discovery.hpp"
#include "js_rewriting.hpp"
#include "native_js_hardener.hpp"
#include "venom/package/hash.hpp"
#include "venom/quickjs/bytecode.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace venom::compiler {
namespace {
constexpr unsigned char kQuickJsEnvelopeMagic[8] = {'V','Q','J','S','E','0','0','6'}; // VQJSE006

void append_u32(std::vector<unsigned char>& out, std::uint32_t value) {
  out.push_back(static_cast<unsigned char>(value & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 8u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 16u) & 0xffu));
  out.push_back(static_cast<unsigned char>((value >> 24u) & 0xffu));
}

std::uint32_t envelope_seed(const std::string& salt, const JsChunk& chunk) {
  std::uint32_t h = 2166136261u;
  const auto mix = [&](unsigned char value) { h ^= value; h *= 16777619u; };
  for (const char raw_c : salt) { const auto c = static_cast<unsigned char>(raw_c); mix(c); }
  for (const char raw_c : chunk.route) { const auto c = static_cast<unsigned char>(raw_c); mix(c); }
  for (const char raw_c : chunk.source) { const auto c = static_cast<unsigned char>(raw_c); mix(c); }
  for (int shift = 0; shift < 32; shift += 8) mix(static_cast<unsigned char>((chunk.order >> shift) & 0xffu));
  h ^= 0xA5C31F27u;
  return h ? h : 0x6D2B79F5u;
}

std::uint32_t xorshift32(std::uint32_t& state) {
  state ^= state << 13u;
  state ^= state >> 17u;
  state ^= state << 5u;
  return state;
}

std::array<unsigned char, 16> bytecode_lane_map(std::uint32_t seed) {
  std::array<unsigned char, 16> map{};
  for (unsigned char i = 0; i < map.size(); ++i) map[i] = i;
  std::uint32_t state = seed ^ 0xC6EF3720u;
  for (std::size_t i = map.size() - 1; i > 0; --i) {
    const std::size_t j = xorshift32(state) % (i + 1u);
    std::swap(map[i], map[j]);
  }
  return map;
}

std::uint32_t bytecode_lane_fingerprint(const std::array<unsigned char, 16>& map) {
  std::uint32_t h = 2166136261u;
  for (const auto value : map) { h ^= value; h *= 16777619u; }
  return h;
}

std::vector<unsigned char> wrap_quickjs_record(const std::vector<unsigned char>& raw,
                                                const std::string& salt,
                                                const JsChunk& chunk) {
  if (raw.size() > 0xffffffffu) raise_error("VENOM-E5000", "QuickJS envelope payload exceeds u32 limits");
  constexpr std::uint32_t header_size = 48u;
  constexpr std::uint32_t lane_width = 16u;
  const std::uint32_t seed = envelope_seed(salt, chunk);
  const auto lane_map = bytecode_lane_map(seed);
  const std::uint32_t lane_fingerprint = bytecode_lane_fingerprint(lane_map);
  const std::uint64_t raw_hash = venom::package::fnv1a64(raw);
  std::vector<unsigned char> out;
  out.reserve(header_size + raw.size());
  out.insert(out.end(), std::begin(kQuickJsEnvelopeMagic), std::end(kQuickJsEnvelopeMagic));
  append_u32(out, 2u);
  append_u32(out, static_cast<std::uint32_t>(raw.size()));
  append_u32(out, seed);
  append_u32(out, 0x01000300u); // QuickJS bytecode ABI fingerprint.
  for (int i = 0; i < 8; ++i) out.push_back(static_cast<unsigned char>((raw_hash >> (i * 8)) & 0xffu));
  append_u32(out, header_size);
  append_u32(out, chunk.flags);
  append_u32(out, lane_width);
  append_u32(out, lane_fingerprint);
  std::uint32_t stream = seed ^ 0x9E3779B9u;
  for (std::size_t block = 0; block < raw.size(); block += lane_width) {
    const std::size_t block_size = std::min<std::size_t>(lane_width, raw.size() - block);
    std::array<unsigned char, lane_width> block_map{};
    std::size_t block_map_size = 0;
    for (const auto lane : lane_map) if (lane < block_size) block_map[block_map_size++] = lane;
    for (std::size_t stored_lane = 0; stored_lane < block_size; ++stored_lane) {
      const std::size_t source_lane = block_map[stored_lane];
      const unsigned char value = raw[block + source_lane];
      if (((block + stored_lane) & 3u) == 0u) xorshift32(stream);
      const auto mask = static_cast<unsigned char>((stream >> (((block + stored_lane) & 3u) * 8u)) & 0xffu);
      out.push_back(static_cast<unsigned char>(value ^ mask));
    }
  }
  return out;
}

std::string opaque_bytecode_label(const std::string& salt, const std::string& domain, const std::string& value) {
  std::vector<unsigned char> material;
  material.insert(material.end(), salt.begin(), salt.end());
  material.push_back(0u);
  material.insert(material.end(), domain.begin(), domain.end());
  material.push_back(0u);
  material.insert(material.end(), value.begin(), value.end());
  const auto digest = venom::package::sha256_hex(material);
  return domain.substr(0, 1) + "/" + digest.substr(0, 20);
}



} // namespace

std::uint32_t js_envelope_seed(const std::string& salt, const JsChunk& chunk) {
  return envelope_seed(salt, chunk);
}

std::vector<unsigned char> wrap_quickjs_record_for_bundle(const std::vector<unsigned char>& raw,
                                                          const std::string& diversification_salt,
                                                          const JsChunk& chunk) {
  return wrap_quickjs_record(raw, diversification_salt, chunk);
}

std::string opaque_js_bytecode_label(const std::string& salt,
                                     const std::string& domain,
                                     const std::string& value) {
  return opaque_bytecode_label(salt, domain, value);
}

std::vector<unsigned char> encode_js_bundle(const std::vector<JsChunk>& chunks, const graph::CanonicalModuleGraph& module_graph, const std::string& diversification_salt, bool development) {
  struct Entry {
    std::uint32_t route_offset = 0;
    std::uint32_t route_size = 0;
    std::uint32_t source_offset = 0;
    std::uint32_t source_size = 0;
    std::uint32_t code_offset = 0;
    std::uint32_t code_size = 0;
    std::uint32_t order = 0;
    std::uint32_t flags = 0;
    std::uint32_t kind = 0;
    std::uint32_t reserved = 0;
  };

  std::vector<Entry> entries;
  std::vector<unsigned char> text_blob;
  std::vector<unsigned char> code_blob;
  std::vector<venom::quickjs::ModuleSourceRecord> module_sources;
  entries.reserve(chunks.size());
  module_sources.reserve(chunks.size());

  std::vector<std::string> stored_sources;
  stored_sources.reserve(chunks.size());
  for (const auto& chunk : chunks) {
    const auto* module = module_graph.find_by_source(chunk.route, chunk.source);
    if (module == nullptr) raise_error("VENOM-E5000", "canonical module graph is missing source: " + chunk.source);
    stored_sources.push_back(module->runtime_source);
  }

  std::vector<std::string> compiled_sources;
  compiled_sources.reserve(chunks.size());
  for (const auto& chunk : chunks) {
    const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    if (!development && !browser_side) {
      compiled_sources.push_back(native_js_hardener::harden(
          is_module ? "protected-module" : "protected-script",
          chunk.code, envelope_seed(diversification_salt, chunk)));
    } else if (browser_side && is_module) {
      const auto* module = module_graph.find_by_source(chunk.route, chunk.source);
      if (module == nullptr) raise_error("VENOM-E5000", "canonical module graph is missing browser source: " + chunk.source);
      compiled_sources.push_back(graph::browser_module_identity_prefix(*module) + graph::browser_module_map_prefix(module_graph, *module) + chunk.code);
    } else {
      compiled_sources.push_back(chunk.code);
    }
  }

  for (std::size_t i = 0; i < chunks.size(); ++i) {
    const auto& chunk = chunks[i];
    if ((chunk.flags & JsChunkModule) == 0u) continue;
    const std::string module_name = "venom://" + stored_sources[i];
    module_sources.push_back({chunk.source, module_name, compiled_sources[i]});
  }

  for (std::size_t chunk_index = 0; chunk_index < chunks.size(); ++chunk_index) {
    const auto& chunk = chunks[chunk_index];
    Entry entry;
    const std::string stored_route = development ? chunk.route : opaque_bytecode_label(diversification_salt, "route", chunk.route);
    const std::string& stored_source = stored_sources[chunk_index];
    entry.route_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.route_size = static_cast<std::uint32_t>(stored_route.size());
    text_blob.insert(text_blob.end(), stored_route.begin(), stored_route.end());

    entry.source_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.source_size = static_cast<std::uint32_t>(stored_source.size());
    text_blob.insert(text_blob.end(), stored_source.begin(), stored_source.end());

    const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    const bool is_dependency = (chunk.flags & JsChunkDependency) != 0u;
    const bool has_module_requests = is_module && detail::has_module_references(chunk.code);
    std::vector<unsigned char> payload;
    const auto& compile_source = compiled_sources[chunk_index];
    const std::string compile_name = is_module ? ("venom://" + stored_source) : stored_source;
    if (browser_side) payload.assign(compile_source.begin(), compile_source.end());
    else {
      const auto raw = (is_module && !is_dependency && has_module_requests)
          ? venom::quickjs::compile_native_quickjs_module_bundle(compile_name, module_sources, true)
          : venom::quickjs::compile_native_quickjs_bytecode(compile_source, compile_name, is_module, true, is_module ? &module_sources : nullptr);
      payload = wrap_quickjs_record(raw, diversification_salt, chunk);
    }
    entry.code_offset = static_cast<std::uint32_t>(code_blob.size());
    entry.code_size = static_cast<std::uint32_t>(payload.size());
    code_blob.insert(code_blob.end(), payload.begin(), payload.end());
    entry.order = chunk.order;
    entry.flags = browser_side ? chunk.flags : (chunk.flags | JsChunkBytecodeEncoded);
    entry.kind = chunk.kind;
    entries.push_back(entry);
  }

  std::vector<unsigned char> out;
  out.insert(out.end(), {'V', 'J', 'S', 'B', '0', '0', '0', '6'});
  append_u32(out, 1);
  append_u32(out, static_cast<std::uint32_t>(entries.size()));
  append_u32(out, static_cast<std::uint32_t>(text_blob.size()));
  append_u32(out, 0);

  for (const auto& entry : entries) {
    append_u32(out, entry.route_offset);
    append_u32(out, entry.route_size);
    append_u32(out, entry.source_offset);
    append_u32(out, entry.source_size);
    append_u32(out, entry.code_offset);
    append_u32(out, entry.code_size);
    append_u32(out, entry.order);
    append_u32(out, entry.flags);
    append_u32(out, entry.kind);
    append_u32(out, entry.reserved);
  }
  out.insert(out.end(), text_blob.begin(), text_blob.end());
  out.insert(out.end(), code_blob.begin(), code_blob.end());
  return out;
}

} // namespace venom::compiler
