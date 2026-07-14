#include "compiler/pipeline/js.hpp"
#include "compiler/pipeline/function_dependencies.hpp"
#include "compiler/pipeline/js_discovery.hpp"
#include "compiler/pipeline/js_planning.hpp"
#include "compiler/pipeline/js_rewriting.hpp"
#include "generated/runtime/wasm_runtime_blob.hpp"
#include "generated/runtime/quickjs_runtime_wasm_blob.hpp"
#include "quickjs/bytecode.hpp"
#include "package/hash.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <functional>
#include <tuple>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <unordered_set>


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
  for (const unsigned char c : salt) mix(c);
  for (const unsigned char c : chunk.route) mix(c);
  for (const unsigned char c : chunk.source) mix(c);
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
  if (raw.size() > 0xffffffffu) throw std::runtime_error("QuickJS envelope payload exceeds u32 limits");
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

std::vector<unsigned char> encode_js_bundle(const std::vector<JsChunk>& chunks, const std::string& diversification_salt) {
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

  for (const auto& chunk : chunks) {
    if ((chunk.flags & JsChunkModule) == 0u) continue;
    module_sources.push_back({chunk.source, chunk.source, chunk.code});
  }

  for (const auto& chunk : chunks) {
    Entry entry;
    entry.route_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.route_size = static_cast<std::uint32_t>(chunk.route.size());
    text_blob.insert(text_blob.end(), chunk.route.begin(), chunk.route.end());

    entry.source_offset = static_cast<std::uint32_t>(text_blob.size());
    entry.source_size = static_cast<std::uint32_t>(chunk.source.size());
    text_blob.insert(text_blob.end(), chunk.source.begin(), chunk.source.end());

    const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;
    const bool is_module = (chunk.flags & JsChunkModule) != 0u;
    const bool is_dependency = (chunk.flags & JsChunkDependency) != 0u;
    const bool has_module_requests = is_module && detail::has_module_references(chunk.code);
    std::vector<unsigned char> payload;
    if (browser_side) payload.assign(chunk.code.begin(), chunk.code.end());
    else {
      const auto raw = (is_module && !is_dependency && has_module_requests)
          ? venom::quickjs::compile_native_quickjs_module_bundle(chunk.source, module_sources, false)
          : venom::quickjs::compile_native_quickjs_bytecode(chunk.code, chunk.source, is_module, false, is_module ? &module_sources : nullptr);
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

std::string make_js_diagnostics(const std::vector<JsChunk>& chunks) {
  std::ostringstream out;
  out << "VJSD0006\n";
  out << "version\t1\n";
  out << "count\t" << chunks.size() << "\n";
  out << "order\troute\tkind\tflags\tsource\tbytes\n";
  for (const auto& chunk : chunks) {
    out << chunk.order << '\t'
        << chunk.route << '\t'
        << chunk.kind << '\t'
        << js_flags_summary(chunk.flags) << '\t'
        << chunk.source << '\t'
        << chunk.code.size() << '\n';
  }
  return out.str();
}

std::string json_escape_plan(const std::string& value) {
  std::ostringstream out;
  for (const unsigned char c : value) {
    switch (c) {
      case '\\': out << "\\\\"; break;
      case '"': out << "\\\""; break;
      case '\n': out << "\\n"; break;
      case '\r': out << "\\r"; break;
      case '\t': out << "\\t"; break;
      default:
        if (c < 0x20u) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned int>(c) << std::dec;
        else out << static_cast<char>(c);
    }
  }
  return out.str();
}

std::string make_execution_plan_text(const std::vector<JsChunk>& chunks) {
  std::ostringstream out;
  out << "VENOM_EXECUTION_PLAN_V1\n";
  out << "order\troute\texecution\tconfidence\tsource\treason\n";
  for (const auto& chunk : chunks) {
    out << chunk.order << '\t' << chunk.route << '\t'
        << (((chunk.flags & JsChunkBrowser) != 0u) ? "browser" : "protected") << '\t'
        << chunk.execution_confidence << '\t' << chunk.source << '\t' << chunk.execution_reason << '\n';
  }
  return out.str();
}

std::string make_execution_plan_json(const std::vector<JsChunk>& chunks) {
  std::ostringstream out;
  out << "{\n  \"schema_version\": 1,\n  \"scripts\": [\n";
  for (std::size_t i = 0; i < chunks.size(); ++i) {
    const auto& chunk = chunks[i];
    out << "    {\"order\":" << chunk.order
        << ",\"route\":\"" << json_escape_plan(chunk.route)
        << "\",\"source\":\"" << json_escape_plan(chunk.source)
        << "\",\"execution\":\"" << (((chunk.flags & JsChunkBrowser) != 0u) ? "browser" : "protected")
        << "\",\"confidence\":" << chunk.execution_confidence
        << ",\"reason\":\"" << json_escape_plan(chunk.execution_reason)
        << "\",\"flags\":\"" << json_escape_plan(js_flags_summary(chunk.flags)) << "\"}";
    if (i + 1 != chunks.size()) out << ',';
    out << '\n';
  }
  out << "  ]\n}\n";
  return out.str();
}

} // namespace

std::string js_flags_summary(std::uint32_t flags) {
  std::vector<std::string> names;
  if ((flags & JsChunkInline) != 0) names.push_back("inline");
  if ((flags & JsChunkModule) != 0) names.push_back("module");
  if ((flags & JsChunkDefer) != 0) names.push_back("defer");
  if ((flags & JsChunkAsync) != 0) names.push_back("async");
  if ((flags & JsChunkExternalFile) != 0) names.push_back("external-file");
  if ((flags & JsChunkRemoteUrl) != 0) names.push_back("remote-url");
  if ((flags & JsChunkBytecodeEncoded) != 0) names.push_back("bytecode");
  if ((flags & JsChunkVendoredRemote) != 0) names.push_back("vendored-remote");
  if ((flags & JsChunkDependency) != 0) names.push_back("dependency");
  if ((flags & JsChunkDynamicImport) != 0) names.push_back("dynamic-import");
  if ((flags & JsChunkModulePreload) != 0) names.push_back("modulepreload");
  if ((flags & JsChunkBrowser) != 0) names.push_back("browser");
  if (names.empty()) return "none";
  std::ostringstream out;
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (i != 0) out << ',';
    out << names[i];
  }
  return out.str();
}

JsBridge compile_js_bridge(const SiteGraph& graph, const RemoteVendorOptions& remote_options, bool development) {
  JsBridge bridge;
  bridge.chunks = detail::collect_script_chunks(graph, remote_options, bridge.remote_vendors, bridge.module_edges);
  const auto initial_function_realm_records = detail::apply_function_realm_planning(bridge.chunks);
  (void)initial_function_realm_records;
  detail::close_classic_browser_realms(bridge.chunks);
  const auto module_rewrite_result = detail::apply_protected_module_rewrites(bridge.chunks, remote_options.bridge_id_salt, development);
  const auto function_records = detail::collect_function_realm_records(bridge.chunks);
  const auto extraction_records = detail::analyze_function_extraction(bridge.chunks, function_records);
  auto rewrite_result = detail::apply_bridge_rewrites(bridge.chunks, extraction_records, remote_options.bridge_id_salt);
  if (!module_rewrite_result.registry_source.empty()) {
    if (rewrite_result.registry_source.empty()) rewrite_result.registry_source = module_rewrite_result.registry_source;
    else rewrite_result.registry_source += "\n" + module_rewrite_result.registry_source;
    rewrite_result.records.insert(rewrite_result.records.begin(), module_rewrite_result.records.begin(), module_rewrite_result.records.end());
  }
  bridge.protected_api_typescript = module_rewrite_result.typescript;
  bridge.diagnostics = make_js_diagnostics(bridge.chunks);
  bridge.execution_plan_text = make_execution_plan_text(bridge.chunks);
  bridge.execution_plan_json = make_execution_plan_json(bridge.chunks);
  bridge.function_plan_text = detail::make_function_plan_text(function_records);
  bridge.function_plan_json = detail::make_function_plan_json(function_records);
  bridge.extraction_plan_text = detail::make_extraction_plan_text(extraction_records);
  bridge.extraction_plan_json = detail::make_extraction_plan_json(extraction_records);
  bridge.realm_bridge_contract_json = detail::make_realm_bridge_contract_json(extraction_records);
  bridge.bridge_rewrite_report_json = detail::make_bridge_rewrite_report_json(rewrite_result.records);
  for (const auto& record : rewrite_result.records) {
    if (record.status == "extracted") {
      bridge.bridge_candidate_ids.push_back(record.candidate);
      bridge.protected_exports.emplace_back(record.function, record.candidate);
    }
  }
  if (!rewrite_result.registry_source.empty()) {
    bridge.bridge_registry_bytecode = venom::quickjs::compile_native_quickjs_bytecode(
        rewrite_result.registry_source, "venom://protected-bridge-registry", false, false, nullptr);
  }
  bridge.bundle = encode_js_bundle(bridge.chunks, remote_options.bridge_id_salt);

  std::ostringstream preview;
  for (const auto& chunk : bridge.chunks) {
    preview << "\n// route: " << chunk.route << " source: " << chunk.source
            << " flags: " << js_flags_summary(chunk.flags) << "\n";
    preview << chunk.code << "\n";
  }
  bridge.preview = preview.str();
  return bridge;
}


std::string js_hash64_literal(std::uint64_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

std::string make_loader_js(const std::string& runtime_asset_name, const std::string& package_asset_name, std::uint64_t expected_package_hash, const std::string& expected_package_sha256, bool hardened, const std::string& quickjs_engine_asset_name, const std::string& quickjs_runtime_wasm_asset_name, const std::string& runtime_wasm_asset_name, const std::string& style_asset_name, const std::string& package_binding_token, const std::string& worker_asset_name, const std::string& quickjs_runtime_wasm_sha256, const std::vector<std::pair<std::string, std::string>>& protected_exports,
                           std::uint32_t bridge_invoke_opcode,
                           std::uint32_t bridge_cancel_opcode,
                           std::uint32_t bridge_result_opcode,
                           std::uint32_t bridge_error_opcode) {
  std::ostringstream out;
  out << "import { bootVenom, renderVenomFailure } from './" << runtime_asset_name << "';\n\n";
  out << "const bootOptions = {\n"
      << "  root: document.getElementById('venom-root'),\n"
      << "  packageUrl: new URL('" << package_asset_name << "', import.meta.url).href,\n"
      << "  assetBaseUrl: new URL('" << (hardened ? "../" : "./") << "', import.meta.url).href,\n"
      << "  runtimeUrl: new URL('" << runtime_asset_name << "', import.meta.url).href,\n"
      << "  loaderUrl: import.meta.url,\n"
      << "  expectedPackageHash: '" << js_hash64_literal(expected_package_hash) << "',\n"
      << "  expectedPackageSha256: '" << expected_package_sha256 << "',\n"
      << "  hardened: " << (hardened ? "true" : "false") << ",\n";
  if (!style_asset_name.empty()) out << "  styleUrl: new URL('" << style_asset_name << "', import.meta.url).href,\n";
  if (!runtime_wasm_asset_name.empty()) out << "  runtimeWasmUrl: new URL('" << runtime_wasm_asset_name << "', import.meta.url).href,\n";
  if (!package_binding_token.empty()) out << "  bindingToken: '" << package_binding_token << "',\n";
  if (!quickjs_engine_asset_name.empty()) out << "  quickJsEngineUrl: new URL('" << quickjs_engine_asset_name << "', import.meta.url).href,\n";
  if (!quickjs_runtime_wasm_asset_name.empty()) out << "  quickJsWasmUrl: new URL('" << quickjs_runtime_wasm_asset_name << "', import.meta.url).href,\n";
  if (!quickjs_runtime_wasm_sha256.empty()) out << "  expectedQuickJsWasmSha256: '" << quickjs_runtime_wasm_sha256 << "',\n";
  if (!worker_asset_name.empty()) out << "  workerUrl: new URL('" << worker_asset_name << "', import.meta.url).href,\n";
  out << "};\n";
  if (!worker_asset_name.empty()) {
    out << "const __venomInvokeOp=" << bridge_invoke_opcode << ",__venomCancelOp=" << bridge_cancel_opcode << ",__venomResultOp=" << bridge_result_opcode << ",__venomErrorOp=" << bridge_error_opcode << ";\n";
    out << "let __venomReadyResolve,__venomReadyReject; const __venomReadyPromise=new Promise((resolve,reject)=>{__venomReadyResolve=resolve;__venomReadyReject=reject;});\n"
        << "async function prepareWorkerOwnedRuntime() {\n"
        << "  if (typeof Worker !== 'function') throw new Error('protected release requires Web Worker support');\n"
        << "  const worker = new Worker(bootOptions.workerUrl, { type: 'module', name: 'venom-engine' });\n"
        << "  const token = crypto.getRandomValues(new Uint32Array(4));\n"
        << "  const nonce = Array.from(token, (v) => v.toString(16).padStart(8, '0')).join('');\n"
        << "  const bridgeChannel = new MessageChannel();\n"
        << "  const prepared = await new Promise((resolve, reject) => {\n"
        << "    const timer = setTimeout(() => reject(new Error('worker runtime preparation timed out')), 15000);\n"
        << "    worker.onmessage = (event) => { const m = event.data || {}; if (m.nonce !== nonce) return; clearTimeout(timer); if (m.protocol !== 1) return reject(new Error('worker protocol version mismatch')); if (m.type !== 'ready') return reject(new Error(m.message || 'worker runtime preparation failed')); if (m.packageHash !== bootOptions.expectedPackageHash) return reject(new Error('worker package hash attestation mismatch')); if (m.packageSha256 !== bootOptions.expectedPackageSha256) return reject(new Error('worker package SHA-256 attestation mismatch')); if (m.quickJsWasmSha256 !== bootOptions.expectedQuickJsWasmSha256) return reject(new Error('worker QuickJS WASM attestation mismatch')); if (m.quickJsRuntimeReady !== true) return reject(new Error('worker QuickJS WASM readiness attestation missing')); resolve(m); };\n"
        << "    worker.onerror = (event) => { clearTimeout(timer); reject(new Error(event.message || 'worker startup failed')); };\n"
        << "    worker.postMessage({ protocol: 1, type: 'prepare', nonce, packageUrl: bootOptions.packageUrl, quickJsWasmUrl: bootOptions.quickJsWasmUrl, expectedPackageHash: bootOptions.expectedPackageHash, expectedPackageSha256: bootOptions.expectedPackageSha256, expectedQuickJsWasmSha256: bootOptions.expectedQuickJsWasmSha256, maxPackageBytes: 67108864, maxWasmBytes: 33554432 }, [bridgeChannel.port2]);\n"
        << "  });\n"
        << "  bootOptions.packageBytes = prepared.packageBytes;\n"
        << "  if (prepared.quickJsModule) globalThis.__venomWorkerQuickJsModule = prepared.quickJsModule;\n"
        << "  const pendingBridge=new Map(),protectedExports=new Map(),bridgePort=bridgeChannel.port1;let bridgeSequence=0,bridgeCounter=0;const bridgeGeneration=Number(prepared.bridgeGeneration)>>>0,bridgeKey=Number(prepared.bridgeKey)>>>0;if(!bridgeGeneration||!bridgeKey)throw new Error('worker binary bridge attestation missing');\n"
        << "  const MAX_PUBLIC_PAYLOAD_BYTES=1048576,MAX_PUBLIC_TIMEOUT_MS=30000,DEFAULT_PUBLIC_TIMEOUT_MS=5000,BRIDGE_MAGIC=0x32524256,BRIDGE_HEADER_BYTES=26,BRIDGE_TAG_BYTES=4;const rotateSessionOpcode=(base,lane)=>{let value=((base>>>0)^(bridgeGeneration>>>0)^((bridgeKey<<((lane+3)&15))|(bridgeKey>>>(32-((lane+3)&15)))))>>>0;value=(value+Math.imul(lane+1,0x9e37))&0xffff;return value||((lane+1)*257);};const sessionInvokeOp=rotateSessionOpcode(__venomInvokeOp,0),sessionCancelOp=rotateSessionOpcode(__venomCancelOp,1),sessionResultOp=rotateSessionOpcode(__venomResultOp,2),sessionErrorOp=rotateSessionOpcode(__venomErrorOp,3);if(new Set([sessionInvokeOp,sessionCancelOp,sessionResultOp,sessionErrorOp]).size!==4)throw new Error('worker session opcode collision');const textEncoder=new TextEncoder(),textDecoder=new TextDecoder('utf-8',{fatal:true});\n"
        << "  class VenomBridgeError extends Error{constructor(code,message){super(message);this.name='VenomBridgeError';this.code=code||'BRIDGE_ERROR';}}\n"
        << "  class VenomTimeoutError extends VenomBridgeError{constructor(){super('TIMEOUT','Protected operation timed out');this.name='VenomTimeoutError';}}\n"
        << "  function publicBridgeError(code,message){return new VenomBridgeError(String(code||'BRIDGE_ERROR').toUpperCase().replace(/-/g,'_'),String(message||'Protected operation failed'));}\n"
        << "  function keyedFrameTag(bytes,key){let hash=(2166136261^(key>>>0))>>>0;for(let i=0;i<bytes.length;++i){hash^=bytes[i];hash=Math.imul(hash,16777619)>>>0;}return(hash^((key<<13)|(key>>>19)))>>>0;}\n"
        << "  function encodeFrame(op,counter,capability,requestId,value){const r=textEncoder.encode(String(requestId||'')),p=value===undefined?new Uint8Array(0):textEncoder.encode(JSON.stringify(value));if(r.byteLength>96||p.byteLength>MAX_PUBLIC_PAYLOAD_BYTES)throw new RangeError('bridge frame exceeds protocol limit');const b=new ArrayBuffer(BRIDGE_HEADER_BYTES+r.byteLength+p.byteLength+BRIDGE_TAG_BYTES),v=new DataView(b),a=new Uint8Array(b);v.setUint32(0,BRIDGE_MAGIC,true);v.setUint16(4,1,true);v.setUint16(6,op>>>0,true);v.setUint32(8,bridgeGeneration,true);v.setUint32(12,counter>>>0,true);v.setUint32(16,capability>>>0,true);v.setUint16(20,r.byteLength,true);v.setUint32(22,p.byteLength,true);a.set(r,BRIDGE_HEADER_BYTES);a.set(p,BRIDGE_HEADER_BYTES+r.byteLength);v.setUint32(b.byteLength-BRIDGE_TAG_BYTES,keyedFrameTag(a.subarray(0,b.byteLength-BRIDGE_TAG_BYTES),bridgeKey),true);return b;}\n"
        << "  function decodeFrame(b){if(!(b instanceof ArrayBuffer)||b.byteLength<BRIDGE_HEADER_BYTES+BRIDGE_TAG_BYTES)throw new Error('invalid bridge frame');const v=new DataView(b),a=new Uint8Array(b);if(v.getUint32(0,true)!==BRIDGE_MAGIC||v.getUint16(4,true)!==1||v.getUint32(8,true)!==bridgeGeneration)throw new Error('bridge frame attestation mismatch');const rl=v.getUint16(20,true),pl=v.getUint32(22,true);if(BRIDGE_HEADER_BYTES+rl+pl+BRIDGE_TAG_BYTES!==b.byteLength)throw new Error('bridge frame length mismatch');if(v.getUint32(b.byteLength-BRIDGE_TAG_BYTES,true)!==keyedFrameTag(a.subarray(0,b.byteLength-BRIDGE_TAG_BYTES),bridgeKey))throw new Error('bridge frame integrity mismatch');const id=textDecoder.decode(a.subarray(BRIDGE_HEADER_BYTES,BRIDGE_HEADER_BYTES+rl)),ps=BRIDGE_HEADER_BYTES+rl,t=pl?textDecoder.decode(a.subarray(ps,ps+pl)):'';return{op:v.getUint16(6,true)>>>0,counter:v.getUint32(12,true)>>>0,id,value:t?JSON.parse(t):undefined};}\n"
        << "  function sendFrame(op,counter,capability,id,value){const frame=encodeFrame(op,counter,capability,id,value);bridgePort.postMessage(frame,[frame]);}\n"
        << "  bridgePort.onmessage=(event)=>{let m;try{m=decodeFrame(event.data);}catch(_){return;}const p=pendingBridge.get(m.id);if(!p||m.counter!==p.counter||(m.op!==sessionResultOp&&m.op!==sessionErrorOp))return;pendingBridge.delete(m.id);clearTimeout(p.timer);if(p.abortCleanup)p.abortCleanup();if(m.op===sessionResultOp)p.resolve(m.value);else p.reject(publicBridgeError(m.value&&m.value.code,m.value&&m.value.message));};bridgePort.start();\n"
        << "  function encodeJson(value,label){let encoded;try{encoded=JSON.stringify(value);}catch(error){throw new TypeError(label+' must contain only JSON-serializable values');}if(typeof encoded!=='string')throw new TypeError(label+' must be a JSON value');if(textEncoder.encode(encoded).byteLength>MAX_PUBLIC_PAYLOAD_BYTES)throw new RangeError(label+' exceeds the 1 MiB bridge limit');return JSON.parse(encoded); }\n"
        << "  function baseCapabilityForSlot(slot){return((((__venomInvokeOp>>>0)^0x9e3779b9)+((slot+1)>>>0)*0x85ebca6b)>>>0)|1;}function leaseCapabilityForSlot(slot,counter){let value=(baseCapabilityForSlot(slot)^bridgeGeneration^Math.imul(counter>>>0,0x85ebca6b)^bridgeKey)>>>0;value^=value>>>16;value=Math.imul(value,0x7feb352d)>>>0;value^=value>>>15;return(value|1)>>>0;}\n"
        << "  function invokeCandidate(candidateSlot,args,options){options=options||{};let normalized;try{normalized=encodeJson(Array.isArray(args)?args:[],'Protected arguments');}catch(error){error.code='INVALID_ARGUMENTS';return Promise.reject(error);}candidateSlot=Number(candidateSlot);if(!Number.isInteger(candidateSlot)||candidateSlot<0)return Promise.reject(publicBridgeError('UNKNOWN_EXPORT','Unknown protected export'));const timeout=Math.max(1,Math.min(MAX_PUBLIC_TIMEOUT_MS,Number(options.timeout)||DEFAULT_PUBLIC_TIMEOUT_MS)),signal=options.signal;if(signal&&signal.aborted)return Promise.reject(new DOMException('The protected operation was aborted','AbortError'));return new Promise((resolve,reject)=>{const requestId='v'+Date.now().toString(36)+(++bridgeSequence).toString(36)+crypto.getRandomValues(new Uint32Array(1))[0].toString(36),counter=++bridgeCounter,capability=leaseCapabilityForSlot(candidateSlot,counter);let settled=false;const finish=(fn,value)=>{if(settled)return;settled=true;pendingBridge.delete(requestId);clearTimeout(timer);if(abortCleanup)abortCleanup();fn(value);};const timer=setTimeout(()=>{sendFrame(sessionCancelOp,++bridgeCounter,0,requestId);finish(reject,new VenomTimeoutError());},timeout);let abortCleanup=null;if(signal){const onAbort=()=>{sendFrame(sessionCancelOp,++bridgeCounter,0,requestId);finish(reject,new DOMException('The protected operation was aborted','AbortError'));};signal.addEventListener('abort',onAbort,{once:true});abortCleanup=()=>signal.removeEventListener('abort',onAbort);}pendingBridge.set(requestId,{resolve:(v)=>finish(resolve,v),reject:(e)=>finish(reject,e),timer,abortCleanup,counter});sendFrame(sessionInvokeOp,counter,capability,requestId,normalized);});}\n"
        << "  globalThis.__venomInvokeProtected=(candidateSlot,args,options)=>invokeCandidate(candidateSlot,args,options);\n"
        << "  globalThis.__venomInvokeProtectedByName=(name,args,options)=>{const candidateSlot=protectedExports.get(String(name||''));if(candidateSlot===undefined)return Promise.reject(publicBridgeError('UNKNOWN_EXPORT','Unknown protected export'));return invokeCandidate(candidateSlot,args,options);};\n"
        << "  globalThis.__venomRegisterProtectedExport=(name,candidateSlot)=>{name=String(name||'');candidateSlot=Number(candidateSlot);if(!/^[A-Za-z_$][A-Za-z0-9_$]*$/.test(name)||!Number.isInteger(candidateSlot)||candidateSlot<0)throw new Error('invalid protected export registration');const prior=protectedExports.get(name);if(prior!==undefined&&prior!==candidateSlot)throw new Error('duplicate protected export: '+name);protectedExports.set(name,candidateSlot);};\n";
    for (std::size_t export_index = 0; export_index < protected_exports.size(); ++export_index) {
      const auto& entry = protected_exports[export_index];
      out << "  globalThis.__venomRegisterProtectedExport(\"" << json_escape_plan(entry.first) << "\"," << export_index << ");\n";
    }
    out        << "  const venomApi=Object.freeze({ready:()=>__venomReadyPromise,call:(name,input,options)=>{const candidateSlot=protectedExports.get(String(name||''));if(candidateSlot===undefined)return Promise.reject(publicBridgeError('UNKNOWN_EXPORT','Unknown protected export'));return invokeCandidate(candidateSlot,[input],options);},info:()=>Object.freeze({protocol:1,transport:'binary-capability-v3-leased',valueContract:'json-value-v1',maxPayloadBytes:MAX_PUBLIC_PAYLOAD_BYTES,maxTimeoutMs:MAX_PUBLIC_TIMEOUT_MS,exports:Array.from(protectedExports.keys()).sort()}),exports:new Proxy(Object.create(null),{get:(_,name)=>typeof name==='string'&&protectedExports.has(name)?((input,options)=>venomApi.call(name,input,options)):undefined,has:(_,name)=>typeof name==='string'&&protectedExports.has(name),ownKeys:()=>Array.from(protectedExports.keys()).sort(),getOwnPropertyDescriptor:(_,name)=>typeof name==='string'&&protectedExports.has(name)?{enumerable:true,configurable:true}:undefined,set:()=>false})});\n"
        << "  Object.defineProperty(globalThis,'venom',{value:venomApi,writable:false,configurable:false,enumerable:true});\n"
        << "  __venomReadyResolve(venomApi);\n"
        << "}\n"
        << "prepareWorkerOwnedRuntime().then(() => bootVenom(bootOptions)).catch((error) => { __venomReadyReject(error); console.error('[venom] boot failed', error); renderVenomFailure(bootOptions.root, error); });\n";
  } else {
    out << "bootVenom(bootOptions).catch((error) => { console.error('[venom] boot failed', error); renderVenomFailure(bootOptions.root, error); });\n";
  }
  return out.str();
}

std::string bundle_js_preview(const SiteGraph& graph) {
  std::ostringstream out;
  for (const auto& file : graph.files) {
    if (file.extension == ".js" || file.extension == ".mjs") {
      out << "\n// source: " << file.relative << "\n";
      out.write(reinterpret_cast<const char*>(file.bytes.data()), static_cast<std::streamsize>(file.bytes.size()));
      out << "\n";
    }
  }
  return out.str();
}

} // namespace venom::compiler
