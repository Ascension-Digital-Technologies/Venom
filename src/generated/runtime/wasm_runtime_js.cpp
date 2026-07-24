#include "pipeline/js.hpp"
#include "generated/runtime/wasm_runtime_blob.hpp"
#include "generated/runtime/turbojs_runtime_wasm_blob.hpp"

#include <stdexcept>
#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <string>
#include <utility>

namespace venom::compiler {

namespace {

std::size_t find_js_function_end(const std::string& source, std::size_t start) {
  const auto open = source.find('{', start);
  if (open == std::string::npos) throw std::runtime_error("failed to locate JavaScript function body");
  std::size_t depth = 0;
  bool in_single = false, in_double = false, in_template = false;
  bool line_comment = false, block_comment = false, escape = false;
  for (std::size_t i = open; i < source.size(); ++i) {
    const char c = source[i];
    const char n = i + 1 < source.size() ? source[i + 1] : '\0';
    if (line_comment) { if (c == '\n') line_comment = false; continue; }
    if (block_comment) { if (c == '*' && n == '/') { block_comment = false; ++i; } continue; }
    if (in_single || in_double || in_template) {
      if (escape) { escape = false; continue; }
      if (c == '\\') { escape = true; continue; }
      if (in_single && c == '\'') in_single = false;
      else if (in_double && c == '"') in_double = false;
      else if (in_template && c == '`') in_template = false;
      continue;
    }
    if (c == '/' && n == '/') { line_comment = true; ++i; continue; }
    if (c == '/' && n == '*') { block_comment = true; ++i; continue; }
    if (c == '\'') { in_single = true; continue; }
    if (c == '"') { in_double = true; continue; }
    if (c == '`') { in_template = true; continue; }
    if (c == '{') ++depth;
    else if (c == '}') {
      if (depth == 0) throw std::runtime_error("invalid JavaScript function nesting");
      --depth;
      if (depth == 0) return i + 1;
    }
  }
  throw std::runtime_error("unterminated JavaScript function body");
}

void replace_js_function(std::string& source, const std::string& name, const std::string& replacement) {
  const std::string marker = "function " + name + "(";
  const auto begin = source.find(marker);
  if (begin == std::string::npos) throw std::runtime_error("failed to locate JavaScript function: " + name);
  auto end = find_js_function_end(source, begin);
  while (end < source.size() && (source[end] == '\r' || source[end] == '\n')) ++end;
  source.replace(begin, end - begin, replacement);
}

void remove_js_function(std::string& source, const std::string& name) {
  replace_js_function(source, name, "");
}

void remove_logical_opcode_table(std::string& source) {
  const std::string marker = "const LOGICAL = Object.freeze({";
  const auto begin = source.find(marker);
  if (begin == std::string::npos) throw std::runtime_error("failed to locate JavaScript logical opcode table");
  const auto object_open = source.find('{', begin);
  std::size_t depth = 0;
  std::size_t i = object_open;
  for (; i < source.size(); ++i) {
    if (source[i] == '{') ++depth;
    else if (source[i] == '}') {
      if (--depth == 0) { ++i; break; }
    }
  }
  const auto close = source.find(";", i);
  if (close == std::string::npos) throw std::runtime_error("failed to locate JavaScript logical opcode table terminator");
  source.erase(begin, close + 1 - begin);
}


std::string opaque_runtime_identifier(const venom::vm::PolymorphicPlan& plan,
                                      const std::string& logical_name,
                                      std::unordered_set<std::string>& used) {
  for (std::uint32_t attempt = 0; attempt < 32u; ++attempt) {
    const auto value = venom::vm::derive_diversification_u32(
        plan, "runtime-js-identifier|" + logical_name + "|" + std::to_string(attempt));
    std::ostringstream out;
    out << "v" << std::hex << std::setw(8) << std::setfill('0') << value;
    auto candidate = out.str();
    if (used.insert(candidate).second) return candidate;
  }
  throw std::runtime_error("failed to derive unique protected runtime identifier");
}

bool js_identifier_char(char ch) {
  const auto u = static_cast<unsigned char>(ch);
  return std::isalnum(u) || ch == '_' || ch == '$';
}

void replace_js_identifier(std::string& source, const std::string& from, const std::string& to) {
  std::size_t pos = 0;
  while ((pos = source.find(from, pos)) != std::string::npos) {
    const bool left_ok = pos == 0 || !js_identifier_char(source[pos - 1]);
    const auto right = pos + from.size();
    const bool right_ok = right >= source.size() || !js_identifier_char(source[right]);
    if (left_ok && right_ok) {
      source.replace(pos, from.size(), to);
      pos += to.size();
    } else {
      pos += from.size();
    }
  }
}

void diversify_protected_runtime_surface(std::string& source,
                                         const venom::vm::PolymorphicPlan& plan) {
  static constexpr std::array<const char*, 17> kInternalIdentifiers{{
      "prepareVenomWasmRuntime",
      "wasmMemoryView",
      "installVenomWasmPackageParser",
      "installVenomWasmRouteResolver",
      "installVenomWasmPackageDecoder",
      "runVenomWasmExecution",
      "readDomOpU32",
      "readDomOpString",
      "decodeWasmDomOpStream",
      "applyWasmDomOperations",
      "installWasmNavigation",
      "makeRouteRuntimeLoader",
      "installVenomHostBridge",
      "executeScriptsForRoute",
      "parseReleaseDiversification",
      "parseRuntimePolicy",
      "selectRoute",
  }};
  std::unordered_set<std::string> used;
  for (const auto* logical : kInternalIdentifiers) {
    const auto opaque = opaque_runtime_identifier(plan, logical, used);
    replace_js_identifier(source, logical, opaque);
  }

  // Production builds expose compact, build-specific public diagnostics instead
  // of stable architecture phrases that make cross-build correlation trivial.
  const auto code = [&](const std::string& domain) {
    const auto value = venom::vm::derive_diversification_u32(plan, "runtime-error-code|" + domain);
    std::ostringstream out;
    out << "V-" << std::uppercase << std::hex << std::setw(6) << std::setfill('0') << (value & 0xffffffu);
    return out.str();
  };
  const std::array<std::pair<const char*, std::string>, 6> messages{{
      {"WASM-owned package parse failed", code("package-parse")},
      {"WASM-owned route resolution failed", code("route-resolution")},
      {"WASM runtime execution failed", code("route-execution")},
      {"TurboJS WASM interpreter unavailable; source-decode fallback denied", code("turbojs-unavailable")},
      {"invalid DOM handle authentication tag", code("dom-handle-auth")},
      {"stale route generation rejected", code("stale-generation")},
  }};
  for (const auto& [message, replacement] : messages) {
    std::size_t pos = 0;
    while ((pos = source.find(message, pos)) != std::string::npos) {
      source.replace(pos, std::char_traits<char>::length(message), replacement);
      pos += replacement.size();
    }
  }

  for (const auto* logical : kInternalIdentifiers) {
    if (source.find(logical) != std::string::npos) {
      throw std::runtime_error(std::string("protected runtime retained semantic identifier: ") + logical);
    }
  }
}

void strip_protected_route_vm_js(std::string& source) {
  const std::string script_only_loader = R"JS(function makeRouteRuntimeLoader(pkg, lazySectionsPlan, strictIsolation = false) {
  let activeRuntime = null;
  let routeGeneration = 0;
  if (strictIsolation && (!lazySectionsPlan || !lazySectionsPlan.enabled)) {
    throw new Error('protected release requires route-lazy sections');
  }
  function disposeActive() {
    if (!activeRuntime) return false;
    const disposed = wipeRouteRuntime(activeRuntime);
    activeRuntime = null;
    return disposed;
  }
  function loadRouteRuntime(route) {
    if (!route) throw new Error('no route available');
    if (activeRuntime && activeRuntime.route && activeRuntime.route.route === route.route && !activeRuntime.disposed) return activeRuntime;
    disposeActive();
    routeGeneration = (routeGeneration + 1) >>> 0;
    const scriptInfo = lazySectionsPlan && lazySectionsPlan.enabled ? lazySectionsPlan.scripts.get(route.route) : null;
    const jsSection = scriptInfo ? cloneSectionForRoute(findSection(pkg, SECTION.JAVASCRIPT, scriptInfo.section)) : null;
    const parsedBundle = jsSection ? parseJsBundle(jsSection) : parseJsBundle(findOptionalSection(pkg, SECTION.JAVASCRIPT, 'scripts.vjsb'));
    const jsBundle = Object.freeze({ chunks: parsedBundle.chunks.map((chunk) => Object.freeze({ ...chunk, routeGeneration })) });
    activeRuntime = {
      route,
      jsBundle,
      lazy: Boolean(scriptInfo),
      routeGeneration,
      ownedBuffers: jsSection ? [jsSection.__venomOwnedData] : [],
      disposed: false,
      routeVmOwner: 'wasm'
    };
    return activeRuntime;
  }
  loadRouteRuntime.dispose = disposeActive;
  loadRouteRuntime.generation = () => routeGeneration;
  return Object.freeze(loadRouteRuntime);
}

)JS";
  replace_js_function(source, "makeRouteRuntimeLoader", script_only_loader);
  for (const char* name : {"parseVmBytecode", "routeWithLazyBytecode", "parsePolymorphicMap", "decodeVmInstruction", "createDomMutationBatch", "executeRoute", "installNavigation"}) {
    remove_js_function(source, name);
  }
  remove_logical_opcode_table(source);
  const std::string opcode_integrity_block =
      "  const opcodeMap = parsePolymorphicMap(findSection(pkg, SECTION.INTEGRITY, 'vm-polymorph.vpol'));\n"
      "  activeRuntimeOpcodeMap = opcodeMap;\n"
      "  activeRuntimeIntegritySeal = computeRuntimeIntegritySeal(opcodeMap);\n"
      "  assertRuntimeIntegrity(opcodeMap);\n";
  const auto opcode_pos = source.find(opcode_integrity_block);
  if (opcode_pos == std::string::npos) throw std::runtime_error("failed to replace protected JavaScript opcode-map integrity initialization");
  const std::string wasm_integrity_block =
      "  activeRuntimeOpcodeMap = null;\n"
      "  activeRuntimeIntegritySeal = computeRuntimeIntegritySeal(null);\n"
      "  assertRuntimeIntegrity(null);\n";
  source.replace(opcode_pos, opcode_integrity_block.size(), wasm_integrity_block);
  for (const char* forbidden : {"function decodeVmInstruction", "function executeRoute", "function parseVmBytecode", "function parsePolymorphicMap", "const LOGICAL =", "route-bytecode.vmbc"}) {
    if (source.find(forbidden) != std::string::npos) throw std::runtime_error(std::string("protected runtime retained Route VM JavaScript surface: ") + forbidden);
  }
}

} // namespace


std::vector<unsigned char> make_runtime_wasm_module() {
  return std::vector<unsigned char>(
      kVenomWasmRuntimeBlob,
      kVenomWasmRuntimeBlob + kVenomWasmRuntimeBlobSize);
}

std::string make_runtime_wasm_bridge_js(const SiteGraph& graph,
                                        const std::string& wasm_asset_name,
                                        bool protected_release,
                                        const venom::vm::PolymorphicPlan* diversification) {
  auto js = make_runtime_js(graph, protected_release);
  // Runtime source fragments may use CRLF on Windows checkouts. Normalize before
  // applying structural patches so host line-ending policy cannot break builds.
  js.erase(std::remove(js.begin(), js.end(), '\r'), js.end());
  const std::string helper = R"JS(
const VENOM_WASM_RUNTIME_ASSET = '__VENOM_WASM_ASSET__';
let venomWasmRuntimePromise = null;
let venomWasmLoadedPackageSize = 0;

async function prepareVenomWasmRuntime(options) {
  if (typeof WebAssembly === 'undefined') {
    throw new Error('WebAssembly is not available in this browser');
  }
  if (!venomWasmRuntimePromise) {
    const baseUrl = options && options.assetBaseUrl ? options.assetBaseUrl : new URL('./', options && options.packageUrl ? options.packageUrl : document.baseURI).href;
    const wasmUrl = new URL(VENOM_WASM_RUNTIME_ASSET, baseUrl).href;
    venomWasmRuntimePromise = fetch(wasmUrl, { cache: 'force-cache' })
      .then((response) => {
        if (!response.ok) throw new Error('failed to load WASM runtime: ' + response.status);
        return response.arrayBuffer();
      })
      .then((bytes) => WebAssembly.instantiate(bytes, {}))
      .then((result) => {
        const instance = result.instance || result;
        const e = instance.exports || {};
        if (typeof e.venom_runtime_abi !== 'function') throw new Error('WASM runtime is missing venom_runtime_abi export');
        if (e.venom_runtime_abi() !== 1) throw new Error('WASM runtime ABI mismatch');
        if (typeof e.venom_package_version !== 'function' || e.venom_package_version() !== 40) throw new Error('WASM runtime package-version mismatch');
        const required = ['memory', 'v12_p', 'v12_n', 'v12_x'];
        for (const name of required) {
          if (!e[name]) throw new Error('WASM runtime is missing ' + name + ' export');
        }
        const runtimeFeatures = e.v12_n(12) >>> 0;
        if ((runtimeFeatures & 0x00000001) === 0) {
          throw new Error('WASM runtime is stale: streamed package upload feature is unavailable');
        }
        return instance;
      });
  }
  return venomWasmRuntimePromise;
}

function wasmMemoryView(instance) {
  const memory = instance.exports.memory;
  if (!memory || !(memory.buffer instanceof ArrayBuffer)) throw new Error('WASM runtime memory export is invalid');
  return new Uint8Array(memory.buffer);
}


function installVenomWasmPackageParser(instance) {
  const e = instance.exports;
  venomPackageParser = (bytes, expectedPackageHash) => {
    if (!(bytes instanceof Uint8Array)) throw new Error('WASM package parser received invalid bytes');
    const packagePtr = e.v12_p(0) >>> 0;
    const packageCap = e.v12_n(0) >>> 0;
    const packageSize = bytes.length >>> 0;
    if (packageSize > packageCap) throw new Error('package exceeds WASM parser capacity');
    let rc = e.v12_x(7, packageSize, 0, 0, 0);
    if (rc !== 0) throw new Error('WASM package upload initialization failed: ' + rc);
    const uploadMemory = wasmMemoryView(instance);
    const chunkSize = 64 * 1024;
    for (let offset = 0; offset < packageSize; offset += chunkSize) {
      const end = Math.min(packageSize, offset + chunkSize);
      uploadMemory.set(bytes.subarray(offset, end), packagePtr + offset);
      rc = e.v12_x(8, offset >>> 0, (end - offset) >>> 0, 0, 0);
      if (rc !== 0) { e.v12_x(10, 0, 0, 0, 0); throw new Error('WASM package chunk upload failed: ' + rc); }
    }
    rc = e.v12_x(9, 0, 0, 0, 0);
    venomWasmLoadedPackageSize = packageSize;
    if (rc !== 0) throw new Error('WASM-owned package parse failed: ' + rc);
    const metaPtr = e.v12_p(5) >>> 0;
    const metaSize = e.v12_n(8) >>> 0;
    const metaCap = e.v12_n(7) >>> 0;
    if (metaSize < 80 || metaSize > metaCap) throw new Error('WASM package metadata size is invalid');
    const memory = wasmMemoryView(instance);
    if (metaPtr + metaSize < metaPtr || metaPtr + metaSize > memory.length) throw new Error('WASM package metadata range is invalid');
    const meta = memory.slice(metaPtr, metaPtr + metaSize);
    requireAscii(meta, 'VPKG0002', 'WASM package metadata');
    const mv = new DataView(meta.buffer, meta.byteOffset, meta.byteLength);
    if (readU32(mv, 8) !== 2) throw new Error('unsupported WASM package metadata schema');
    const pkg = {
      bytes: null, byteLength: packageSize, residentWasm: true, view: null, version: readU32(mv, 12), runtimeAbi: readU32(mv, 16), flags: readU32(mv, 20),
      sectionCount: readU32(mv, 24), sectionTableOffset: readU32(mv, 28), sectionTableSize: readU32(mv, 32),
      nameTableOffset: readU32(mv, 36), nameTableSize: readU32(mv, 40), payloadOffset: readU32(mv, 44),
      payloadSize: readU32(mv, 48), packageHash: readHash64(mv, 56), sections: [], decodedSectionCount: 0,
      integrityRequired: false, integrityMap: null, featureDigestMap: null, parserOwner: 'wasm'
    };
    if (readU32(mv, 52) !== packageSize) throw new Error('WASM package metadata file-size mismatch');
    if (expectedPackageHash !== null && expectedPackageHash !== undefined && String(expectedPackageHash).length > 0 && BigInt(String(expectedPackageHash)) !== pkg.packageHash) {
      throw new Error('package hash allowlist mismatch');
    }
    if (pkg.sectionTableSize !== pkg.sectionCount * 48) throw new Error('WASM package metadata table mismatch');
    for (let i = 0; i < pkg.sectionCount; ++i) {
      const base = pkg.sectionTableOffset + i * 48;
      const type = readU32(mv, base), flags = readU32(mv, base + 4), nameOffset = readU32(mv, base + 8), nameSize = readU32(mv, base + 12);
      const dataOffset = readU64(mv, base + 16), dataSize = readU64(mv, base + 24), rawSize = readU64(mv, base + 32), hash = readHash64(mv, base + 40);
      const name = decodeName(meta, pkg.nameTableOffset + nameOffset, nameSize);
      pkg.sections.push(makeLazySection(pkg, i, type, flags, name, dataOffset, dataSize, rawSize, hash, null));
    }
    verifyPackageFeatures(pkg); verifyIntegrityMetadata(pkg);
    bytes.fill(0);
    return pkg;
  };
}


function installVenomWasmRouteResolver(instance) {
  const e = instance.exports;
  venomRouteResolver = (pathname) => {
    const routeBytes = textEncoder.encode(String(pathname || '/'));
    const routePtr = e.v12_p(1) >>> 0;
    const routeCap = e.v12_n(1) >>> 0;
    if (routeBytes.length > routeCap) throw new Error('route exceeds WASM resolver capacity');
    wasmMemoryView(instance).set(routeBytes, routePtr);
    const rc = e.v12_x(2, venomWasmLoadedPackageSize >>> 0, routeBytes.length >>> 0, 0, 0);
    if (rc !== 0) throw new Error('WASM-owned route resolution failed: ' + rc);
    const ptr = e.v12_p(6) >>> 0;
    const size = e.v12_n(10) >>> 0;
    const cap = e.v12_n(9) >>> 0;
    if (size < 48 || size > cap) throw new Error('WASM route record size is invalid');
    const bytes = wasmMemoryView(instance).slice(ptr, ptr + size);
    requireAscii(bytes, 'VROUTE01', 'WASM route record');
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (readU32(view, 8) !== 1) throw new Error('unsupported WASM route record version');
    const routeSize = readU32(view, 40), sourceSize = readU32(view, 44);
    if (48 + routeSize + sourceSize !== size) throw new Error('WASM route record ranges are invalid');
    return Object.freeze({
      route: textDecoder.decode(bytes.slice(48, 48 + routeSize)),
      sourcePath: textDecoder.decode(bytes.slice(48 + routeSize)),
      bytecodeOffset: readU32(view, 24), bytecodeSize: readU32(view, 28),
      instructionCount: readU32(view, 32), flags: readU32(view, 36),
      routeIndex: readU32(view, 12), resolverOwner: 'wasm'
    });
  };
}

function installVenomWasmPackageDecoder(instance) {
  const e = instance.exports;
  let loadedPackage = null;
  venomPackageSectionDecoder = (pkg, sectionIndex, rawSize, name, type) => {
    if (!pkg || pkg.residentWasm !== true || !(pkg.byteLength > 0)) throw new Error('WASM section decoder received a non-resident package');
    if (loadedPackage !== pkg) loadedPackage = pkg;
    const rc = e.v12_x(3, pkg.byteLength >>> 0, sectionIndex >>> 0, type >>> 0, rawSize >>> 0);
    if (rc !== 0) throw new Error('WASM-owned section decode failed for ' + name + ': ' + rc);
    try {
      const size = e.v12_n(6) >>> 0;
      const cap = e.v12_n(5) >>> 0;
      if (size > cap || size !== (rawSize >>> 0)) throw new Error('WASM-owned section decode size mismatch: ' + name);
      const ptr = e.v12_p(4) >>> 0;
      const memory = wasmMemoryView(instance);
      if (ptr + size < ptr || ptr + size > memory.length) throw new Error('WASM-owned section output range is invalid');
      return memory.slice(ptr, ptr + size);
    } finally {
      e.v12_x(5, 0, 0, 0, 0);
    }
  };
}

function runVenomWasmExecution(instance, pkg, routeName) {
  const e = instance.exports;
  if (!pkg || pkg.residentWasm !== true || !(pkg.byteLength > 0)) throw new Error('WASM runtime requires a resident package');
  const routeBytes = textEncoder.encode(routeName || '/');
  const packagePtr = e.v12_p(0);
  const packageCap = e.v12_n(0);
  const routePtr = e.v12_p(1);
  const routeCap = e.v12_n(1);
  if (pkg.byteLength > packageCap) throw new Error('package exceeds WASM runtime package buffer');
  if (routeBytes.length > routeCap) throw new Error('route exceeds WASM runtime route buffer');
  let memory = wasmMemoryView(instance);
  memory.set(routeBytes, routePtr);
  const rc = e.v12_x(4, pkg.byteLength >>> 0, routeBytes.length >>> 0, 0, 0);
  if (rc !== 0) throw new Error('WASM runtime execution failed: ' + rc);
  try {
    memory = wasmMemoryView(instance);
    const resultPtr = e.v12_p(2);
    const resultSize = e.v12_n(2);
    const text = textDecoder.decode(memory.slice(resultPtr, resultPtr + resultSize));
    const report = JSON.parse(text);
    if (report.version !== 40 || report.abi !== 1) throw new Error('WASM runtime report version mismatch');
    const domOpsPtr = e.v12_p(3);
    const domOpsSize = e.v12_n(4);
    const domOpsCap = e.v12_n(3);
    if (domOpsSize > domOpsCap) throw new Error('WASM binary DOM operation stream exceeds capacity');
    const domOpBytes = memory.slice(domOpsPtr, domOpsPtr + domOpsSize);
    const domOps = decodeWasmDomOpStream(domOpBytes);
    if ((report.domOpCount >>> 0) !== domOps.length) throw new Error('WASM binary DOM operation count mismatch');
    report.domOps = domOps;
    report.binaryDomOps = true;
    report.domOpBytes = domOpsSize;
    return Object.freeze(report);
  } finally {
    e.v12_x(6, 0, 0, 0, 0);
  }
}

function readDomOpU32(view, state) {
  if (state.offset + 4 > view.byteLength) throw new Error('truncated WASM binary DOM operation stream');
  const value = view.getUint32(state.offset, true);
  state.offset += 4;
  return value >>> 0;
}

function readDomOpString(bytes, state, size) {
  if (state.offset + size > bytes.length) throw new Error('truncated WASM binary DOM operation payload');
  const text = textDecoder.decode(bytes.slice(state.offset, state.offset + size));
  state.offset += size;
  return text;
}

function decodeWasmDomOpStream(bytes) {
  if (!(bytes instanceof Uint8Array)) throw new Error('WASM binary DOM operation stream is not bytes');
  if (bytes.length < 80) throw new Error('WASM binary DOM operation stream is too small');
  requireAscii(bytes, 'VDOP0020', 'WASM binary DOM operations');
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const state = { offset: 8 };
  const version = readDomOpU32(view, state);
  const count = readDomOpU32(view, state);
  const mapCount = readDomOpU32(view, state);
  if (version !== 2 || mapCount !== 7) throw new Error('unsupported WASM binary DOM operation stream contract');
  const physicalToLogical = new Map();
  for (let i = 0; i < mapCount; ++i) {
    const logical = readDomOpU32(view, state);
    const physical = readDomOpU32(view, state);
    if (logical < 1 || logical > 7 || !physical || physicalToLogical.has(physical)) throw new Error('invalid build-specific DOM command map');
    physicalToLogical.set(physical, logical);
  }
  if (state.offset + 4 > bytes.length) throw new Error('truncated DOM field layout');
  const fieldLayout = [bytes[state.offset], bytes[state.offset + 1], bytes[state.offset + 2]];
  state.offset += 4;
  if (new Set(fieldLayout).size !== 3 || fieldLayout.some((v) => v > 2)) throw new Error('invalid build-specific DOM field layout');
  const ops = [];
  for (let i = 0; i < count; ++i) {
    const physical = readDomOpU32(view, state);
    const logical = physicalToLogical.get(physical);
    if (!logical) throw new Error('unknown build-specific DOM command');
    const physicalLengths = [readDomOpU32(view, state), readDomOpU32(view, state), readDomOpU32(view, state)];
    const fields = ['', '', ''];
    for (let j = 0; j < 3; ++j) fields[fieldLayout[j]] = readDomOpString(bytes, state, physicalLengths[j]);
    const [a, b, c] = fields;
    let item;
    if (logical === 1) item = { op: 'createElement', tag: a };
    else if (logical === 2) item = { op: 'enterElement' };
    else if (logical === 3) item = { op: 'setAttribute', name: a, value: b };
    else if (logical === 4) item = { op: 'bindEvent', attr: a, event: b, handler: c };
    else if (logical === 5) item = { op: 'setText', value: a };
    else if (logical === 6) item = { op: 'leaveElement' };
    else if (logical === 7) item = { op: 'appendChild' };
    else throw new Error('unsupported logical DOM command');
    ops.push(Object.freeze(item));
  }
  if (state.offset !== bytes.length) throw new Error('WASM binary DOM operation stream has trailing bytes');
  return Object.freeze(ops);
}

function applyWasmDomOperations(route, domOps, root, assetManifest = new Map(), assetBaseUrl = document.baseURI, hostBridgePlan = null) {
  if (!route) throw new Error('no route available');
  if (!Array.isArray(domOps)) throw new Error('WASM DOM operation stream is invalid');
  const stack = [root];
  let pending = null;
  let executed = 0;

  root.replaceChildren();
  root.setAttribute('data-venom-route', route.route);
  root.setAttribute('data-venom-runtime', 'wasm-bin-dom');

  for (const item of domOps) {
    const op = item && item.op;
    executed++;
    if (op === 'createElement') {
      if (pending) appendPending(stack, pending);
      pending = document.createElement(item.tag || 'div');
      continue;
    }
    if (op === 'enterElement') {
      if (!pending) throw new Error('WASM DOM enterElement without pending element');
      stack.push(pending);
      pending = null;
      continue;
    }
    if (op === 'setAttribute') {
      const target = pending || currentElement(stack);
      if (!(target instanceof Element)) throw new Error('WASM DOM setAttribute target is not an element');
      const attrName = String(item.name || '');
      let attrValue = String(item.value || '');
      if (shouldRemapAttribute(attrName)) {
        attrValue = attrName === 'srcset' ? resolveSrcset(route, attrValue, assetManifest, assetBaseUrl) : resolveAssetValue(route, attrValue, assetManifest, assetBaseUrl);
      }
      target.setAttribute(attrName, attrValue);
      continue;
    }
    if (op === 'bindEvent') {
      const target = pending || currentElement(stack);
      if (!(target instanceof Element)) throw new Error('WASM DOM bindEvent target is not an element');
      const attrName = item.attr ? String(item.attr) : ('on' + String(item.event || ''));
      bindInlineEventAttribute(target, attrName, String(item.handler || ''), hostBridgePlan);
      continue;
    }
    if (op === 'setText') {
      const text = document.createTextNode(String(item.value || ''));
      if (pending) pending.appendChild(text);
      else stack[stack.length - 1].appendChild(text);
      continue;
    }
    if (op === 'leaveElement') {
      if (stack.length <= 1) throw new Error('WASM DOM leaveElement tried to pop root');
      if (pending) appendPending(stack, pending);
      pending = stack.pop();
      continue;
    }
    if (op === 'appendChild') {
      appendPending(stack, pending);
      pending = null;
      continue;
    }
    throw new Error('unknown WASM DOM operation: ' + op);
  }

  if (pending) appendPending(stack, pending);
  root.setAttribute('data-venom-instructions', String(executed));
  root.setAttribute('data-venom-wasm-dom-ops', String(domOps.length));
}

function installWasmNavigation(wasmRuntime, routes, root, assetManifest, assetBaseUrl, jsBundle, pkg, hostBridgePlan, scriptIsolationPlan = null, scriptPolicyPlan = null, turboJsChunkPlan = null, turboJsEnginePlan = null, scriptEnginePolicyPlan = null) {
  document.addEventListener('click', (event) => {
    const anchor = event.target && event.target.closest ? event.target.closest('a[href]') : null;
    if (!anchor) return;
    const url = new URL(anchor.getAttribute('href'), location.href);
    if (url.origin !== location.origin) return;
    const targetRoute = selectRoute(routes, url.pathname);
    if (!targetRoute || normalizeRoute(url.pathname) !== targetRoute.route) return;
    event.preventDefault();
    history.pushState({ venomRoute: targetRoute.route }, '', url.pathname);
    const bridge = globalThis.__venomRuntime || null;
    if (bridge && typeof bridge.teardownRoute === 'function') bridge.teardownRoute('wasm-navigation');
    const nextGeneration = bridge && typeof bridge.currentRouteGeneration === 'function' ? ((bridge.currentRouteGeneration() + 1) >>> 0) : 1;
    if (bridge && typeof bridge.activateRouteGeneration === 'function') bridge.activateRouteGeneration(nextGeneration);
    setActiveBrowserAssetRoute(targetRoute);
    const wasmExecution = runVenomWasmExecution(wasmRuntime, pkg, targetRoute.route);
    applyWasmDomOperations(targetRoute, wasmExecution.domOps, root, assetManifest, assetBaseUrl, hostBridgePlan);
    root.setAttribute('data-venom-wasm-runtime', 'executed');
    root.setAttribute('data-venom-wasm-executed', String(wasmExecution.executed));
    root.setAttribute('data-venom-wasm-dom-bytes', String(wasmExecution.domOpBytes || 0));
    executeScriptsForRoute(targetRoute, jsBundle, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan).catch((error) => console.error('[venom] route script failed', error));
  });

  window.addEventListener('popstate', () => {
    const targetRoute = selectRoute(routes, location.pathname);
    const bridge = globalThis.__venomRuntime || null;
    if (bridge && typeof bridge.teardownRoute === 'function') bridge.teardownRoute('wasm-history-navigation');
    const nextGeneration = bridge && typeof bridge.currentRouteGeneration === 'function' ? ((bridge.currentRouteGeneration() + 1) >>> 0) : 1;
    if (bridge && typeof bridge.activateRouteGeneration === 'function') bridge.activateRouteGeneration(nextGeneration);
    setActiveBrowserAssetRoute(targetRoute);
    const wasmExecution = runVenomWasmExecution(wasmRuntime, pkg, targetRoute.route);
    applyWasmDomOperations(targetRoute, wasmExecution.domOps, root, assetManifest, assetBaseUrl, hostBridgePlan);
    root.setAttribute('data-venom-wasm-runtime', 'executed');
    root.setAttribute('data-venom-wasm-executed', String(wasmExecution.executed));
    root.setAttribute('data-venom-wasm-dom-bytes', String(wasmExecution.domOpBytes || 0));
    executeScriptsForRoute(targetRoute, jsBundle, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan).catch((error) => console.error('[venom] route script failed', error));
  });
}
)JS";
  auto patched_helper = helper;
  const auto placeholder = std::string("__VENOM_WASM_ASSET__");
  const auto pos_asset = patched_helper.find(placeholder);
  if (pos_asset != std::string::npos) {
    patched_helper.replace(pos_asset, placeholder.size(), wasm_asset_name);
  }
  js.insert(0, patched_helper + "\n");
  const std::string needle = "export async function bootVenom(options) {\n";
  const auto pos = js.find(needle);
  if (pos == std::string::npos) {
    throw std::runtime_error("failed to patch WASM runtime bridge boot hook");
  }
  js.replace(pos, needle.size(), needle + "  const wasmRuntime = await prepareVenomWasmRuntime(options || {});\n  installVenomWasmPackageParser(wasmRuntime);\n  installVenomWasmRouteResolver(wasmRuntime);\n  installVenomWasmPackageDecoder(wasmRuntime);\n");

  const auto boot_begin = pos;
  const auto boot_end = find_js_function_end(js, boot_begin);
  auto find_boot_line = [&](const std::string& prefix, const char* error) {
    const auto line_begin = js.find(prefix, boot_begin);
    if (line_begin == std::string::npos || line_begin >= boot_end) {
      throw std::runtime_error(error);
    }
    const auto line_end = js.find('\n', line_begin);
    if (line_end == std::string::npos || line_end > boot_end) {
      throw std::runtime_error(error);
    }
    return std::pair<std::size_t, std::size_t>{line_begin, line_end + 1};
  };

  // Patch around stable statement boundaries instead of matching the complete
  // host-bridge argument list. That list grows frequently and must not become
  // an implicit ABI for the WASM runtime generator.
  const auto [route_begin, route_end] = find_boot_line(
      "  const route = selectRoute(routes, location.pathname);",
      "failed to patch WASM runtime execution hook");
  js.insert(route_end,
      "  const wasmExecution = runVenomWasmExecution(wasmRuntime, pkg, route.route);\n");

  const auto [execute_begin, execute_end] = find_boot_line(
      "  executeRoute(initialRuntime.route,",
      "failed to patch WASM runtime result hook");
  js.replace(execute_begin, execute_end - execute_begin,
      "  applyWasmDomOperations(route, wasmExecution.domOps, root, assetManifest, assetBaseUrl, hostBridgePlan);\n"
      "  root.setAttribute('data-venom-wasm-runtime', 'executed');\n"
      "  root.setAttribute('data-venom-wasm-executed', String(wasmExecution.executed));\n"
      "  root.setAttribute('data-venom-wasm-dom-bytes', String(wasmExecution.domOpBytes || 0));\n");

  const auto [nav_begin, nav_end] = find_boot_line(
      "  installNavigation(routes,",
      "failed to patch WASM runtime navigation hook");
  js.replace(nav_begin, nav_end - nav_begin,
      "  installWasmNavigation(wasmRuntime, routes, root, assetManifest, assetBaseUrl, initialRuntime.jsBundle, pkg, hostBridgePlan, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan);\n");

  const std::string mode_needle = "    runtimeMode: 'js',\n";
  const auto mode_pos = js.find(mode_needle);
  if (mode_pos != std::string::npos) {
    js.replace(mode_pos, mode_needle.size(), "    runtimeMode: 'wasm',\n    wasmRuntimeAbi: wasmRuntime.exports.venom_runtime_abi(),\n    wasmExecution,\n");
  }
  if (protected_release) {
    strip_protected_route_vm_js(js);
    if (!diversification || !diversification->enabled) {
      throw std::runtime_error("protected runtime requires an enabled diversification plan");
    }
    diversify_protected_runtime_surface(js, *diversification);
  }
  return js;
}

std::vector<unsigned char> make_turbojs_runtime_wasm_module() {
  return std::vector<unsigned char>(
      kTurboJsRuntimeWasmBlob,
      kTurboJsRuntimeWasmBlob + kTurboJsRuntimeWasmBlobSize);
}

std::string turbojs_runtime_wasm_provenance_text() {
  return std::string(kTurboJsRuntimeWasmBlobProvenance ? kTurboJsRuntimeWasmBlobProvenance : "");
}

std::string turbojs_runtime_wasm_sha256() {
  return std::string(kTurboJsRuntimeWasmBlobSha256 ? kTurboJsRuntimeWasmBlobSha256 : "");
}


} // namespace venom::compiler
