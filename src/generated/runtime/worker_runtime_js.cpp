#include "compiler/pipeline/js.hpp"

#include <sstream>

namespace venom::compiler {

std::string make_worker_runtime_js(const std::vector<std::string>& bridge_candidate_ids,
                                   const std::vector<unsigned char>& bridge_registry_bytecode,
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
    for (unsigned char c : bridge_candidate_ids[i]) seal_byte(c);
    seal_byte(0xffu);
  }
  for (const unsigned char value : bridge_registry_bytecode) seal_byte(value);
  for (const std::uint32_t opcode : {bridge_invoke_opcode, bridge_cancel_opcode, bridge_result_opcode, bridge_error_opcode}) {
    for (unsigned shift = 0; shift < 32; shift += 8) seal_byte(static_cast<std::uint8_t>((opcode >> shift) & 0xffu));
  }

  generated << "const BRIDGE_REGISTRY_BYTECODE = new Uint8Array([";
  for (std::size_t i = 0; i < bridge_registry_bytecode.size(); ++i) {
    if (i) generated << ',';
    generated << static_cast<unsigned int>(bridge_registry_bytecode[i]);
  }
  generated << "]);\n";
  generated << "const BRIDGE_INVOKE_OP=" << bridge_invoke_opcode << ";\n";
  generated << "const BRIDGE_CANCEL_OP=" << bridge_cancel_opcode << ";\n";
  generated << "const BRIDGE_RESULT_OP=" << bridge_result_opcode << ";\n";
  generated << "const BRIDGE_ERROR_OP=" << bridge_error_opcode << ";\n";
  generated << "const BRIDGE_INTEGRITY_SEAL=" << bridge_integrity_seal << ";\n";
  generated << R"WORKER(const WORKER_PROTOCOL = 1;
const MAX_MESSAGE_BYTES = 1024;
let closed = false;
let prepared = false;
let activeInvocations = 0;
const cancelledRequests = new Set();
let bridgePort = null;
let bridgeGeneration = 0;
let bridgeKey = 0;
let bridgeCounter = 0;
let sessionInvokeOp = 0;
let sessionCancelOp = 0;
let sessionResultOp = 0;
let sessionErrorOp = 0;
let quickJsInstance = null;
let quickJsContext = 0;
const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder('utf-8', { fatal: true });
const MAX_BRIDGE_MESSAGE_BYTES = 1048576;
const MAX_BRIDGE_ARGUMENTS = 64;
const MAX_BRIDGE_CONCURRENCY = 8;
const BRIDGE_MAGIC = 0x32524256;
const BRIDGE_HEADER_BYTES = 26;
const BRIDGE_TAG_BYTES = 4;
function fnv1a32(bytes) { let hash = 2166136261 >>> 0; for (let i = 0; i < bytes.length; ++i) { hash ^= bytes[i]; hash = Math.imul(hash, 16777619) >>> 0; } return hash >>> 0; }
function bridgeIntegritySeal() {
  let hash = 2166136261 >>> 0;
  const mix = (value) => { hash ^= value & 255; hash = Math.imul(hash, 16777619) >>> 0; };
  for (let i = 0; i < BRIDGE_CAPABILITIES.length; ++i) { const capability = BRIDGE_CAPABILITIES[i] >>> 0; for (let shift = 0; shift < 32; shift += 8) mix(capability >>> shift); const name = BRIDGE_CANDIDATES[i] || ''; for (let j = 0; j < name.length; ++j) mix(name.charCodeAt(j)); mix(255); }
  for (let i = 0; i < BRIDGE_REGISTRY_BYTECODE.length; ++i) mix(BRIDGE_REGISTRY_BYTECODE[i]);
  for (const opcode of [BRIDGE_INVOKE_OP, BRIDGE_CANCEL_OP, BRIDGE_RESULT_OP, BRIDGE_ERROR_OP]) for (let shift = 0; shift < 32; shift += 8) mix(opcode >>> shift);
  return hash >>> 0;
}
function assertBridgeIntegrity() { if ((bridgeIntegritySeal() >>> 0) !== (BRIDGE_INTEGRITY_SEAL >>> 0)) throw new Error('bridge integrity seal mismatch'); }
function rotateSessionOpcode(base, lane) { let value = ((base >>> 0) ^ (bridgeGeneration >>> 0) ^ ((bridgeKey << ((lane + 3) & 15)) | (bridgeKey >>> (32 - ((lane + 3) & 15))))) >>> 0; value = (value + Math.imul(lane + 1, 0x9e37)) & 0xffff; return value || ((lane + 1) * 257); }
function deriveLeaseCapability(baseCapability, counter) { let value = (baseCapability ^ bridgeGeneration ^ Math.imul(counter >>> 0, 0x85ebca6b) ^ bridgeKey) >>> 0; value ^= value >>> 16; value = Math.imul(value, 0x7feb352d) >>> 0; value ^= value >>> 15; return (value | 1) >>> 0; }
function refreshSessionProtocol() { sessionInvokeOp = rotateSessionOpcode(BRIDGE_INVOKE_OP, 0); sessionCancelOp = rotateSessionOpcode(BRIDGE_CANCEL_OP, 1); sessionResultOp = rotateSessionOpcode(BRIDGE_RESULT_OP, 2); sessionErrorOp = rotateSessionOpcode(BRIDGE_ERROR_OP, 3); if (new Set([sessionInvokeOp, sessionCancelOp, sessionResultOp, sessionErrorOp]).size !== 4) throw new Error('bridge session opcode collision'); }
assertBridgeIntegrity();
function keyedFrameTag(bytes, key) { let hash = (2166136261 ^ (key >>> 0)) >>> 0; for (let i = 0; i < bytes.length; ++i) { hash ^= bytes[i]; hash = Math.imul(hash, 16777619) >>> 0; } return (hash ^ ((key << 13) | (key >>> 19))) >>> 0; }
function encodeBridgeFrame(op, counter, capability, requestId, value) {
  const requestBytes = textEncoder.encode(String(requestId || ''));
  const payloadBytes = value === undefined ? new Uint8Array(0) : textEncoder.encode(JSON.stringify(value));
  if (requestBytes.byteLength > 96 || payloadBytes.byteLength > MAX_BRIDGE_MESSAGE_BYTES) throw new Error('bridge frame exceeds protocol limit');
  const buffer = new ArrayBuffer(BRIDGE_HEADER_BYTES + requestBytes.byteLength + payloadBytes.byteLength + BRIDGE_TAG_BYTES);
  const view = new DataView(buffer); const bytes = new Uint8Array(buffer);
  view.setUint32(0, BRIDGE_MAGIC, true); view.setUint16(4, WORKER_PROTOCOL, true); view.setUint16(6, op >>> 0, true);
  view.setUint32(8, bridgeGeneration >>> 0, true); view.setUint32(12, counter >>> 0, true); view.setUint32(16, capability >>> 0, true);
  view.setUint16(20, requestBytes.byteLength, true); view.setUint32(22, payloadBytes.byteLength, true);
  bytes.set(requestBytes, BRIDGE_HEADER_BYTES); bytes.set(payloadBytes, BRIDGE_HEADER_BYTES + requestBytes.byteLength);
  view.setUint32(buffer.byteLength - BRIDGE_TAG_BYTES, keyedFrameTag(bytes.subarray(0, buffer.byteLength - BRIDGE_TAG_BYTES), bridgeKey), true);
  return buffer;
}
function decodeBridgeFrame(raw) {
  if (!(raw instanceof ArrayBuffer) || raw.byteLength < BRIDGE_HEADER_BYTES + BRIDGE_TAG_BYTES || raw.byteLength > MAX_BRIDGE_MESSAGE_BYTES + 256) throw new Error('invalid bridge frame');
  const view = new DataView(raw); const bytes = new Uint8Array(raw);
  if (view.getUint32(0, true) !== BRIDGE_MAGIC || view.getUint16(4, true) !== WORKER_PROTOCOL) throw new Error('bridge frame protocol mismatch');
  if (view.getUint32(8, true) !== (bridgeGeneration >>> 0)) throw new Error('bridge frame session mismatch');
  const requestLength = view.getUint16(20, true); const payloadLength = view.getUint32(22, true);
  const expected = BRIDGE_HEADER_BYTES + requestLength + payloadLength + BRIDGE_TAG_BYTES; if (expected !== raw.byteLength) throw new Error('bridge frame length mismatch');
  const actualTag = view.getUint32(raw.byteLength - BRIDGE_TAG_BYTES, true); const expectedTag = keyedFrameTag(bytes.subarray(0, raw.byteLength - BRIDGE_TAG_BYTES), bridgeKey);
  if (actualTag !== expectedTag) throw new Error('bridge frame integrity mismatch');
  const requestId = textDecoder.decode(bytes.subarray(BRIDGE_HEADER_BYTES, BRIDGE_HEADER_BYTES + requestLength));
  const payloadStart = BRIDGE_HEADER_BYTES + requestLength; const payloadText = payloadLength ? textDecoder.decode(bytes.subarray(payloadStart, payloadStart + payloadLength)) : '';
  return { op: view.getUint16(6, true) >>> 0, counter: view.getUint32(12, true) >>> 0, capability: view.getUint32(16, true) >>> 0, requestId, value: payloadText ? JSON.parse(payloadText) : undefined };
}
function sendBridgeFrame(op, counter, capability, requestId, value) { if (!bridgePort) return; const frame = encodeBridgeFrame(op, counter, capability, requestId, value); bridgePort.postMessage(frame, [frame]); }
function bridgeError(requestId, code, message, counter = 0) { sendBridgeFrame(sessionErrorOp, counter, 0, requestId, { code, message }); }
function isJsonValue(value, depth = 0) {
  if (depth > 32) return false;
  if (value === null || typeof value === 'string' || typeof value === 'boolean') return true;
  if (typeof value === 'number') return Number.isFinite(value);
  if (Array.isArray(value)) return value.length <= 4096 && value.every((item) => isJsonValue(item, depth + 1));
  if (typeof value === 'object') {
    const proto = Object.getPrototypeOf(value);
    if (proto !== Object.prototype && proto !== null) return false;
    const keys = Object.keys(value);
    return keys.length <= 4096 && keys.every((key) => isJsonValue(value[key], depth + 1));
  }
  return false;
}
function fail(nonce, error) { postMessage({ protocol: WORKER_PROTOCOL, type: 'error', nonce, message: String(error && error.message ? error.message : error) }); }
function packageHash(buffer) {
  const bytes = new Uint8Array(buffer);
  let hash = 1469598103934665603n;
  const prime = 0x100000001b3n;
  for (let i = 0; i < bytes.length; ++i) {
    hash ^= BigInt(i >= 72 && i < 80 ? 0 : bytes[i]);
    hash = BigInt.asUintN(64, hash * prime);
  }
  return '0x' + hash.toString(16).padStart(16, '0');
}
async function sha256Hex(buffer) {
  const digest = new Uint8Array(await crypto.subtle.digest('SHA-256', buffer));
  return Array.from(digest, (byte) => byte.toString(16).padStart(2, '0')).join('');
}
async function boundedFetch(url, limit, label) {
  const parsed = new URL(url, self.location.href);
  if (parsed.origin !== self.location.origin) throw new Error(label + ' URL must be same-origin');
  const response = await fetch(parsed.href, { cache: 'force-cache', credentials: 'same-origin', redirect: 'error' });
  if (!response.ok) throw new Error(label + ' fetch failed: ' + response.status);
  const declared = Number(response.headers.get('content-length') || 0);
  if (declared && declared > limit) throw new Error(label + ' exceeds declared size limit');
  const bytes = await response.arrayBuffer();
  if (bytes.byteLength > limit) throw new Error(label + ' exceeds size limit');
  return bytes;
}
function secureMemoryRange(exportsObject, ptr, size, label) {
  const memory = exportsObject && exportsObject.memory;
  const start = ptr >>> 0;
  const length = size >>> 0;
  if (!memory || !(memory.buffer instanceof ArrayBuffer)) throw new Error(label + ' memory export is invalid');
  if (!start || !length || start + length < start || start + length > memory.buffer.byteLength) {
    throw new Error(label + ' memory range is invalid');
  }
  return new Uint8Array(memory.buffer, start, length);
}
function secureMemoryWrite(exportsObject, ptr, bytes, label) {
  const source = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
  const view = secureMemoryRange(exportsObject, ptr, source.byteLength, label);
  view.set(source);
  return view;
}
function secureMemoryRead(exportsObject, ptr, size, label) {
  const view = secureMemoryRange(exportsObject, ptr, size, label);
  const copy = view.slice();
  return copy;
}
function secureMemoryZero(exportsObject, ptr, size) {
  if (!ptr || !size) return;
  try { secureMemoryRange(exportsObject, ptr, size, 'zeroize').fill(0); } catch (_) {}
}
function makeQuickJsProbeTable() {
  try { return new WebAssembly.Table({ initial: 4096, element: 'funcref' }); }
  catch (_) { return new WebAssembly.Table({ initial: 4096, element: 'anyfunc' }); }
}
function makeQuickJsProbeGlobal(desc) {
  const value = desc && desc.type && desc.type.value ? desc.type.value : 'i32';
  const mutable = !!(desc && desc.type && desc.type.mutable);
  const initial = value === 'i64' ? 0n : (value === 'f32' || value === 'f64' ? 0.0 : (String(value).includes('ref') ? null : 0));
  return new WebAssembly.Global({ value, mutable }, initial);
}
function makeQuickJsProbeImport(desc) {
  if (!desc || desc.kind === 'function') return function venomQuickJsProbeImport() { return 0; };
  if (desc.kind === 'memory') return new WebAssembly.Memory({ initial: 2048, maximum: 4096 });
  if (desc.kind === 'table') return makeQuickJsProbeTable();
  if (desc.kind === 'global') return makeQuickJsProbeGlobal(desc);
  throw new Error('unsupported QuickJS WASM import kind: ' + String(desc.kind || 'unknown'));
}
function makeQuickJsProbeImports(module) {
  const imports = Object.create(null);
  for (const desc of WebAssembly.Module.imports(module)) {
    const moduleName = String(desc.module || 'env');
    const importName = String(desc.name || '');
    if (!imports[moduleName]) imports[moduleName] = Object.create(null);
    imports[moduleName][importName] = makeQuickJsProbeImport(desc);
  }
  return imports;
}
async function probeQuickJsModule(module) {
  const required = [
    'memory',
    'venom_qjs_context_alloc',
    'venom_qjs_context_free',
    'venom_qjs_context_set_limits',
    'venom_qjs_context_set_execution_limits',
    'venom_qjs_script_buffer_alloc',
    'venom_qjs_script_buffer_capacity',
    'venom_qjs_script_buffer_set_expected_hash',
    'venom_qjs_script_buffer_free',
    'venom_qjs_bytecode_validate',
    'venom_qjs_execute_bytecode',
    'venom_qjs_compact_result_ptr',
    'venom_qjs_diversification_install',
    'venom_qjs_exception_message_ptr',
    'venom_qjs_exception_message_size',
    'venom_qjs_exception_clear',
    'venom_qjs_upstream_quickjs_ready',
    'venom_qjs_bridge_abi',
    'venom_qjs_bridge_input_alloc',
    'venom_qjs_bridge_input_capacity',
    'venom_qjs_bridge_invoke',
    'venom_qjs_bridge_result_ptr',
    'venom_qjs_bridge_result_size',
    'venom_qjs_bridge_release',
  ].sort();
  const requiredKinds = new Map(required.map((name) => [name, name === 'memory' ? 'memory' : 'function']));
  const approvedToolchainExtras = new Map([
    ['__indirect_function_table', 'table'],
    ['_emscripten_stack_restore', 'function'],
    ['emscripten_stack_get_current', 'function'],
  ]);
  const actualEntries = WebAssembly.Module.exports(module).map((entry) => ({
    name: String(entry.name),
    kind: String(entry.kind),
  }));
  const actualKinds = new Map(actualEntries.map((entry) => [entry.name, entry.kind]));
  const missingRequired = required.filter((name) => !actualKinds.has(name));
  const kindMismatches = [];
  for (const [name, kind] of requiredKinds) {
    if (actualKinds.has(name) && actualKinds.get(name) !== kind) kindMismatches.push(name + ':' + actualKinds.get(name) + '!=' + kind);
  }
  for (const [name, kind] of approvedToolchainExtras) {
    if (actualKinds.has(name) && actualKinds.get(name) !== kind) kindMismatches.push(name + ':' + actualKinds.get(name) + '!=' + kind);
  }
  const unapprovedExtras = actualEntries
    .filter((entry) => !requiredKinds.has(entry.name) && !approvedToolchainExtras.has(entry.name))
    .map((entry) => entry.name + ':' + entry.kind)
    .sort();
  if (missingRequired.length || kindMismatches.length || unapprovedExtras.length) {
    throw new Error(
      'QuickJS WASM release export surface mismatch: missing=' + (missingRequired.join(',') || 'none') +
      ' kind_mismatch=' + (kindMismatches.join(',') || 'none') +
      ' unapproved=' + (unapprovedExtras.join(',') || 'none') +
      ' actual=' + actualEntries.map((entry) => entry.name + ':' + entry.kind).sort().join(',')
    );
  }
  let result;
  try {
    result = await WebAssembly.instantiate(module, makeQuickJsProbeImports(module));
  } catch (error) {
    throw new Error('QuickJS WASM instantiate probe failed: ' + String(error && error.message ? error.message : error));
  }
  const instance = result && result.instance ? result.instance : result;
  const e = instance && instance.exports ? instance.exports : {};
  if (typeof e.venom_qjs_upstream_quickjs_ready !== 'function' || (e.venom_qjs_upstream_quickjs_ready() >>> 0) !== 1) {
    throw new Error('QuickJS WASM upstream runtime readiness probe failed');
  }
  const context = e.venom_qjs_context_alloc() >>> 0;
  if (!context) throw new Error('QuickJS WASM context allocation probe failed');
  if (typeof e.venom_qjs_bridge_abi !== 'function' || (e.venom_qjs_bridge_abi() >>> 0) !== 1) { e.venom_qjs_context_free(context); throw new Error('QuickJS WASM protected bridge ABI readiness probe failed'); }
  return { instance, context };
}
self.onmessage = async (event) => {
  if (closed) return;
  const raw = event.data;
  if (raw instanceof ArrayBuffer) {
    let frame; try { frame = decodeBridgeFrame(raw); } catch (_) { return; }
    try { assertBridgeIntegrity(); } catch (_) { closed = true; try { bridgePort && bridgePort.close(); } catch (_) {} return; }
    const op = frame.op, counter = frame.counter, requestId = frame.requestId;
    if (!prepared || !bridgePort || !counter || counter <= bridgeCounter) return bridgeError(requestId, 'replay', 'stale or replayed bridge request', counter);
    bridgeCounter = counter;
    if (op === sessionCancelOp) { if (/^[A-Za-z0-9_-]{8,96}$/.test(requestId)) cancelledRequests.add(requestId); return; }
    if (op !== sessionInvokeOp) return;
    let candidateSlot = -1;
    for (let i = 0; i < BRIDGE_CAPABILITIES.length; ++i) { if ((deriveLeaseCapability(BRIDGE_CAPABILITIES[i] >>> 0, counter) >>> 0) === (frame.capability >>> 0)) { candidateSlot = i; break; } }
    const args = frame.value;
    const candidate = candidateSlot >= 0 && candidateSlot < BRIDGE_CANDIDATES.length ? BRIDGE_CANDIDATES[candidateSlot] : '';
    if (!/^[A-Za-z0-9_-]{8,96}$/.test(requestId)) return bridgeError(requestId, 'invalid-request-id', 'invalid bridge request id', counter);
    if (!candidate) return bridgeError(requestId, 'unknown-capability', 'bridge capability is not declared by the package', counter);
    if (!Array.isArray(args) || args.length > MAX_BRIDGE_ARGUMENTS || !isJsonValue(args)) return bridgeError(requestId, 'invalid-arguments', 'bridge arguments violate json-value-v1', counter);
    if (activeInvocations >= MAX_BRIDGE_CONCURRENCY) return bridgeError(requestId, 'busy', 'bridge concurrency limit reached', counter);
    activeInvocations++;
    try {
      if (!quickJsInstance || !quickJsContext) return bridgeError(requestId, 'executor-not-ready', 'protected QuickJS bridge executor is not ready', counter);
      const e = quickJsInstance.exports;
      const envelope = textEncoder.encode(JSON.stringify({ candidate, args }));
      const capacity = e.venom_qjs_bridge_input_capacity() >>> 0;
      if (!envelope.byteLength || envelope.byteLength > capacity) return bridgeError(requestId, 'input-too-large', 'bridge request exceeds QuickJS input capacity', counter);
      const ptr = e.venom_qjs_bridge_input_alloc(quickJsContext >>> 0, envelope.byteLength >>> 0) >>> 0;
      if (!ptr) return bridgeError(requestId, 'input-allocation-failed', 'QuickJS bridge input allocation failed', counter);
      try {
        secureMemoryWrite(e, ptr, envelope, 'QuickJS bridge input');
        const ok = e.venom_qjs_bridge_invoke(quickJsContext >>> 0, envelope.byteLength >>> 0) >>> 0;
        if (!ok) {
          const ep = e.venom_qjs_exception_message_ptr() >>> 0; const es = e.venom_qjs_exception_message_size() >>> 0; let detail = 'protected bridge invocation failed';
          if (ep && es) detail = textDecoder.decode(secureMemoryRead(e, ep, es, 'QuickJS bridge exception'));
          if (typeof e.venom_qjs_exception_clear === 'function') e.venom_qjs_exception_clear();
          return bridgeError(requestId, detail.includes('not registered') ? 'candidate-not-registered' : 'execution-failed', detail, counter);
        }
        const resultPtr = e.venom_qjs_bridge_result_ptr() >>> 0; const resultSize = e.venom_qjs_bridge_result_size() >>> 0;
        if (!resultPtr || !resultSize) return bridgeError(requestId, 'invalid-result', 'QuickJS bridge returned an invalid result range', counter);
        const resultBytes = secureMemoryRead(e, resultPtr, resultSize, 'QuickJS bridge result');
        const result = JSON.parse(textDecoder.decode(resultBytes));
        resultBytes.fill(0);
        if (!isJsonValue(result)) return bridgeError(requestId, 'invalid-result', 'QuickJS bridge result violates json-value-v1', counter);
        if (!cancelledRequests.has(requestId)) sendBridgeFrame(sessionResultOp, counter, 0, requestId, result);
      } finally {
        secureMemoryZero(e, ptr, envelope.byteLength);
        envelope.fill(0);
        e.venom_qjs_bridge_release(quickJsContext >>> 0);
      }
    } catch (_) { return bridgeError(requestId, 'execution-failed', 'Protected operation failed', counter); }
    finally { cancelledRequests.delete(requestId); activeInvocations--; }
    return;
  }
  const m = raw || {};
  const encodedMessageSize = JSON.stringify(m).length;
  if (encodedMessageSize > MAX_MESSAGE_BYTES) return fail(m.nonce || '', new Error('worker request exceeds protocol limit'));
  if (m.type === 'dispose') {
    if (quickJsInstance && quickJsContext) { try { quickJsInstance.exports.venom_qjs_bridge_release(quickJsContext); quickJsInstance.exports.venom_qjs_context_free(quickJsContext); } catch (_) {} }
    quickJsInstance = null; quickJsContext = 0; closed = true; self.close(); return;
  }
  if (m.protocol !== WORKER_PROTOCOL) return fail(m.nonce || '', new Error('invalid worker protocol version'));
  if (m.type !== 'prepare' || typeof m.nonce !== 'string' || m.nonce.length !== 32 || !event.ports || event.ports.length !== 1) return fail(m.nonce || '', new Error('invalid worker protocol request'));
  if (!/^0x[0-9a-f]{16}$/i.test(String(m.expectedPackageHash || ''))) return fail(m.nonce, new Error('invalid expected package hash'));
  if (!/^[0-9a-f]{64}$/i.test(String(m.expectedPackageSha256 || ''))) return fail(m.nonce, new Error('invalid expected package SHA-256 digest'));
  if (!/^[0-9a-f]{64}$/i.test(String(m.expectedQuickJsWasmSha256 || ''))) return fail(m.nonce, new Error('invalid expected QuickJS WASM digest'));
  try {
    const [packageBytes, wasmBytes] = await Promise.all([
      boundedFetch(String(m.packageUrl || ''), Math.min(Number(m.maxPackageBytes) || 0, 67108864), 'package'),
      boundedFetch(String(m.quickJsWasmUrl || ''), Math.min(Number(m.maxWasmBytes) || 0, 33554432), 'QuickJS WASM'),
    ]);
    const [actualPackageSha256, actualQuickJsWasmSha256] = await Promise.all([
      sha256Hex(packageBytes),
      sha256Hex(wasmBytes),
    ]);
    if (actualPackageSha256 !== String(m.expectedPackageSha256).toLowerCase()) throw new Error('package SHA-256 attestation mismatch');
    const actualPackageHash = packageHash(packageBytes);
    if (actualPackageHash !== String(m.expectedPackageHash).toLowerCase()) throw new Error('package hash attestation mismatch');
    if (actualQuickJsWasmSha256 !== String(m.expectedQuickJsWasmSha256).toLowerCase()) throw new Error('QuickJS WASM digest attestation mismatch');
    const quickJsModule = await WebAssembly.compile(wasmBytes);
    const quickJsProbe = await probeQuickJsModule(quickJsModule);
    quickJsInstance = quickJsProbe.instance;
    quickJsContext = quickJsProbe.context >>> 0;
    if (BRIDGE_REGISTRY_BYTECODE.byteLength) {
      const e = quickJsInstance.exports;
      const ptr = e.venom_qjs_script_buffer_alloc(quickJsContext, BRIDGE_REGISTRY_BYTECODE.byteLength >>> 0) >>> 0;
      const cap = e.venom_qjs_script_buffer_capacity(quickJsContext) >>> 0;
      if (!ptr || BRIDGE_REGISTRY_BYTECODE.byteLength > cap) throw new Error('protected bridge registry allocation failed');
      try {
        new Uint8Array(e.memory.buffer, ptr, BRIDGE_REGISTRY_BYTECODE.byteLength).set(BRIDGE_REGISTRY_BYTECODE);
        e.venom_qjs_script_buffer_set_expected_hash(quickJsContext, fnv1a32(BRIDGE_REGISTRY_BYTECODE));
        if ((e.venom_qjs_execute_bytecode(quickJsContext, BRIDGE_REGISTRY_BYTECODE.byteLength >>> 0) >>> 0) !== 1) {
          const ep = e.venom_qjs_exception_message_ptr() >>> 0;
          const es = e.venom_qjs_exception_message_size() >>> 0;
          let detail = 'protected bridge registry execution failed';
          if (ep && es && ep + es <= e.memory.buffer.byteLength) detail = textDecoder.decode(new Uint8Array(e.memory.buffer, ep, es));
          throw new Error(detail);
        }
      } finally { e.venom_qjs_script_buffer_free(quickJsContext, ptr); }
    }
    bridgePort = event.ports[0];
    bridgeGeneration = crypto.getRandomValues(new Uint32Array(1))[0] || 1;
    bridgeKey = crypto.getRandomValues(new Uint32Array(1))[0] || 0xa5a5a5a5;
    bridgeCounter = 0;
    refreshSessionProtocol();
    bridgePort.onmessage = (bridgeEvent) => { if (!closed) self.onmessage({ data: bridgeEvent.data, ports: [] }); };
    bridgePort.start();
    prepared = true;
    postMessage({ protocol: WORKER_PROTOCOL, type: 'ready', nonce: m.nonce, bridgeGeneration, bridgeKey, bridgeCandidateCount: BRIDGE_CANDIDATES.length, packageHash: actualPackageHash, packageSha256: actualPackageSha256, quickJsWasmSha256: actualQuickJsWasmSha256, quickJsRuntimeReady: true, packageBytes, quickJsModule }, [packageBytes]);
  } catch (error) { fail(m.nonce, error); }
};
)WORKER";
  return generated.str();
}


} // namespace venom::compiler
