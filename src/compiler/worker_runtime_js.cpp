#include "compiler/js.hpp"

#include <sstream>

namespace venom::compiler {

std::string make_worker_runtime_js(const std::vector<std::string>& bridge_candidate_ids) {
  std::ostringstream generated;
  generated << "const BRIDGE_CANDIDATES = new Set([";
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
  generated << R"WORKER(const WORKER_PROTOCOL = 1;
const MAX_MESSAGE_BYTES = 1024;
let closed = false;
let prepared = false;
let activeInvocations = 0;
let quickJsInstance = null;
let quickJsContext = 0;
const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder('utf-8', { fatal: true });
const MAX_BRIDGE_MESSAGE_BYTES = 65536;
const MAX_BRIDGE_ARGUMENTS = 64;
const MAX_BRIDGE_CONCURRENCY = 8;
function bridgeError(requestId, code, message) {
  postMessage({ protocol: WORKER_PROTOCOL, type: 'bridge-error', requestId, code, message });
}
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
  const m = event.data || {};
  const encodedMessageSize = JSON.stringify(m).length;
  if (m.type !== 'invoke' && encodedMessageSize > MAX_MESSAGE_BYTES) return fail(m.nonce || '', new Error('worker request exceeds protocol limit'));
  if (m.type === 'dispose') {
    if (quickJsInstance && quickJsContext) { try { quickJsInstance.exports.venom_qjs_bridge_release(quickJsContext); quickJsInstance.exports.venom_qjs_context_free(quickJsContext); } catch (_) {} }
    quickJsInstance = null; quickJsContext = 0; closed = true; self.close(); return;
  }
  if (m.protocol !== WORKER_PROTOCOL) return fail(m.nonce || '', new Error('invalid worker protocol version'));
  if (m.type === 'bridge-capabilities') {
    postMessage({ protocol: WORKER_PROTOCOL, type: 'bridge-capabilities', requestId: String(m.requestId || ''), prepared, candidates: Array.from(BRIDGE_CANDIDATES).sort(), transport: 'worker-message-v1', valueContract: 'json-value-v1', synchronousCalls: false, executorReady: !!(quickJsInstance && quickJsContext) });
    return;
  }
  if (m.type === 'invoke') {
    const requestId = String(m.requestId || '');
    if (!prepared) return bridgeError(requestId, 'not-prepared', 'protected bridge worker is not prepared');
    if (!/^[A-Za-z0-9_-]{8,96}$/.test(requestId)) return bridgeError(requestId, 'invalid-request-id', 'invalid bridge request id');
    if (JSON.stringify(m).length > MAX_BRIDGE_MESSAGE_BYTES) return bridgeError(requestId, 'message-too-large', 'bridge request exceeds protocol limit');
    const candidate = String(m.candidate || '');
    if (!BRIDGE_CANDIDATES.has(candidate)) return bridgeError(requestId, 'unknown-candidate', 'bridge candidate is not declared by the package');
    if (!Array.isArray(m.args) || m.args.length > MAX_BRIDGE_ARGUMENTS || !isJsonValue(m.args)) return bridgeError(requestId, 'invalid-arguments', 'bridge arguments violate json-value-v1');
    if (activeInvocations >= MAX_BRIDGE_CONCURRENCY) return bridgeError(requestId, 'busy', 'bridge concurrency limit reached');
    activeInvocations++;
    try {
      if (!quickJsInstance || !quickJsContext) return bridgeError(requestId, 'executor-not-ready', 'protected QuickJS bridge executor is not ready');
      const e = quickJsInstance.exports;
      const envelope = textEncoder.encode(JSON.stringify({ candidate, args: m.args }));
      const capacity = e.venom_qjs_bridge_input_capacity() >>> 0;
      if (!envelope.byteLength || envelope.byteLength > capacity) return bridgeError(requestId, 'input-too-large', 'bridge request exceeds QuickJS input capacity');
      const ptr = e.venom_qjs_bridge_input_alloc(quickJsContext >>> 0, envelope.byteLength >>> 0) >>> 0;
      if (!ptr) return bridgeError(requestId, 'input-allocation-failed', 'QuickJS bridge input allocation failed');
      try {
        new Uint8Array(e.memory.buffer, ptr, envelope.byteLength).set(envelope);
        const ok = e.venom_qjs_bridge_invoke(quickJsContext >>> 0, envelope.byteLength >>> 0) >>> 0;
        if (!ok) {
          const ep = e.venom_qjs_exception_message_ptr() >>> 0;
          const es = e.venom_qjs_exception_message_size() >>> 0;
          let detail = 'protected bridge invocation failed';
          if (ep && es && ep + es <= e.memory.buffer.byteLength) detail = textDecoder.decode(new Uint8Array(e.memory.buffer, ep, es));
          if (typeof e.venom_qjs_exception_clear === 'function') e.venom_qjs_exception_clear();
          const code = detail.includes('not registered') ? 'candidate-not-registered' : 'execution-failed';
          return bridgeError(requestId, code, detail);
        }
        const resultPtr = e.venom_qjs_bridge_result_ptr() >>> 0;
        const resultSize = e.venom_qjs_bridge_result_size() >>> 0;
        if (!resultPtr || !resultSize || resultPtr + resultSize > e.memory.buffer.byteLength) return bridgeError(requestId, 'invalid-result', 'QuickJS bridge returned an invalid result range');
        const result = JSON.parse(textDecoder.decode(new Uint8Array(e.memory.buffer, resultPtr, resultSize)));
        if (!isJsonValue(result)) return bridgeError(requestId, 'invalid-result', 'QuickJS bridge result violates json-value-v1');
        postMessage({ protocol: WORKER_PROTOCOL, type: 'bridge-result', requestId, result });
      } finally {
        e.venom_qjs_bridge_release(quickJsContext >>> 0);
      }
    } catch (error) {
      return bridgeError(requestId, 'execution-failed', String(error && error.message ? error.message : error));
    } finally { activeInvocations--; }
  }
  if (m.type !== 'prepare' || typeof m.nonce !== 'string' || m.nonce.length !== 32) return fail(m.nonce || '', new Error('invalid worker protocol request'));
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
    prepared = true;
    postMessage({ protocol: WORKER_PROTOCOL, type: 'ready', nonce: m.nonce, bridgeCandidateCount: BRIDGE_CANDIDATES.size, packageHash: actualPackageHash, packageSha256: actualPackageSha256, quickJsWasmSha256: actualQuickJsWasmSha256, quickJsRuntimeReady: true, packageBytes, quickJsModule }, [packageBytes]);
  } catch (error) { fail(m.nonce, error); }
};
)WORKER";
  return generated.str();
}


} // namespace venom::compiler
