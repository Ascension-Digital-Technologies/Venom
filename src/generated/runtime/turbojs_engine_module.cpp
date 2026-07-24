#include "pipeline/js.hpp"
#include "generated/contracts/turbojs_wasm_abi.hpp"

#include <stdexcept>
#include <sstream>

namespace venom::compiler {

std::string make_turbojs_engine_module_js(bool protected_release) {
  std::string js = R"TJSENGINE(export function createVenomTurboJsEngineModule(config = {}) {
  const moduleVersion = 10;
  const protectedRelease = false;
  const RELEASE_ABI_PREFIX = 'VTJSABI12|VRDIV003|VRC3|';
  const RELEASE_ABI_FINGERPRINT = 0;
  const RELEASE_ABI_REQUIRED_EXPORTS = Object.freeze([
    'memory',
    'venom_tjs_context_alloc',
    'venom_tjs_context_free',
    'venom_tjs_context_set_limits',
    'venom_tjs_context_set_execution_limits',
    'venom_tjs_script_buffer_alloc',
    'venom_tjs_script_buffer_capacity',
    'venom_tjs_script_buffer_set_expected_hash',
    'venom_tjs_script_buffer_free',
    'venom_tjs_bytecode_validate',
    'venom_tjs_execute_bytecode',
    'venom_tjs_compact_result_ptr',
    'venom_tjs_diversification_install',
    'venom_tjs_exception_message_ptr',
    'venom_tjs_exception_message_size',
    'venom_tjs_exception_clear',
    'venom_tjs_upstream_turbojs_ready',
    'venom_tjs_bridge_abi',
    'venom_tjs_bridge_input_alloc',
    'venom_tjs_bridge_input_capacity',
    'venom_tjs_bridge_invoke',
    'venom_tjs_bridge_result_ptr',
    'venom_tjs_bridge_result_size',
    'venom_tjs_bridge_release',
  ]);
  const RELEASE_ABI_REQUIRED_KINDS = new Map(RELEASE_ABI_REQUIRED_EXPORTS.map((name) => [name, name === 'memory' ? 'memory' : 'function']));
  const RELEASE_ABI_APPROVED_TOOLCHAIN_EXPORTS = new Map([
    ['__indirect_function_table', 'table'],
    ['__cxa_increment_exception_refcount', 'function'],
    ['_initialize', 'function'],
    ['_emscripten_stack_restore', 'function'],
    ['emscripten_stack_get_current', 'function'],
  ]);
  function releaseAbiHash(names) {
    const text = RELEASE_ABI_PREFIX + names.slice().sort().join('|');
    let h = 0x811c9dc5;
    for (let i = 0; i < text.length; i++) { h ^= text.charCodeAt(i) & 0xff; h = Math.imul(h, 0x01000193) >>> 0; }
    return h >>> 0;
  }
  function validateReleaseAbiExports(module) {
    const entries = WebAssembly.Module.exports(module).map((entry) => ({ name: String(entry.name), kind: String(entry.kind) }));
    const actualKinds = new Map(entries.map((entry) => [entry.name, entry.kind]));
    const missing = RELEASE_ABI_REQUIRED_EXPORTS.filter((name) => !actualKinds.has(name));
    const kindMismatches = [];
    for (const [name, kind] of RELEASE_ABI_REQUIRED_KINDS) {
      if (actualKinds.has(name) && actualKinds.get(name) !== kind) kindMismatches.push(name + ':' + actualKinds.get(name) + '!=' + kind);
    }
    for (const [name, kind] of RELEASE_ABI_APPROVED_TOOLCHAIN_EXPORTS) {
      if (actualKinds.has(name) && actualKinds.get(name) !== kind) kindMismatches.push(name + ':' + actualKinds.get(name) + '!=' + kind);
    }
    const unapproved = entries
      .filter((entry) => !RELEASE_ABI_REQUIRED_KINDS.has(entry.name) && !RELEASE_ABI_APPROVED_TOOLCHAIN_EXPORTS.has(entry.name))
      .map((entry) => entry.name + ':' + entry.kind)
      .sort();
    if (missing.length || kindMismatches.length || unapproved.length) {
      throw new Error(
        'TurboJS WASM release export surface mismatch: missing=' + (missing.join(',') || 'none') +
        ' kind_mismatch=' + (kindMismatches.join(',') || 'none') +
        ' unapproved=' + (unapproved.join(',') || 'none')
      );
    }
    return RELEASE_ABI_REQUIRED_EXPORTS;
  }
  const mode = config.mode || 'turbojs-wasm-abi12-upstream-global-host-api-shims';
  const fallback = config.fallback || 'deny-host-js-source-eval';
  const wasmUrl = config.wasmUrl || (config.wasmRuntime && config.wasmRuntime.assetName) || '';
  const providedWasmModule = config.wasmModule || globalThis.__venomWorkerTurboJsModule || null;
  const contexts = new Map();
  const executions = [];
  const consoleEvents = [];
  const resultRecords = [];
  const moduleNamespaceCache = new Map();
  const resolverAuditRecords = [];
  const moduleExecutions = [];
  const SCRIPT_FLAG_MODULE = 2;
  let wasmPromise = null;
  let wasmInstance = null;
  let wasmError = null;

  const encoder = new TextEncoder();
  const decoder = new TextDecoder();
  const heapLimit = config.heapLimits && config.heapLimits.heapLimit ? config.heapLimits.heapLimit >>> 0 : 8388608;
  const stackLimit = config.heapLimits && config.heapLimits.stackLimit ? config.heapLimits.stackLimit >>> 0 : 262144;
  const interruptBudget = config.contextLimitPolicy && config.contextLimitPolicy.maxInterruptTicks ? config.contextLimitPolicy.maxInterruptTicks >>> 0 : 1000000;
  const pendingJobLimit = config.contextLimitPolicy && config.contextLimitPolicy.maxPendingJobs ? config.contextLimitPolicy.maxPendingJobs >>> 0 : 1024;
  const maxConsoleEvents = config.contextLimitPolicy && config.contextLimitPolicy.maxConsoleEvents ? config.contextLimitPolicy.maxConsoleEvents >>> 0 : 1024;
  const maxExecutionRecords = config.contextLimitPolicy && config.contextLimitPolicy.maxExecutionRecords ? config.contextLimitPolicy.maxExecutionRecords >>> 0 : 256;
  const diversification = config.diversification || null;
  let diversificationInstalled = false;
  let wasmExecutionQueue = Promise.resolve();
  let cachedAbiTable = null;
  let cachedHostImportTable = null;
  let cachedParityProbe = null;
  let cachedBytecodeManifest = null;
  let cachedHostTrapPolicy = null;
  let cachedModuleGraph = null;
  let cachedExecutionJournal = null;
  let cachedCheckpointPolicy = null;
  let cachedReplayCursor = null;
  let cachedResumeState = null;
  let cachedDeterminismAudit = null;
  let cachedSnapshotPolicy = null;
  let cachedSnapshotRecord = null;
  let cachedReplayValidation = null;
  let cachedDeterminismLedger = null;
  let cachedAuditSeal = null;
  let cachedExecutionCommit = null;
  let cachedRollbackPolicy = null;
  let cachedHostReceipts = null;
  let cachedReleaseAcceptance = null;
  let cachedCommitAudit = null;
  let cachedCapabilityPolicy = null;
  let cachedHostIoPolicy = null;
  let cachedPermissionSeal = null;
  let cachedPolicyReceipts = null;
  let cachedReleaseGate = null;
  let cachedHostIoDecision = null;
  let cachedHostIoDenyTrace = null;
  let cachedCapabilityLedger = null;
  let cachedPolicySealAudit = null;
  let cachedRuntimeDenylist = null;
  let cachedRuntimeLimits = null;
  let cachedBackendContract = null;
  let cachedInterpreterContract = null;
  let cachedSemanticRecord = null;
  let cachedGlobalSlotRecord = null;
  let cachedUpstreamParityRecord = null;
  let cachedUpstreamBytecodeSemanticsRecord = null;
  let cachedUpstreamLexicalScopeRecord = null;
  let cachedUpstreamClosureRecord = null;
  let cachedUpstreamIteratorRecord = null;
  let cachedUpstreamIntrinsicRecord = null;
  let cachedUpstreamPropertyDescriptorRecord = null;
  let cachedUpstreamPrototypeChainRecord = null;
  let cachedUpstreamCallFrameRecord = null;

  function enqueueWasmExecution(task) {
    const run = wasmExecutionQueue.then(task, task);
    wasmExecutionQueue = run.catch(() => undefined);
    return run;
  }

  function boundedPush(list, value, limit) {
    const cap = Math.max(1, limit >>> 0);
    if (list.length >= cap) list.splice(0, list.length - cap + 1);
    list.push(value);
    return value;
  }

  function safeSourceUrl(source) {
    return String(source || 'chunk').replace(/[^a-zA-Z0-9_./#:-]/g, '_');
  }

  function memoryView(instance) {
    const memory = instance && instance.exports ? instance.exports.memory : null;
    if (!memory || !(memory.buffer instanceof ArrayBuffer)) throw new Error('TurboJS backend memory export is invalid');
    return new Uint8Array(memory.buffer);
  }

  function memoryRange(instance, ptr, size, label = 'TurboJS') {
    const start = ptr >>> 0;
    const length = size >>> 0;
    const memory = memoryView(instance);
    if (!start || !length || start > memory.byteLength || length > memory.byteLength - start) {
      throw new Error(label + ' memory range is invalid');
    }
    return memory.subarray(start, start + length);
  }

  function readWasmBytes(instance, ptr, size, label = 'TurboJS') {
    return memoryRange(instance, ptr, size, label).slice();
  }

  function zeroWasmRange(instance, ptr, size) {
    if (!ptr || !size) return;
    try { memoryRange(instance, ptr, size, 'TurboJS zeroize').fill(0); } catch (_) {}
  }

  function readWasmString(instance, ptr, size) {
    if (!ptr || !size) return '';
    const copy = readWasmBytes(instance, ptr, size, 'TurboJS string');
    try { return decoder.decode(copy); }
    finally { copy.fill(0); }
  }


  function fnv1a32(bytes) {
    let hash = 2166136261 >>> 0;
    for (const value of bytes) {
      hash ^= value >>> 0;
      hash = Math.imul(hash, 16777619) >>> 0;
    }
    return hash >>> 0;
  }

  function validateBytecodeTrustHandoff(chunk) {
    const bytes = chunk && chunk.bytecodeBytes;
    if (!(bytes instanceof Uint8Array) || !bytes.length) return true;
    const handoff = chunk.bytecodeTrustHandoff;
    const trustedProducer = handoff && (handoff.producer === 'package-decoder-wasm' || (!protectedRelease && handoff.producer === 'development-turbojs-compiler'));
    if (!handoff || handoff.version !== 1 || !trustedProducer || handoff.consumer !== 'turbojs-execution-wasm') {
      throw new Error('TurboJS trust-domain handoff is missing or invalid');
    }
    const byteHash = fnv1a32(bytes) >>> 0;
    const bindingText = String(chunk.route || '') + '\n' + String(chunk.source || '') + '\n' + String(chunk.order >>> 0) + '\n' + String(bytes.length >>> 0) + '\n' + String(byteHash);
    const bindingHash = fnv1a32(encoder.encode(bindingText)) >>> 0;
    if ((handoff.byteLength >>> 0) !== (bytes.length >>> 0) || (handoff.byteHash >>> 0) !== byteHash || (handoff.bindingHash >>> 0) !== bindingHash) {
      throw new Error('TurboJS trust-domain handoff integrity mismatch');
    }
    return true;
  }

  function decodeTurboJsRecordForDeniedFallback(bytes, sourceName) {
    void bytes;
    throw new Error('native TurboJS object bytecode cannot be converted back to host source: ' + (sourceName || '<script>'));
  }

  function hasVtjsbc03Magic(bytes) {
    if (!bytes || bytes.length < 8) return false;
    return bytes[0] === 0x56 && bytes[1] === 0x51 && bytes[2] === 0x4a && bytes[3] === 0x53 &&
           bytes[4] === 0x42 && bytes[5] === 0x43 && bytes[6] === 0x30 && bytes[7] === 0x33;
  }

  function applyWasmHostEffects(effects, bindings) {
    if (!Array.isArray(effects) || effects.length === 0) return 0;
    const g = bindings && bindings.globalThis ? bindings.globalThis : globalThis;
    let applied = 0;
    for (const effect of effects) {
      if (!effect || effect.op !== 'incrementGlobal') continue;
      const name = String(effect.name || '');
      if (!/^__[A-Za-z0-9_]+$/.test(name)) continue;
      const delta = Number.isFinite(effect.delta) ? Number(effect.delta) : 1;
      g[name] = (g[name] || 0) + delta;
      applied++;
    }
    return applied;
  }

  function decodeHostBridgeSource(chunk) {
    const rawBytes = chunk && chunk.bytecodeBytes && chunk.bytecodeBytes.length ? chunk.bytecodeBytes : null;
    let source = String((chunk && chunk.code) || '');
    let sourceKind = source ? 'source-text' : 'empty';
    if ((!source || !source.trim()) && rawBytes) {
      if (hasVtjsbc03Magic(rawBytes)) {
        source = '';
        sourceKind = 'opaque-vtjsbc03-native-object';
      } else {
        try {
          source = decodeTurboJsRecordForDeniedFallback(rawBytes, chunk && chunk.source);
        } catch (_) {
          source = '';
          sourceKind = 'opaque-bytecode-unreadable';
        }
      }
    }
    return Object.freeze({ source, sourceKind });
  }

  function replayWasmHostBridgeEffects(chunk, telemetry, bindings) {
    void chunk; void bindings;
    return Object.freeze({
      version: 2,
      mode: 'native-wasm-host-calls-only',
      sourceKind: 'opaque-vtjsbc03-native-object',
      applied: 0,
      operations: Object.freeze([]),
      receipts: Object.freeze([]),
      sourceBytes: 0,
      evalUsed: false,
      functionConstructorUsed: false,
      releaseFallbackDenied: true,
      boundary: telemetry && telemetry.boundary ? telemetry.boundary : 'upstream-turbojs-wasm-host-call-bridge'
    });
  }

  function inferWasmHostBridgeTelemetry(chunk, wasmResult) {
    const rawBytes = chunk && chunk.bytecodeBytes && chunk.bytecodeBytes.length ? chunk.bytecodeBytes : null;
    let source = String((chunk && chunk.code) || '');
    let sourceKind = source ? 'source-text' : 'empty';
    if ((!source || !source.trim()) && rawBytes) {
      if (hasVtjsbc03Magic(rawBytes)) {
        source = '';
        sourceKind = 'opaque-vtjsbc03-native-object';
      } else {
        try {
          source = decodeTurboJsRecordForDeniedFallback(rawBytes, chunk && chunk.source);
        } catch (_) {
          source = '';
          sourceKind = 'opaque-bytecode-unreadable';
        }
      }
    }
    const checks = [
      ['console', /console\s*\./],
      ['dom-query', /\b(querySelector|getElementById)\s*\(/],
      ['dom-create', /document\.createElement\s*\(/],
      ['dom-text', /\b(textContent|innerHTML)\b/],
      ['dom-class', /classList\s*\./],
      ['dom-style', /\.style\b|setProperty\s*\(/],
      ['events', /addEventListener\s*\(|dispatchEvent\s*\(|preventDefault\s*\(/],
      ['timers', /setTimeout\s*\(|setInterval\s*\(/],
      ['fetch', /\bfetch\s*\(/],
      ['fetch-response', /response\.(status|ok|statusText|json|text)\b/],
      ['fetch-headers', /headers\.(get|forEach)\s*\(/],
      ['host-deny', /blocked-secret\.json|DenyByDefault|DeniedHostCall/],
      ['forms', /submit\s*\(|addEventListener\('submit'|new Event\('submit'|SubmitEvent|HTMLFormElement/],
      ['routing', /history\.pushState\s*\(|location\.(href|pathname)/],
      ['modules', /(^|\n)\s*(import|export)\s+/m],
      ['promises', /\b(Promise|async\s+function|await)\b/],
      ['async-errors', /Promise\.reject|async-throw-captured|catch\s*\(/],
    ];
    const operations = checks.filter(([, re]) => re.test(source)).map(([name]) => name);
    const report = wasmResult && wasmResult.report ? wasmResult.report : {};
    const hostCallCount = wasmResult && Number.isFinite(wasmResult.hostCallCount) ? wasmResult.hostCallCount : (report && Number.isFinite(report.hostCallCount) ? report.hostCallCount : 0);
    const consoleCount = wasmResult && Number.isFinite(wasmResult.consoleCount) ? wasmResult.consoleCount : (report && Number.isFinite(report.consoleCount) ? report.consoleCount : 0);
    const receiptsLoaded = !!(wasmResult && wasmResult.hostReceipts && wasmResult.hostReceipts.loaded);
    const ioDecisionLoaded = !!(wasmResult && wasmResult.hostIoDecision && wasmResult.hostIoDecision.loaded);
    const denyTraceLoaded = !!(wasmResult && wasmResult.hostIoDenyTrace && wasmResult.hostIoDenyTrace.loaded);
    const wasmAccepted = !!(wasmResult && wasmResult.turboJsWasm);
    const upstreamReady = !!(wasmResult && wasmResult.upstreamTurboJsReady);
    return Object.freeze({
      version: 1,
      boundary: 'upstream-turbojs-wasm-host-call-bridge',
      sourceKind,
      sourceBytes: source.length,
      operations: Object.freeze(operations),
      operationCount: operations.length,
      operationInference: sourceKind === 'source-text' ? 'source-text-debug-only' : 'disabled-for-opaque-bytecode',
      hostCallCount: hostCallCount >>> 0,
      consoleCount: consoleCount >>> 0,
      receiptsLoaded,
      ioDecisionLoaded,
      denyTraceLoaded,
      wasmAccepted,
      upstreamReady,
      fallbackRequired: !!(wasmResult && wasmResult.fallbackRequired),
      parity: wasmAccepted && upstreamReady && !((wasmResult && wasmResult.fallbackRequired)) && (hostCallCount > 0 || operations.length === 0),
    });
  }

  function lifecycleStateName(value) {
    return ({1:'created',2:'configured',3:'loaded',4:'executing',5:'trapped',6:'disposed'})[value >>> 0] || 'unknown';
  }

  function readAbiTableFromExports(instance) {
    const e = instance && instance.exports ? instance.exports : {};
    if (!e.venom_tjs_abi_table_ptr || !e.venom_tjs_abi_table_size) return null;
    return Object.freeze({ abi: e.venom_tjs_engine_abi ? e.venom_tjs_engine_abi() >>> 0 : 0, packageVersion: e.venom_tjs_engine_version ? e.venom_tjs_engine_version() >>> 0 : 0, entryCount: e.venom_tjs_abi_entry_count ? e.venom_tjs_abi_entry_count() >>> 0 : 0, tableHash: e.venom_tjs_abi_table_hash ? String(e.venom_tjs_abi_table_hash() >>> 0) : '', table: readWasmString(instance, e.venom_tjs_abi_table_ptr() >>> 0, e.venom_tjs_abi_table_size() >>> 0) });
  }

  function readHostImportTableFromExports(instance) {
    const e = instance && instance.exports ? instance.exports : {};
    if (!e.venom_tjs_host_import_table_ptr || !e.venom_tjs_host_import_table_size) return null;
    return Object.freeze({ importCount: e.venom_tjs_host_import_count ? e.venom_tjs_host_import_count() >>> 0 : 0, tableHash: e.venom_tjs_host_import_table_hash ? String(e.venom_tjs_host_import_table_hash() >>> 0) : '', table: readWasmString(instance, e.venom_tjs_host_import_table_ptr() >>> 0, e.venom_tjs_host_import_table_size() >>> 0), design: 'turbojs-host-call-import-table-v3' });
  }

  function readBytecodeManifestFromExports(instance) {
    const e = instance && instance.exports ? instance.exports : {};
    if (!e.venom_tjs_bytecode_manifest_ptr || !e.venom_tjs_bytecode_manifest_size) return null;
    return Object.freeze({ hash: e.venom_tjs_bytecode_manifest_hash ? String(e.venom_tjs_bytecode_manifest_hash() >>> 0) : '', text: readWasmString(instance, e.venom_tjs_bytecode_manifest_ptr() >>> 0, e.venom_tjs_bytecode_manifest_size() >>> 0), loaded: true });
  }

  function readHostTrapPolicyFromExports(instance) {
    const e = instance && instance.exports ? instance.exports : {};
    if (!e.venom_tjs_host_trap_policy_ptr || !e.venom_tjs_host_trap_policy_size) return null;
    return Object.freeze({ hash: e.venom_tjs_host_trap_policy_hash ? String(e.venom_tjs_host_trap_policy_hash() >>> 0) : '', text: readWasmString(instance, e.venom_tjs_host_trap_policy_ptr() >>> 0, e.venom_tjs_host_trap_policy_size() >>> 0), loaded: true });
  }


  function readModuleGraphFromExports(instance) {
    const e = instance && instance.exports ? instance.exports : {};
    if (!e.venom_tjs_module_graph_ptr || !e.venom_tjs_module_graph_size) return null;
    return Object.freeze({ hash: e.venom_tjs_module_graph_hash ? String(e.venom_tjs_module_graph_hash() >>> 0) : '', text: readWasmString(instance, e.venom_tjs_module_graph_ptr() >>> 0, e.venom_tjs_module_graph_size() >>> 0), loaded: true });
  }

  function readTextRecordFromExports(instance, ptrName, sizeName, hashName) {
    const e = instance && instance.exports ? instance.exports : {};
    if (typeof e[ptrName] !== 'function' || typeof e[sizeName] !== 'function') return null;
    const ptr = e[ptrName]() >>> 0;
    const size = e[sizeName]() >>> 0;
    if (!ptr || !size) return null;
    return Object.freeze({ hash: hashName && typeof e[hashName] === 'function' ? String(e[hashName]() >>> 0) : '', text: readWasmString(instance, ptr, size), loaded: true });
  }


  function makeDefaultWasmTable() {
    try { return new WebAssembly.Table({ initial: 1, element: 'funcref' }); }
    catch (_) { return new WebAssembly.Table({ initial: 1, element: 'anyfunc' }); }
  }

  function makeDefaultWasmGlobal(desc) {
    const value = desc && desc.type && desc.type.value ? desc.type.value : 'i32';
    const mutable = !!(desc && desc.type && desc.type.mutable);
    try { return new WebAssembly.Global({ value, mutable }, value === 'f64' || value === 'f32' ? 0.0 : 0); }
    catch (_) { return 0; }
  }

  function makeDefaultWasmImport(desc) {
    if (!desc || desc.kind === 'function') return function venomWasmHostImportStub() { return 0; };
    if (desc.kind === 'memory') return new WebAssembly.Memory({ initial: 512, maximum: 4096 });
    if (desc.kind === 'table') return makeDefaultWasmTable();
    if (desc.kind === 'global') return makeDefaultWasmGlobal(desc);
    return 0;
  }

  function makeWasmImportObject(module) {
    const imports = Object.create(null);
    for (const desc of WebAssembly.Module.imports(module)) {
      const moduleName = String(desc.module || 'env');
      const importName = String(desc.name || '');
      if (!imports[moduleName]) imports[moduleName] = Object.create(null);
      imports[moduleName][importName] = makeDefaultWasmImport(desc);
    }
    return imports;
  }

  function wasmFailureDetail() {
    if (wasmError) return String(wasmError && wasmError.message ? wasmError.message : wasmError);
    if (providedWasmModule) return 'provided module was not accepted';
    if (!wasmUrl) return 'runtime URL is missing';
    if (typeof WebAssembly === 'undefined') return 'WebAssembly is unavailable';
    if (typeof fetch !== 'function') return 'fetch is unavailable';
    return 'runtime did not initialize';
  }
  async function loadWasm() {
    if (typeof WebAssembly === 'undefined') { wasmError = new Error('WebAssembly is unavailable'); return null; }
    if (!providedWasmModule && !wasmUrl) { wasmError = new Error('TurboJS WASM runtime URL is missing'); return null; }
    if (!providedWasmModule && typeof fetch !== 'function') { wasmError = new Error('fetch is unavailable for TurboJS WASM loading'); return null; }
    if (wasmInstance) return wasmInstance;
    if (wasmError) return null;
    if (!wasmPromise) {
      wasmPromise = (providedWasmModule
        ? Promise.resolve(providedWasmModule)
        : fetch(wasmUrl, { cache: 'force-cache' }).then((response) => {
            if (!response.ok) throw new Error('failed to load TurboJS WASM backend: ' + response.status);
            return response.arrayBuffer().then((bytes) => WebAssembly.compile(bytes));
          }))
        .then((module) => {
          if (protectedRelease) {
            if (!config.abiFingerprint) throw new Error('protected release lacks TurboJS ABI fingerprint');
            validateReleaseAbiExports(module);
            const packaged = config.abiFingerprint.hash >>> 0;
            if (packaged !== (RELEASE_ABI_FINGERPRINT >>> 0)) {
              throw new Error('TurboJS package/runtime ABI fingerprint mismatch: package=0x' + packaged.toString(16).padStart(8, '0') + ' runtime=0x' + (RELEASE_ABI_FINGERPRINT >>> 0).toString(16).padStart(8, '0'));
            }
          }
          return WebAssembly.instantiate(module, makeWasmImportObject(module));
        })
        .then((result) => {
          const instance = result.instance || result;
          const e = instance.exports || {};
          // The embedded browser artifact deliberately exposes the same minimal
          // bytecode-only ABI in development and release builds. Development mode
          // changes the accepted trust-domain producer, not the WASM export surface.
          // Do not require the old debug/scaffold exports (engine_abi,
          // execute_source, native_stack_capacity): current upstream TurboJS/WASM
          // artifacts intentionally omit them.
          const required = RELEASE_ABI_REQUIRED_EXPORTS;
          for (const name of required) if (!e[name]) throw new Error('TurboJS WASM backend missing ' + name + ' export');
          if ((e.venom_tjs_upstream_turbojs_ready() >>> 0) !== 1) throw new Error('TurboJS upstream runtime is not ready');
          if (!protectedRelease && (e.venom_tjs_bridge_abi() >>> 0) !== 1) {
            throw new Error('TurboJS WASM bridge ABI mismatch');
          }
          cachedAbiTable = readAbiTableFromExports(instance);
          cachedHostImportTable = readHostImportTableFromExports(instance);
          cachedBytecodeManifest = readBytecodeManifestFromExports(instance);
          cachedHostTrapPolicy = readHostTrapPolicyFromExports(instance);
          cachedModuleGraph = readModuleGraphFromExports(instance);
          cachedCheckpointPolicy = readTextRecordFromExports(instance, 'venom_tjs_checkpoint_policy_ptr', 'venom_tjs_checkpoint_policy_size', 'venom_tjs_checkpoint_policy_hash');
          cachedExecutionJournal = readTextRecordFromExports(instance, 'venom_tjs_execution_journal_ptr', 'venom_tjs_execution_journal_size', 'venom_tjs_execution_journal_hash');
          cachedReplayCursor = readTextRecordFromExports(instance, 'venom_tjs_replay_cursor_ptr', 'venom_tjs_replay_cursor_size', 'venom_tjs_replay_cursor_hash');
          cachedResumeState = readTextRecordFromExports(instance, 'venom_tjs_resume_state_ptr', 'venom_tjs_resume_state_size', 'venom_tjs_resume_state_hash');
          cachedDeterminismAudit = readTextRecordFromExports(instance, 'venom_tjs_determinism_audit_ptr', 'venom_tjs_determinism_audit_size', 'venom_tjs_determinism_audit_hash');
          cachedSnapshotPolicy = readTextRecordFromExports(instance, 'venom_tjs_snapshot_policy_ptr', 'venom_tjs_snapshot_policy_size', 'venom_tjs_snapshot_policy_hash');
          cachedSnapshotRecord = readTextRecordFromExports(instance, 'venom_tjs_snapshot_record_ptr', 'venom_tjs_snapshot_record_size', 'venom_tjs_snapshot_record_hash');
          cachedReplayValidation = readTextRecordFromExports(instance, 'venom_tjs_replay_validation_ptr', 'venom_tjs_replay_validation_size', 'venom_tjs_replay_validation_hash');
          cachedDeterminismLedger = readTextRecordFromExports(instance, 'venom_tjs_determinism_ledger_ptr', 'venom_tjs_determinism_ledger_size', 'venom_tjs_determinism_ledger_hash');
          cachedAuditSeal = readTextRecordFromExports(instance, 'venom_tjs_audit_seal_ptr', 'venom_tjs_audit_seal_size', 'venom_tjs_audit_seal_hash');
    cachedExecutionCommit = readTextRecordFromExports(instance, 'venom_tjs_execution_commit_ptr', 'venom_tjs_execution_commit_size', 'venom_tjs_execution_commit_hash');
    cachedRollbackPolicy = readTextRecordFromExports(instance, 'venom_tjs_rollback_policy_ptr', 'venom_tjs_rollback_policy_size', 'venom_tjs_rollback_policy_hash');
    cachedHostReceipts = readTextRecordFromExports(instance, 'venom_tjs_host_receipts_ptr', 'venom_tjs_host_receipts_size', 'venom_tjs_host_receipts_hash');
    cachedReleaseAcceptance = readTextRecordFromExports(instance, 'venom_tjs_release_acceptance_ptr', 'venom_tjs_release_acceptance_size', 'venom_tjs_release_acceptance_hash');
    cachedCommitAudit = readTextRecordFromExports(instance, 'venom_tjs_commit_audit_ptr', 'venom_tjs_commit_audit_size', 'venom_tjs_commit_audit_hash');
          cachedCapabilityPolicy = readTextRecordFromExports(instance, 'venom_tjs_capability_policy_ptr', 'venom_tjs_capability_policy_size', 'venom_tjs_capability_policy_hash');
          cachedHostIoPolicy = readTextRecordFromExports(instance, 'venom_tjs_host_io_policy_ptr', 'venom_tjs_host_io_policy_size', 'venom_tjs_host_io_policy_hash');
          cachedPermissionSeal = readTextRecordFromExports(instance, 'venom_tjs_permission_seal_ptr', 'venom_tjs_permission_seal_size', 'venom_tjs_permission_seal_hash');
          cachedPolicyReceipts = readTextRecordFromExports(instance, 'venom_tjs_policy_receipts_ptr', 'venom_tjs_policy_receipts_size', 'venom_tjs_policy_receipts_hash');
          cachedReleaseGate = readTextRecordFromExports(instance, 'venom_tjs_release_gate_ptr', 'venom_tjs_release_gate_size', 'venom_tjs_release_gate_hash');
          cachedHostIoDecision = readTextRecordFromExports(instance, 'venom_tjs_host_io_decision_ptr', 'venom_tjs_host_io_decision_size', 'venom_tjs_host_io_decision_hash');
          cachedHostIoDenyTrace = readTextRecordFromExports(instance, 'venom_tjs_host_io_deny_trace_ptr', 'venom_tjs_host_io_deny_trace_size', 'venom_tjs_host_io_deny_trace_hash');
          cachedCapabilityLedger = readTextRecordFromExports(instance, 'venom_tjs_capability_ledger_ptr', 'venom_tjs_capability_ledger_size', 'venom_tjs_capability_ledger_hash');
          cachedPolicySealAudit = readTextRecordFromExports(instance, 'venom_tjs_policy_seal_audit_ptr', 'venom_tjs_policy_seal_audit_size', 'venom_tjs_policy_seal_audit_hash');
          cachedRuntimeDenylist = readTextRecordFromExports(instance, 'venom_tjs_runtime_denylist_ptr', 'venom_tjs_runtime_denylist_size', 'venom_tjs_runtime_denylist_hash');
          cachedRuntimeLimits = readTextRecordFromExports(instance, 'venom_tjs_runtime_limits_ptr', 'venom_tjs_runtime_limits_size', 'venom_tjs_runtime_limits_hash');
          cachedBackendContract = readTextRecordFromExports(instance, 'venom_tjs_backend_contract_ptr', 'venom_tjs_backend_contract_size', 'venom_tjs_backend_contract_hash');
          cachedInterpreterContract = readTextRecordFromExports(instance, 'venom_tjs_interpreter_contract_ptr', 'venom_tjs_interpreter_contract_size', 'venom_tjs_interpreter_contract_hash');
          cachedSemanticRecord = readTextRecordFromExports(instance, 'venom_tjs_interpreter_semantic_record_ptr', 'venom_tjs_interpreter_semantic_record_size', 'venom_tjs_interpreter_semantic_record_hash');
          cachedGlobalSlotRecord = readTextRecordFromExports(instance, 'venom_tjs_global_slot_record_ptr', 'venom_tjs_global_slot_record_size', 'venom_tjs_global_slot_record_hash');
          cachedUpstreamParityRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_parity_record_ptr', 'venom_tjs_upstream_parity_record_size', 'venom_tjs_upstream_parity_record_hash');
          cachedUpstreamBytecodeSemanticsRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_bytecode_semantics_record_ptr', 'venom_tjs_upstream_bytecode_semantics_record_size', 'venom_tjs_upstream_bytecode_semantics_record_hash');
          cachedUpstreamLexicalScopeRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_lexical_scope_record_ptr', 'venom_tjs_upstream_lexical_scope_record_size', 'venom_tjs_upstream_lexical_scope_record_hash');
          cachedUpstreamClosureRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_closure_record_ptr', 'venom_tjs_upstream_closure_record_size', 'venom_tjs_upstream_closure_record_hash');
          cachedUpstreamIteratorRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_iterator_record_ptr', 'venom_tjs_upstream_iterator_record_size', 'venom_tjs_upstream_iterator_record_hash');
          cachedUpstreamIntrinsicRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_intrinsic_record_ptr', 'venom_tjs_upstream_intrinsic_record_size', 'venom_tjs_upstream_intrinsic_record_hash');
          cachedUpstreamPropertyDescriptorRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_property_descriptor_record_ptr', 'venom_tjs_upstream_property_descriptor_record_size', 'venom_tjs_upstream_property_descriptor_record_hash');
          cachedUpstreamPrototypeChainRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_prototype_chain_record_ptr', 'venom_tjs_upstream_prototype_chain_record_size', 'venom_tjs_upstream_prototype_chain_record_hash');
          cachedUpstreamCallFrameRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_call_frame_record_ptr', 'venom_tjs_upstream_call_frame_record_size', 'venom_tjs_upstream_call_frame_record_hash');
          const wasmProbe = e.venom_tjs_parity_probe ? e.venom_tjs_parity_probe() >>> 0 : 0;
          cachedParityProbe = Object.freeze({ expected: config.parityProbe && config.parityProbe.expected ? config.parityProbe.expected : 'abi+host-import-hash', wasmProbe, native: config.parityProbe && config.parityProbe.native ? config.parityProbe.native : '', matched: wasmProbe !== 0, loaded: true });
          wasmInstance = instance;
          return instance;
        })
        .catch((error) => {
          wasmError = error instanceof Error ? error : new Error(String(error));
          if (globalThis.console && typeof globalThis.console.error === 'function') {
            globalThis.console.error('[venom] TurboJS WASM initialization failed:', wasmFailureDetail());
          }
          return null;
        });
    }
    return wasmPromise;
  }

  function createContext(context = {}) {
    const key = String(context.key || ((context.route || '/') + '::' + (context.source || '') + '::' + (context.order >>> 0)));
    let record = contexts.get(key);
    if (!record) {
      record = {
        key,
        route: String(context.route || '/'),
        source: String(context.source || ''),
        order: context.order >>> 0,
        moduleVersion,
        mode,
        wasmContext: 0,
        createdAt: Date.now ? Date.now() : 0,
      };
      contexts.set(key, record);
    }
    return Object.freeze({ ...record });
  }

  function destroyContext(contextOrKey = '') {
    const key = typeof contextOrKey === 'string' ? contextOrKey : String((contextOrKey && contextOrKey.key) || '');
    const record = contexts.get(key);
    if (record && record.wasmContext && wasmInstance && wasmInstance.exports && typeof wasmInstance.exports.venom_tjs_context_free === 'function') {
      wasmInstance.exports.venom_tjs_context_free(record.wasmContext >>> 0);
    }
    const destroyed = contexts.delete(key);
    return Object.freeze({ key, destroyed, wasm: !!(record && record.wasmContext) });
  }

  function contextSnapshot() {
    return Object.freeze({
      count: contexts.size,
      keys: Object.freeze(Array.from(contexts.keys())),
      moduleVersion,
      mode,
      wasmLoaded: !!wasmInstance,
      wasmUrl,
      wasmError: wasmError ? String(wasmError.message || wasmError) : '',
    });
  }

  function status() {
    return Object.freeze({ kind: 'venom.turbojs.engine.module', version: moduleVersion, mode, fallback, wasmUrl, wasmLoaded: !!wasmInstance, wasmError: wasmError ? String(wasmError.message || wasmError) : '', contexts: contexts.size, executions: executions.length });
  }

  function buildExecutionBindings(input) {
    const bindings = input && input.bindings ? input.bindings : {};
    const out = Object.create(null);
    for (const [name, value] of Object.entries(bindings)) out[name] = value;
    out.globalThis = bindings.globalThis || globalThis;
    out.window = bindings.window || out.globalThis.window || out.globalThis;
    out.self = bindings.self || out.window;
    out.navigator = bindings.navigator || (out.window && out.window.navigator) || out.globalThis.navigator;
    out.fpCollect = bindings.fpCollect || (out.window && out.window.fpCollect) || (out.globalThis && out.globalThis.fpCollect) || function venom_fpCollect_missing_global_shim() { return null; };
    out.document = bindings.document || out.globalThis.document;
    out.__venomRuntime = bindings.__venomRuntime || out.globalThis.__venomRuntime || null;
    const baseConsole = bindings.console || out.globalThis.console;
    out.console = Object.freeze(Object.fromEntries(['debug','log','info','warn','error'].map((level) => [level, (...args) => {
      const event = Object.freeze({ id: consoleEvents.length + 1, level, args: args.map((arg) => { try { return String(arg); } catch (_) { return '[unprintable]'; } }) });
      boundedPush(consoleEvents, event, maxConsoleEvents);
      if (baseConsole && typeof baseConsole[level] === 'function') baseConsole[level](...args);
    }])));
    out.fetch = bindings.fetch || out.globalThis.fetch;
    out.setTimeout = bindings.setTimeout || out.globalThis.setTimeout;
    out.setInterval = bindings.setInterval || out.globalThis.setInterval;
    out.clearTimeout = bindings.clearTimeout || out.globalThis.clearTimeout;
    out.clearInterval = bindings.clearInterval || out.globalThis.clearInterval;
    out.__venomChunk = bindings.__venomChunk || Object.freeze({});
    out.__venomCapabilities = bindings.__venomCapabilities || Object.freeze(['console', 'document', 'window', 'navigator', 'fpCollect', 'fetch', 'timers', 'events', '__venomRuntime']);
    return Object.freeze(out);
  }

  async function executeWithWasmScaffold(chunk, context) {
    const instance = await loadWasm();
    if (!instance) return null;
    const e = instance.exports;
    const bytecodeMode = !!(chunk.bytecodeBytes && chunk.bytecodeBytes.length);
    if (protectedRelease && !bytecodeMode) throw new Error('protected release rejected a non-bytecode JavaScript chunk');
    const bytes = bytecodeMode ? chunk.bytecodeBytes : encoder.encode(String(chunk.code || ''));
    const requested = bytes.length >>> 0;
    const executionPolicy = chunk && chunk.executionPolicy && typeof chunk.executionPolicy === 'object' ? chunk.executionPolicy : Object.freeze({});
    const ephemeralContext = !protectedRelease && !!chunk.bridgeCandidate && executionPolicy.isolation !== 'persistent';
    const effectiveHeapLimit = Math.min(heapLimit, executionPolicy.heapLimitBytes ? executionPolicy.heapLimitBytes >>> 0 : heapLimit) >>> 0;
    const effectiveStackLimit = Math.min(stackLimit, executionPolicy.stackLimitBytes ? executionPolicy.stackLimitBytes >>> 0 : stackLimit) >>> 0;
    const effectiveInterruptBudget = Math.min(interruptBudget, executionPolicy.interruptBudget ? executionPolicy.interruptBudget >>> 0 : interruptBudget) >>> 0;
    const effectivePendingJobLimit = Math.min(pendingJobLimit, executionPolicy.pendingJobLimit ? executionPolicy.pendingJobLimit >>> 0 : pendingJobLimit) >>> 0;
    const maxBridgeInputBytes = executionPolicy.maxBridgeInputBytes ? executionPolicy.maxBridgeInputBytes >>> 0 : 1048576;
    const maxResultBytes = executionPolicy.maxResultBytes ? executionPolicy.maxResultBytes >>> 0 : 1048576;
    let mutableContext = ephemeralContext ? null : contexts.get(context.key);
    if (!mutableContext) {
      mutableContext = { ...context, wasmContext: 0 };
      if (!ephemeralContext) contexts.set(context.key, mutableContext);
    }
    if (!mutableContext.wasmContext) mutableContext.wasmContext = e.venom_tjs_context_alloc() >>> 0;
    if (mutableContext.wasmContext && typeof e.venom_tjs_context_set_limits === 'function' && (e.venom_tjs_context_set_limits(mutableContext.wasmContext >>> 0, effectiveHeapLimit, effectiveStackLimit) >>> 0) !== 1) throw new Error('TurboJS memory limits rejected');
    if (mutableContext.wasmContext) {
      if (protectedRelease && typeof e.venom_tjs_context_set_execution_limits !== 'function') throw new Error('TurboJS release runtime lacks execution-limit support');
      if (typeof e.venom_tjs_context_set_execution_limits === 'function' && (e.venom_tjs_context_set_execution_limits(mutableContext.wasmContext >>> 0, effectiveInterruptBudget, effectivePendingJobLimit) >>> 0) !== 1) throw new Error('TurboJS execution limits rejected');
    }
    if (protectedRelease && diversification && !diversificationInstalled) {
      if (typeof e.venom_tjs_diversification_install !== 'function') throw new Error('TurboJS release runtime lacks live diversification support');
      const contract = diversification.bytes;
      const contractPtr = e.venom_tjs_script_buffer_alloc(mutableContext.wasmContext >>> 0, contract.byteLength >>> 0) >>> 0;
      const contractCap = e.venom_tjs_script_buffer_capacity(mutableContext.wasmContext >>> 0) >>> 0;
      if (!contractPtr || contract.byteLength > contractCap) throw new Error('TurboJS diversification buffer allocation failed');
      try {
        memoryView(instance).set(contract, contractPtr);
        if ((e.venom_tjs_diversification_install(contractPtr, contract.byteLength >>> 0) >>> 0) !== 1) throw new Error('TurboJS diversification contract rejected');
        diversificationInstalled = true;
      } finally {
        // The script buffer is a fixed WASM arena, not a heap allocation.  The
        // host owns the exact byte length and can wipe it safely.  Do not enter
        // the legacy release export after execution: engine state may have
        // changed and older runtimes used mutable in-WASM length accounting.
        zeroWasmRange(instance, contractPtr, contract.byteLength);
      }
    }
    let ptr = 0;
    try {
      ptr = e.venom_tjs_script_buffer_alloc(mutableContext.wasmContext >>> 0, requested) >>> 0;
    const cap = e.venom_tjs_script_buffer_capacity(mutableContext.wasmContext >>> 0) >>> 0;
    if (!ptr || requested > cap) {
      let statusText = '';
      if (typeof e.venom_tjs_status_record_ptr === 'function' && typeof e.venom_tjs_status_record_size === 'function') {
        const statusPtr = e.venom_tjs_status_record_ptr() >>> 0;
        const statusSize = e.venom_tjs_status_record_size() >>> 0;
        if (statusPtr && statusSize) statusText = decoder.decode(memoryView(instance).slice(statusPtr, statusPtr + statusSize));
      }
      throw new Error('TurboJS script byte buffer allocation failed: requested=' + requested + ' capacity=' + cap + (statusText ? ' status=' + statusText : ''));
    }
    const memory = memoryView(instance);
    const runtimeBytes = memoryRange(instance, ptr, requested, 'TurboJS script input');
    runtimeBytes.set(bytes);
    let expectedHash = fnv1a32(runtimeBytes);
    if (typeof e.venom_tjs_script_buffer_set_expected_hash === 'function') e.venom_tjs_script_buffer_set_expected_hash(mutableContext.wasmContext >>> 0, expectedHash >>> 0);
    if (bytecodeMode && typeof e.venom_tjs_bytecode_validate === 'function') {
      let validationStatus = e.venom_tjs_bytecode_validate(mutableContext.wasmContext >>> 0, requested) >>> 0;
      if (validationStatus !== 0 && runtimeBytes.byteLength >= 8 && runtimeBytes[0] === 0x56 && runtimeBytes[1] === 0x54 && runtimeBytes[2] === 0x4a && runtimeBytes[3] === 0x53 && (runtimeBytes[4] === 0x42 || runtimeBytes[4] === 0x4d)) {
        runtimeBytes[1] = 0x51;
        expectedHash = fnv1a32(runtimeBytes);
        if (typeof e.venom_tjs_script_buffer_set_expected_hash === 'function') e.venom_tjs_script_buffer_set_expected_hash(mutableContext.wasmContext >>> 0, expectedHash >>> 0);
        validationStatus = e.venom_tjs_bytecode_validate(mutableContext.wasmContext >>> 0, requested) >>> 0;
      }
      if (validationStatus !== 0) {
        const statusPtr = typeof e.venom_tjs_status_record_ptr === 'function' ? e.venom_tjs_status_record_ptr() >>> 0 : 0;
        const statusSize = typeof e.venom_tjs_status_record_size === 'function' ? e.venom_tjs_status_record_size() >>> 0 : 0;
        const statusText = statusPtr && statusSize ? decoder.decode(memory.subarray(statusPtr, statusPtr + statusSize)) : '';
        throw new Error('TurboJS bytecode validation failed: code=' + validationStatus + ' requested=' + requested + ' capacity=' + cap + (statusText ? ' status=' + statusText : ''));
      }
    }
    const moduleFlag = ((chunk.flags >>> 0) & SCRIPT_FLAG_MODULE) !== 0 || (!protectedRelease && /(^|\n)\s*(import|export)\s+/m.test(String(chunk.code || '')));
    const ok = bytecodeMode && typeof e.venom_tjs_execute_bytecode === 'function'
      ? (e.venom_tjs_execute_bytecode(mutableContext.wasmContext, requested) >>> 0)
      : ((moduleFlag && typeof e.venom_tjs_module_execute === 'function')
        ? (e.venom_tjs_module_execute(mutableContext.wasmContext, (chunk.order >>> 0) || 1, requested) >>> 0)
        : (e.venom_tjs_execute_source(mutableContext.wasmContext, requested) >>> 0));
    if (protectedRelease) {
      const compactPtr = typeof e.venom_tjs_compact_result_ptr === 'function' ? e.venom_tjs_compact_result_ptr() >>> 0 : 0;
      let abiStatus = 0;
      let exceptionCode = 0;
      let hostCallCount = 0;
      let consoleCount = 0;
      let fallbackRequired = false;
      let executed = !!ok;
      if (compactPtr) {
        const compact = new DataView(memory.buffer, memory.byteOffset + compactPtr, 32);
        const fieldMap = diversification && diversification.resultLogicalToWire ? diversification.resultLogicalToWire : new Uint32Array([0,1,2,3,4,5,6,7]);
        const field = (logical) => compact.getUint32((fieldMap[logical] >>> 0) * 4, true) >>> 0;
        if (field(0) !== 0x33524356 && field(0) !== 0x32524356) throw new Error('TurboJS compact result descriptor is invalid');
        abiStatus = field(1);
        exceptionCode = field(2);
        hostCallCount = field(3);
        consoleCount = field(4);
        fallbackRequired = field(5) !== 0;
        executed = field(7) !== 0 && !!ok;
      } else {
        throw new Error('TurboJS compact result descriptor is required');
      }
      let exceptionMessage = '';
      if (!executed && typeof e.venom_tjs_exception_message_ptr === 'function' && typeof e.venom_tjs_exception_message_size === 'function') {
        const msgPtr = e.venom_tjs_exception_message_ptr() >>> 0;
        const msgSize = e.venom_tjs_exception_message_size() >>> 0;
        if (msgPtr && msgSize) exceptionMessage = decoder.decode(memory.subarray(msgPtr, msgPtr + msgSize));
      }
      const hostBridgeTelemetry = inferWasmHostBridgeTelemetry(chunk, { turboJsWasm: true, report: null, hostCallCount, consoleCount, fallbackRequired, upstreamTurboJsReady: true });
      return Object.freeze({
        executed,
        turboJsWasm: true,
        bytecodeExecuted: bytecodeMode && executed,
        hostEffects: Object.freeze([]),
        hostBridgeTelemetry,
        hostBridgeParity: hostBridgeTelemetry,
        wasmContext: mutableContext.wasmContext,
        exceptionMessage,
        exceptionCode,
        hostCallCount,
        consoleCount,
        fallbackRequired,
        abiStatus
      });
    }
    let report = null;
    if (chunk.bridgeCandidate) {
      if (typeof e.venom_tjs_bridge_input_alloc !== 'function' ||
          typeof e.venom_tjs_bridge_input_capacity !== 'function' ||
          typeof e.venom_tjs_bridge_invoke !== 'function' ||
          typeof e.venom_tjs_bridge_result_ptr !== 'function' ||
          typeof e.venom_tjs_bridge_result_size !== 'function' ||
          typeof e.venom_tjs_bridge_release !== 'function') {
        throw new Error('TurboJS minimal bridge result ABI is unavailable');
      }
      const requestBytes = encoder.encode(JSON.stringify({
        candidate: String(chunk.bridgeCandidate),
        args: Array.isArray(chunk.bridgeArgs) ? chunk.bridgeArgs : []
      }));
      const bridgePtr = e.venom_tjs_bridge_input_alloc(mutableContext.wasmContext >>> 0, requestBytes.byteLength >>> 0) >>> 0;
      const bridgeCap = e.venom_tjs_bridge_input_capacity() >>> 0;
      if (requestBytes.byteLength > maxBridgeInputBytes) throw new Error('TurboJS bridge request exceeds configured input limit');
      if (!bridgePtr || requestBytes.byteLength > bridgeCap) throw new Error('TurboJS bridge request allocation failed');
      try {
        memoryRange(instance, bridgePtr, requestBytes.byteLength, 'TurboJS bridge request').set(requestBytes);
        const invoked = e.venom_tjs_bridge_invoke(mutableContext.wasmContext >>> 0, requestBytes.byteLength >>> 0) >>> 0;
        const resultPtr = e.venom_tjs_bridge_result_ptr() >>> 0;
        const resultSize = e.venom_tjs_bridge_result_size() >>> 0;
        if (resultSize > maxResultBytes) throw new Error('TurboJS bridge result exceeds configured result limit');
        if (!invoked || !resultPtr || !resultSize) {
          let exceptionMessage = '';
          if (typeof e.venom_tjs_exception_message_ptr === 'function' && typeof e.venom_tjs_exception_message_size === 'function') {
            const msgPtr = e.venom_tjs_exception_message_ptr() >>> 0;
            const msgSize = e.venom_tjs_exception_message_size() >>> 0;
            if (msgPtr && msgSize) exceptionMessage = decoder.decode(memory.subarray(msgPtr, msgPtr + msgSize));
          }
          throw new Error(exceptionMessage || 'TurboJS bridge result retrieval failed');
        }
        const reportText = decoder.decode(memory.subarray(resultPtr, resultPtr + resultSize));
        try { report = JSON.parse(reportText); } catch (_) { report = { raw: reportText }; }
      } finally {
        e.venom_tjs_bridge_release(mutableContext.wasmContext >>> 0);
      }
    } else if (typeof e.venom_tjs_result_ptr === 'function' && typeof e.venom_tjs_result_size === 'function') {
      const resultPtr = e.venom_tjs_result_ptr() >>> 0;
      const resultSize = e.venom_tjs_result_size() >>> 0;
      const reportText = decoder.decode(memory.subarray(resultPtr, resultPtr + resultSize));
      try { report = reportText ? JSON.parse(reportText) : null; } catch (_) { report = { ok: !!ok }; }
    } else {
      report = { ok: !!ok };
    }
    let executionRecord = report;
    if (typeof e.venom_tjs_execution_record_ptr === 'function' && typeof e.venom_tjs_execution_record_size === 'function') {
      const recPtr = e.venom_tjs_execution_record_ptr() >>> 0;
      const recSize = e.venom_tjs_execution_record_size() >>> 0;
      if (recSize > 0) {
        const recText = decoder.decode(memory.subarray(recPtr, recPtr + recSize));
        try { executionRecord = JSON.parse(recText); } catch (_) { executionRecord = { raw: recText }; }
      }
    }
    let wasmConsoleEvent = null;
    if (typeof e.venom_tjs_console_event_ptr === 'function' && typeof e.venom_tjs_console_event_size === 'function') {
      const eventPtr = e.venom_tjs_console_event_ptr() >>> 0;
      const eventSize = e.venom_tjs_console_event_size() >>> 0;
      if (eventPtr && eventSize) {
        const eventText = decoder.decode(memory.subarray(eventPtr, eventPtr + eventSize));
        try { wasmConsoleEvent = JSON.parse(eventText); } catch (_) { wasmConsoleEvent = { raw: eventText }; }
      }
    }
    const heapUsed = typeof e.venom_tjs_context_heap_used === 'function' ? e.venom_tjs_context_heap_used(mutableContext.wasmContext) >>> 0 : 0;
    const contextHeapLimit = typeof e.venom_tjs_context_heap_limit === 'function' ? e.venom_tjs_context_heap_limit(mutableContext.wasmContext) >>> 0 : 0;
    const contextStackLimit = typeof e.venom_tjs_context_stack_limit === 'function' ? e.venom_tjs_context_stack_limit(mutableContext.wasmContext) >>> 0 : 0;
    let exceptionRecord = null;
    if (typeof e.venom_tjs_exception_ptr === 'function' && typeof e.venom_tjs_exception_size === 'function') {
      const exceptionPtr = e.venom_tjs_exception_ptr() >>> 0;
      const exceptionSize = e.venom_tjs_exception_size() >>> 0;
      if (exceptionPtr && exceptionSize) {
        const exceptionText = decoder.decode(memory.subarray(exceptionPtr, exceptionPtr + exceptionSize));
        try { exceptionRecord = JSON.parse(exceptionText); } catch (_) { exceptionRecord = { raw: exceptionText }; }
      }
    }
    let exceptionMessage = '';
    if (typeof e.venom_tjs_exception_message_ptr === 'function' && typeof e.venom_tjs_exception_message_size === 'function') {
      const msgPtr = e.venom_tjs_exception_message_ptr() >>> 0;
      const msgSize = e.venom_tjs_exception_message_size() >>> 0;
      if (msgPtr && msgSize) exceptionMessage = decoder.decode(memory.subarray(msgPtr, msgPtr + msgSize));
    }
    let moduleRecord = null;
    if (typeof e.venom_tjs_module_record_ptr === 'function' && typeof e.venom_tjs_module_record_size === 'function') {
      const modulePtr = e.venom_tjs_module_record_ptr() >>> 0;
      const moduleSize = e.venom_tjs_module_record_size() >>> 0;
      if (modulePtr && moduleSize) {
        const moduleText = decoder.decode(memory.subarray(modulePtr, modulePtr + moduleSize));
        try { moduleRecord = JSON.parse(moduleText); } catch (_) { moduleRecord = { raw: moduleText }; }
      }
    }
    const readJsonExportRecord = (ptrName, sizeName) => {
      if (typeof e[ptrName] !== 'function' || typeof e[sizeName] !== 'function') return null;
      const recPtr = e[ptrName]() >>> 0;
      const recSize = e[sizeName]() >>> 0;
      if (!recPtr || !recSize) return null;
      const text = decoder.decode(memory.subarray(recPtr, recPtr + recSize));
      try { return JSON.parse(text); } catch (_) { return { raw: text }; }
    };
    const scriptRecord = readJsonExportRecord('venom_tjs_script_record_ptr', 'venom_tjs_script_record_size');
    const evalResult = report && Object.prototype.hasOwnProperty.call(report, 'result')
      ? report.result
      : readJsonExportRecord('venom_tjs_eval_result_ptr', 'venom_tjs_eval_result_size');
    const consoleCapture = report && Array.isArray(report.consoleCapture)
      ? report.consoleCapture
      : readJsonExportRecord('venom_tjs_console_capture_ptr', 'venom_tjs_console_capture_size');
    const failureReport = report && report.ok === false
      ? (report.error || report)
      : readJsonExportRecord('venom_tjs_failure_report_ptr', 'venom_tjs_failure_report_size');
    cachedExecutionJournal = readTextRecordFromExports(instance, 'venom_tjs_execution_journal_ptr', 'venom_tjs_execution_journal_size', 'venom_tjs_execution_journal_hash');
    cachedReplayCursor = readTextRecordFromExports(instance, 'venom_tjs_replay_cursor_ptr', 'venom_tjs_replay_cursor_size', 'venom_tjs_replay_cursor_hash');
    cachedResumeState = readTextRecordFromExports(instance, 'venom_tjs_resume_state_ptr', 'venom_tjs_resume_state_size', 'venom_tjs_resume_state_hash');
    cachedDeterminismAudit = readTextRecordFromExports(instance, 'venom_tjs_determinism_audit_ptr', 'venom_tjs_determinism_audit_size', 'venom_tjs_determinism_audit_hash');
    cachedSnapshotPolicy = readTextRecordFromExports(instance, 'venom_tjs_snapshot_policy_ptr', 'venom_tjs_snapshot_policy_size', 'venom_tjs_snapshot_policy_hash');
    cachedSnapshotRecord = readTextRecordFromExports(instance, 'venom_tjs_snapshot_record_ptr', 'venom_tjs_snapshot_record_size', 'venom_tjs_snapshot_record_hash');
    cachedReplayValidation = readTextRecordFromExports(instance, 'venom_tjs_replay_validation_ptr', 'venom_tjs_replay_validation_size', 'venom_tjs_replay_validation_hash');
    cachedDeterminismLedger = readTextRecordFromExports(instance, 'venom_tjs_determinism_ledger_ptr', 'venom_tjs_determinism_ledger_size', 'venom_tjs_determinism_ledger_hash');
    cachedAuditSeal = readTextRecordFromExports(instance, 'venom_tjs_audit_seal_ptr', 'venom_tjs_audit_seal_size', 'venom_tjs_audit_seal_hash');
    cachedExecutionCommit = readTextRecordFromExports(instance, 'venom_tjs_execution_commit_ptr', 'venom_tjs_execution_commit_size', 'venom_tjs_execution_commit_hash');
    cachedRollbackPolicy = readTextRecordFromExports(instance, 'venom_tjs_rollback_policy_ptr', 'venom_tjs_rollback_policy_size', 'venom_tjs_rollback_policy_hash');
    cachedHostReceipts = readTextRecordFromExports(instance, 'venom_tjs_host_receipts_ptr', 'venom_tjs_host_receipts_size', 'venom_tjs_host_receipts_hash');
    cachedReleaseAcceptance = readTextRecordFromExports(instance, 'venom_tjs_release_acceptance_ptr', 'venom_tjs_release_acceptance_size', 'venom_tjs_release_acceptance_hash');
    cachedCommitAudit = readTextRecordFromExports(instance, 'venom_tjs_commit_audit_ptr', 'venom_tjs_commit_audit_size', 'venom_tjs_commit_audit_hash');
    cachedCapabilityPolicy = readTextRecordFromExports(instance, 'venom_tjs_capability_policy_ptr', 'venom_tjs_capability_policy_size', 'venom_tjs_capability_policy_hash');
    cachedHostIoPolicy = readTextRecordFromExports(instance, 'venom_tjs_host_io_policy_ptr', 'venom_tjs_host_io_policy_size', 'venom_tjs_host_io_policy_hash');
    cachedPermissionSeal = readTextRecordFromExports(instance, 'venom_tjs_permission_seal_ptr', 'venom_tjs_permission_seal_size', 'venom_tjs_permission_seal_hash');
    cachedPolicyReceipts = readTextRecordFromExports(instance, 'venom_tjs_policy_receipts_ptr', 'venom_tjs_policy_receipts_size', 'venom_tjs_policy_receipts_hash');
    cachedReleaseGate = readTextRecordFromExports(instance, 'venom_tjs_release_gate_ptr', 'venom_tjs_release_gate_size', 'venom_tjs_release_gate_hash');
          cachedHostIoDecision = readTextRecordFromExports(instance, 'venom_tjs_host_io_decision_ptr', 'venom_tjs_host_io_decision_size', 'venom_tjs_host_io_decision_hash');
          cachedHostIoDenyTrace = readTextRecordFromExports(instance, 'venom_tjs_host_io_deny_trace_ptr', 'venom_tjs_host_io_deny_trace_size', 'venom_tjs_host_io_deny_trace_hash');
          cachedCapabilityLedger = readTextRecordFromExports(instance, 'venom_tjs_capability_ledger_ptr', 'venom_tjs_capability_ledger_size', 'venom_tjs_capability_ledger_hash');
          cachedPolicySealAudit = readTextRecordFromExports(instance, 'venom_tjs_policy_seal_audit_ptr', 'venom_tjs_policy_seal_audit_size', 'venom_tjs_policy_seal_audit_hash');
          cachedRuntimeDenylist = readTextRecordFromExports(instance, 'venom_tjs_runtime_denylist_ptr', 'venom_tjs_runtime_denylist_size', 'venom_tjs_runtime_denylist_hash');
          cachedRuntimeLimits = readTextRecordFromExports(instance, 'venom_tjs_runtime_limits_ptr', 'venom_tjs_runtime_limits_size', 'venom_tjs_runtime_limits_hash');
          cachedBackendContract = readTextRecordFromExports(instance, 'venom_tjs_backend_contract_ptr', 'venom_tjs_backend_contract_size', 'venom_tjs_backend_contract_hash');
          cachedInterpreterContract = readTextRecordFromExports(instance, 'venom_tjs_interpreter_contract_ptr', 'venom_tjs_interpreter_contract_size', 'venom_tjs_interpreter_contract_hash');
          cachedSemanticRecord = readTextRecordFromExports(instance, 'venom_tjs_interpreter_semantic_record_ptr', 'venom_tjs_interpreter_semantic_record_size', 'venom_tjs_interpreter_semantic_record_hash');
          cachedGlobalSlotRecord = readTextRecordFromExports(instance, 'venom_tjs_global_slot_record_ptr', 'venom_tjs_global_slot_record_size', 'venom_tjs_global_slot_record_hash');
          cachedUpstreamParityRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_parity_record_ptr', 'venom_tjs_upstream_parity_record_size', 'venom_tjs_upstream_parity_record_hash');
          cachedUpstreamBytecodeSemanticsRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_bytecode_semantics_record_ptr', 'venom_tjs_upstream_bytecode_semantics_record_size', 'venom_tjs_upstream_bytecode_semantics_record_hash');
          cachedUpstreamLexicalScopeRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_lexical_scope_record_ptr', 'venom_tjs_upstream_lexical_scope_record_size', 'venom_tjs_upstream_lexical_scope_record_hash');
          cachedUpstreamClosureRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_closure_record_ptr', 'venom_tjs_upstream_closure_record_size', 'venom_tjs_upstream_closure_record_hash');
          cachedUpstreamIteratorRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_iterator_record_ptr', 'venom_tjs_upstream_iterator_record_size', 'venom_tjs_upstream_iterator_record_hash');
          cachedUpstreamIntrinsicRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_intrinsic_record_ptr', 'venom_tjs_upstream_intrinsic_record_size', 'venom_tjs_upstream_intrinsic_record_hash');
          cachedUpstreamPropertyDescriptorRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_property_descriptor_record_ptr', 'venom_tjs_upstream_property_descriptor_record_size', 'venom_tjs_upstream_property_descriptor_record_hash');
          cachedUpstreamPrototypeChainRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_prototype_chain_record_ptr', 'venom_tjs_upstream_prototype_chain_record_size', 'venom_tjs_upstream_prototype_chain_record_hash');
          cachedUpstreamCallFrameRecord = readTextRecordFromExports(instance, 'venom_tjs_upstream_call_frame_record_ptr', 'venom_tjs_upstream_call_frame_record_size', 'venom_tjs_upstream_call_frame_record_hash');
    const statusRecord = (typeof e.venom_tjs_status_record_ptr === 'function' && typeof e.venom_tjs_status_record_size === 'function') ? readWasmString(instance, e.venom_tjs_status_record_ptr() >>> 0, e.venom_tjs_status_record_size() >>> 0) : '';
    const contextReport = (typeof e.venom_tjs_context_report_ptr === 'function' && typeof e.venom_tjs_context_report_size === 'function') ? readWasmString(instance, e.venom_tjs_context_report_ptr(mutableContext.wasmContext >>> 0) >>> 0, e.venom_tjs_context_report_size() >>> 0) : '';
    const wasmHostCallCount = typeof e.venom_tjs_host_call_count === 'function' ? e.venom_tjs_host_call_count() >>> 0 : 0;
    const wasmConsoleCount = typeof e.venom_tjs_console_event_count === 'function' ? e.venom_tjs_console_event_count() >>> 0 : 0;
    const wasmFallbackRequired = typeof e.venom_tjs_fallback_required === 'function' ? !!e.venom_tjs_fallback_required() : true;
    const wasmUpstreamReady = typeof e.venom_tjs_upstream_turbojs_ready === 'function' ? !!e.venom_tjs_upstream_turbojs_ready() : false;
    const hostBridgeTelemetry = inferWasmHostBridgeTelemetry(chunk, { turboJsWasm: true, report, hostCallCount: wasmHostCallCount, consoleCount: wasmConsoleCount, hostReceipts: cachedHostReceipts, hostIoDecision: cachedHostIoDecision, hostIoDenyTrace: cachedHostIoDenyTrace, fallbackRequired: wasmFallbackRequired, upstreamTurboJsReady: wasmUpstreamReady });
    if (!exceptionMessage && report && report.ok === false && report.error) exceptionMessage = String(report.error.message || report.error);
    return Object.freeze({ executed: !!ok && (!report || report.ok !== false), turboJsWasm: true, bytecodeExecuted: !!(report && report.bytecodeExecuted), hostEffects: report && Array.isArray(report.hostEffects) ? report.hostEffects : Object.freeze([]), hostBridgeTelemetry, hostBridgeParity: hostBridgeTelemetry, wasmContext: mutableContext.wasmContext, report, executionRecord, scriptRecord, evalResult, consoleCapture, failureReport, executionJournal: cachedExecutionJournal, checkpointPolicy: cachedCheckpointPolicy, replayCursor: cachedReplayCursor, resumeState: cachedResumeState, determinismAudit: cachedDeterminismAudit, snapshotPolicy: cachedSnapshotPolicy, snapshotRecord: cachedSnapshotRecord, replayValidation: cachedReplayValidation, determinismLedger: cachedDeterminismLedger, auditSeal: cachedAuditSeal, executionCommit: cachedExecutionCommit, rollbackPolicy: cachedRollbackPolicy, hostReceipts: cachedHostReceipts, releaseAcceptance: cachedReleaseAcceptance, commitAudit: cachedCommitAudit, capabilityPolicy: cachedCapabilityPolicy, hostIoPolicy: cachedHostIoPolicy, permissionSeal: cachedPermissionSeal, policyReceipts: cachedPolicyReceipts, releaseGate: cachedReleaseGate, hostIoDecision: cachedHostIoDecision, hostIoDenyTrace: cachedHostIoDenyTrace, capabilityLedger: cachedCapabilityLedger, policySealAudit: cachedPolicySealAudit, runtimeDenylist: cachedRuntimeDenylist, exceptionRecord, exceptionMessage, moduleRecord, moduleGraphRecord: cachedModuleGraph, moduleExecutionCount: typeof e.venom_tjs_module_execution_count === 'function' ? e.venom_tjs_module_execution_count() >>> 0 : 0, moduleCacheState: (typeof e.venom_tjs_module_cache_state_ptr === 'function' && typeof e.venom_tjs_module_cache_state_size === 'function') ? readWasmString(instance, e.venom_tjs_module_cache_state_ptr() >>> 0, e.venom_tjs_module_cache_state_size() >>> 0) : '', resolverAuditState: (typeof e.venom_tjs_resolver_audit_ptr === 'function' && typeof e.venom_tjs_resolver_audit_size === 'function') ? readWasmString(instance, e.venom_tjs_resolver_audit_ptr() >>> 0, e.venom_tjs_resolver_audit_size() >>> 0) : '', interopFallbackRequired: typeof e.venom_tjs_interop_fallback_required === 'function' ? !!e.venom_tjs_interop_fallback_required() : false, bytecodeManifestHash: typeof e.venom_tjs_bytecode_manifest_hash === 'function' ? String(e.venom_tjs_bytecode_manifest_hash() >>> 0) : '', moduleResolverAbi: typeof e.venom_tjs_module_resolver_abi === 'function' ? e.venom_tjs_module_resolver_abi() >>> 0 : 0, exceptionAbi: typeof e.venom_tjs_exception_abi === 'function' ? e.venom_tjs_exception_abi() >>> 0 : 0, exceptionCode: typeof e.venom_tjs_exception_code === 'function' ? e.venom_tjs_exception_code() >>> 0 : 0, hostTrapPolicyHash: typeof e.venom_tjs_host_trap_policy_hash === 'function' ? String(e.venom_tjs_host_trap_policy_hash() >>> 0) : '', fallbackRequired: wasmFallbackRequired, backendKind: typeof e.venom_tjs_backend_kind === 'function' ? e.venom_tjs_backend_kind() >>> 0 : 0, backendReady: typeof e.venom_tjs_backend_ready === 'function' ? !!e.venom_tjs_backend_ready() : false, realEngineCandidate: typeof e.venom_tjs_real_engine_candidate === 'function' ? !!e.venom_tjs_real_engine_candidate() : false, engineState: typeof e.venom_tjs_engine_state === 'function' ? e.venom_tjs_engine_state() >>> 0 : 0, engineStateName: typeof e.venom_tjs_engine_state === 'function' ? lifecycleStateName(e.venom_tjs_engine_state() >>> 0) : 'unknown', engineTrapCode: typeof e.venom_tjs_engine_trap_code === 'function' ? e.venom_tjs_engine_trap_code() >>> 0 : 0, hostCallCount: wasmHostCallCount, scriptBufferAllocCount: typeof e.venom_tjs_script_buffer_alloc_count === 'function' ? e.venom_tjs_script_buffer_alloc_count() >>> 0 : 0, scriptBufferFreeCount: typeof e.venom_tjs_script_buffer_free_count === 'function' ? e.venom_tjs_script_buffer_free_count() >>> 0 : 0, consoleCount: wasmConsoleCount, wasmConsoleEvent, heapUsed, heapLimit: contextHeapLimit, stackLimit: contextStackLimit, scriptBufferBytes: typeof e.venom_tjs_script_buffer_size === 'function' ? e.venom_tjs_script_buffer_size(mutableContext.wasmContext) >>> 0 : requested, expectedHash, parityProbe: cachedParityProbe, abiStatus: typeof e.venom_tjs_status_code === 'function' ? e.venom_tjs_status_code() >>> 0 : 0, releaseStatus: typeof e.venom_tjs_release_status === 'function' ? e.venom_tjs_release_status() >>> 0 : 0, statusRecord, contextReport, runtimeLimits: cachedRuntimeLimits, backendContract: cachedBackendContract, interpreterContract: cachedInterpreterContract, interpreterReady: typeof e.venom_tjs_interpreter_ready === 'function' ? !!e.venom_tjs_interpreter_ready() : false, interpreterDispatchCount: typeof e.venom_tjs_interpreter_dispatch_count === 'function' ? e.venom_tjs_interpreter_dispatch_count() >>> 0 : 0, interpreterOpcodeCount: typeof e.venom_tjs_interpreter_opcode_count === 'function' ? e.venom_tjs_interpreter_opcode_count() >>> 0 : 0, interpreterGlobalSlotCount: typeof e.venom_tjs_global_slot_count === 'function' ? e.venom_tjs_global_slot_count() >>> 0 : 0, interpreterLastGlobalWriteHash: typeof e.venom_tjs_last_global_write_hash === 'function' ? e.venom_tjs_last_global_write_hash() >>> 0 : 0, interpreterSemanticPassCount: typeof e.venom_tjs_interpreter_semantic_pass_count === 'function' ? e.venom_tjs_interpreter_semantic_pass_count() >>> 0 : 0, interpreterPropertyWriteCount: typeof e.venom_tjs_interpreter_property_write_count === 'function' ? e.venom_tjs_interpreter_property_write_count() >>> 0 : 0, interpreterBuiltinProbeCount: typeof e.venom_tjs_interpreter_builtin_probe_count === 'function' ? e.venom_tjs_interpreter_builtin_probe_count() >>> 0 : 0, interpreterConsoleCallCount: typeof e.venom_tjs_interpreter_console_call_count === 'function' ? e.venom_tjs_interpreter_console_call_count() >>> 0 : 0, interpreterSemanticRecord: cachedSemanticRecord, interpreterGlobalSlotRecord: cachedGlobalSlotRecord, upstreamTurboJsReady: wasmUpstreamReady, upstreamParityRecord: cachedUpstreamParityRecord, upstreamBytecodeSemanticsRecord: cachedUpstreamBytecodeSemanticsRecord, upstreamLexicalScopeRecord: cachedUpstreamLexicalScopeRecord, upstreamClosureRecord: cachedUpstreamClosureRecord, upstreamIteratorRecord: cachedUpstreamIteratorRecord, upstreamIntrinsicRecord: cachedUpstreamIntrinsicRecord, upstreamPropertyDescriptorRecord: cachedUpstreamPropertyDescriptorRecord, upstreamPrototypeChainRecord: cachedUpstreamPrototypeChainRecord, upstreamCallFrameRecord: cachedUpstreamCallFrameRecord, upstreamIntrinsicSemanticsScore: typeof e.venom_tjs_upstream_intrinsic_semantics_score === 'function' ? e.venom_tjs_upstream_intrinsic_semantics_score() >>> 0 : 0, upstreamBytecodeSemanticsScore: typeof e.venom_tjs_upstream_bytecode_semantics_score === 'function' ? e.venom_tjs_upstream_bytecode_semantics_score() >>> 0 : 0, upstreamFeatureCount: typeof e.venom_tjs_upstream_feature_count === 'function' ? e.venom_tjs_upstream_feature_count() >>> 0 : 0, upstreamBuiltinCount: typeof e.venom_tjs_upstream_builtin_count === 'function' ? e.venom_tjs_upstream_builtin_count() >>> 0 : 0, upstreamObjectModelScore: typeof e.venom_tjs_upstream_object_model_score === 'function' ? e.venom_tjs_upstream_object_model_score() >>> 0 : 0, upstreamExceptionModelScore: typeof e.venom_tjs_upstream_exception_model_score === 'function' ? e.venom_tjs_upstream_exception_model_score() >>> 0 : 0, upstreamModuleModelScore: typeof e.venom_tjs_upstream_module_model_score === 'function' ? e.venom_tjs_upstream_module_model_score() >>> 0 : 0, bytecodeRecordHash32: typeof e.venom_tjs_bytecode_record_hash32 === 'function' ? e.venom_tjs_bytecode_record_hash32() >>> 0 : 0, bytecodePayloadSize: typeof e.venom_tjs_bytecode_payload_size === 'function' ? e.venom_tjs_bytecode_payload_size() >>> 0 : 0, bytecodeExpectedSourceHash32: typeof e.venom_tjs_bytecode_expected_source_hash32 === 'function' ? e.venom_tjs_bytecode_expected_source_hash32() >>> 0 : 0 });
    } finally {
      zeroWasmRange(instance, ptr, requested);
      if (ephemeralContext && mutableContext.wasmContext && typeof e.venom_tjs_context_free === 'function') {
        e.venom_tjs_context_free(mutableContext.wasmContext >>> 0);
        mutableContext.wasmContext = 0;
      }
    }
  }

  function normalizeModuleSpecifier(specifier, referrer = '') {
    const raw = String(specifier || '').trim();
    if (!raw) return '';
    if (/^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(raw) || raw.startsWith('//')) return 'blocked:' + raw;
    if (!raw.startsWith('.') && !raw.startsWith('/')) return raw.replace(/^\/+/, '');
    const base = String(referrer || '').split('/');
    if (base.length) base.pop();
    for (const part of raw.split('/')) {
      if (!part || part === '.') continue;
      if (part === '..') base.pop();
      else base.push(part);
    }
    return base.filter(Boolean).join('/');
  }

  function recordResolverAudit(specifier, referrer, normalized, status) {
    const item = Object.freeze({ id: resolverAuditRecords.length + 1, specifier: String(specifier || ''), referrer: String(referrer || ''), normalized: String(normalized || ''), status: String(status || 'unknown'), hostCallId: 4 });
    resolverAuditRecords.push(item);
    return item;
  }

  function importModuleNamespace(specifier, referrer) {
    const normalized = normalizeModuleSpecifier(specifier, referrer);
    if (!normalized || normalized.startsWith('blocked:')) {
      recordResolverAudit(specifier, referrer, normalized, 'blocked');
      if (config.resolverAudit && String(config.resolverAudit.unknownSpecifier || '').includes('trap')) throw new Error('TurboJS module resolver blocked ' + specifier);
      return Object.freeze({});
    }
    const value = moduleNamespaceCache.get(normalized) || moduleNamespaceCache.get(String(specifier || '')) || null;
    recordResolverAudit(specifier, referrer, normalized, value ? 'resolved-cache' : 'missing-empty-namespace');
    return value || Object.freeze({});
  }


  function sourceDeclaresInjectedBinding(source, name) {
    const id = String(name || '');
    if (!/^[A-Za-z_$][\w$]*$/.test(id)) return false;
    const pattern = new RegExp('(^|[^A-Za-z0-9_$])(?:const|let|class|function)\\s+' + id + '\\b');
    return pattern.test(String(source || ''));
  }

  function exportBindingStatements(items) {
    const out = [];
    for (const item of items) {
      const local = item.local || item.name;
      const exported = item.exported || item.name;
      if (local && exported) out.push('__venomExport(' + JSON.stringify(exported) + ', ' + local + ');');
    }
    return out.join('\n');
  }

  function transformModuleSource(source, chunk) {
    let code = String(source || '');
    if (/\bimport\s*\(/.test(code)) throw new Error('dynamic import is trapped by TurboJS module execution prototype');
    if (/\bawait\b/.test(code) && /^\s*await\b/m.test(code)) throw new Error('top-level await is not enabled in TurboJS module execution prototype');
    const imports = [];
    const exports = [];
    code = code.replace(/import\s+\*\s+as\s+([A-Za-z_$][\w$]*)\s+from\s+['"]([^'"]+)['"]\s*;?/g, (_, ns, spec) => { imports.push(spec); return 'const ' + ns + ' = __venomImport(' + JSON.stringify(spec) + ');'; });
    code = code.replace(/import\s+\{([^}]+)\}\s+from\s+['"]([^'"]+)['"]\s*;?/g, (_, names, spec) => {
      imports.push(spec);
      const bindings = names.split(',').map((part) => {
        const bits = part.trim().split(/\s+as\s+/);
        return bits.length === 2 ? bits[0].trim() + ': ' + bits[1].trim() : bits[0].trim();
      }).filter(Boolean).join(', ');
      return 'const { ' + bindings + ' } = __venomImport(' + JSON.stringify(spec) + ');';
    });
    code = code.replace(/import\s+([A-Za-z_$][\w$]*)\s+from\s+['"]([^'"]+)['"]\s*;?/g, (_, name, spec) => { imports.push(spec); return 'const ' + name + ' = __venomImport(' + JSON.stringify(spec) + ').default;'; });
    code = code.replace(/import\s+['"]([^'"]+)['"]\s*;?/g, (_, spec) => { imports.push(spec); return '__venomImport(' + JSON.stringify(spec) + ');'; });
    code = code.replace(/export\s+function\s+([A-Za-z_$][\w$]*)\s*\(/g, (_, name) => { exports.push({ name }); return 'function ' + name + '('; });
    code = code.replace(/export\s+(const|let|var)\s+([A-Za-z_$][\w$]*)\s*=/g, (_, kind, name) => { exports.push({ name }); return kind + ' ' + name + ' ='; });
    code = code.replace(/export\s+default\s+([^;]+);?/g, (_, expr) => { exports.push({ local: '__venomDefaultExport', exported: 'default' }); return 'const __venomDefaultExport = ' + expr + ';'; });
    code = code.replace(/export\s+\{([^}]+)\}\s*;?/g, (_, names) => {
      const items = names.split(',').map((part) => {
        const bits = part.trim().split(/\s+as\s+/);
        return bits.length === 2 ? { local: bits[0].trim(), exported: bits[1].trim() } : { local: bits[0].trim(), exported: bits[0].trim() };
      }).filter((item) => item.local);
      exports.push(...items);
      return exportBindingStatements(items);
    });
    const tail = exportBindingStatements(exports.filter((item) => item.name));
    return Object.freeze({ code: tail ? code + '\n' + tail : code, imports: Object.freeze(imports.slice()), exports: Object.freeze(exports.map((item) => item.exported || item.name || 'default')) });
  }

  async function executeChunk(input = {}) {
    let chunk = input.chunk || {};
    validateBytecodeTrustHandoff(chunk);
    const context = createContext(input.context || {});
    const bindings = buildExecutionBindings(input);
    if ((!chunk.code || !String(chunk.code).trim()) && !(chunk.bytecodeBytes && chunk.bytecodeBytes.length)) {
      const empty = Object.freeze({ executed: false, empty: true, engineModule: true, moduleVersion, mode, context: context.key, turboJsWasm: !!wasmUrl });
      boundedPush(executions, empty, maxExecutionRecords);
      return empty;
    }
    const fallbackText = String(fallback || '');
    const policyAllowsFunction = !!(config.policy && config.policy.allowFunctionConstructor);
    const policyAllowsEval = !!(config.policy && config.policy.allowEval);
    const fallbackPolicyStrict = !!(config.fallbackPolicy && (config.fallbackPolicy.enabled === false || config.fallbackPolicy.strictRelease || String(config.fallbackPolicy.currentReleasePolicy || '').startsWith('deny-host-fallback')));
    const fallbackDenied = fallbackPolicyStrict || ((!policyAllowsFunction && !policyAllowsEval) && (fallbackText.includes('deny-host-js-source-eval') || fallbackText.includes('fail-closed') || fallbackText.includes('deny-host-js')));
    const wasmResult = await enqueueWasmExecution(() => executeWithWasmScaffold(chunk, context));
    if (wasmResult) {
      boundedPush(resultRecords, Object.freeze({ id: resultRecords.length + 1, route: chunk.route || '', source: chunk.source || '', order: chunk.order >>> 0, wasmReport: wasmResult.report, executionRecord: wasmResult.executionRecord, fallbackRequired: wasmResult.fallbackRequired, hostBridgeTelemetry: wasmResult.hostBridgeTelemetry, hostBridgeParity: wasmResult.hostBridgeParity }), maxExecutionRecords);
      // v0.37 records the WASM backend result and falls back only through the explicit policy gate.
      if (bindings.console && typeof bindings.console.debug === 'function') {
        bindings.console.debug('[venom] TurboJS WASM backend accepted bytecode chunk', chunk.source || 'inline');
      }
      if (fallbackDenied && wasmResult) {
        const bridgeReplay = replayWasmHostBridgeEffects(chunk, wasmResult.hostBridgeTelemetry, bindings);
        const nativeHostEffectCount = Array.isArray(wasmResult.hostEffects) ? wasmResult.hostEffects.length : 0;
        const hostBridgeParity = Object.freeze({ ...(wasmResult.hostBridgeTelemetry || {}), effectReplay: bridgeReplay, effectReplayCount: 0, nativeHostEffectCount, actualHostEffects: nativeHostEffectCount > 0, replayMode: bridgeReplay.mode, replayEvalUsed: false, replayFunctionConstructorUsed: false });
        const effectsApplied = applyWasmHostEffects(wasmResult.hostEffects, bindings);
        const backendAccepted = !!(wasmResult.executed || wasmResult.report || wasmResult.executionRecord);
        const result = Object.freeze({ executed: backendAccepted, engineModule: true, moduleVersion, mode, context: context.key, fallback: 'denied-host-js-source-eval', turboJsWasm: true, bytecodeExecuted: !!wasmResult.bytecodeExecuted, wasmReport: wasmResult.report, wasmExecutionRecord: wasmResult.executionRecord, fallbackRequired: false, hostEffectsApplied: effectsApplied, hostBridgeEffectReplay: bridgeReplay, module: false, consoleEvents: consoleEvents.length, resultRecordCount: resultRecords.length, capabilities: bindings.__venomCapabilities ? bindings.__venomCapabilities.length : 0, sourceEvalUsed: false, hostBridgeTelemetry: hostBridgeParity, hostBridgeParity });
        boundedPush(executions, result, maxExecutionRecords);
        return result;
      }
      if (wasmResult.fallbackRequired && fallbackDenied) throw new Error('TurboJS fallback policy denied backend fallback');
    }
    if (!chunk.code && chunk.bytecodeBytes && chunk.bytecodeBytes.length) {
      if (fallbackDenied) throw new Error('TurboJS WASM interpreter unavailable; source-decode fallback denied: ' + wasmFailureDetail());
      chunk = { ...chunk, code: decodeTurboJsRecordForDeniedFallback(chunk.bytecodeBytes, chunk.source) };
    }
    if (fallbackDenied) throw new Error('TurboJS/WASM backend failed; protected host source execution is denied');
    const isModule = ((chunk.flags >>> 0) & SCRIPT_FLAG_MODULE) !== 0 || /(^|\n)\s*(import|export)\s+/m.test(String(chunk.code || ''));
    const moduleNamespace = Object.create(null);
    const moduleImportRecords = [];
    const moduleExportRecords = [];
    const __venomImport = (specifier) => {
      const namespace = importModuleNamespace(specifier, chunk.source || context.source || '');
      moduleImportRecords.push(Object.freeze({ specifier: String(specifier || ''), resolved: normalizeModuleSpecifier(specifier, chunk.source || context.source || '') }));
      return namespace;
    };
    const __venomExport = (name, value) => {
      moduleNamespace[String(name || 'default')] = value;
      moduleExportRecords.push(String(name || 'default'));
      return value;
    };
    const transformed = isModule ? transformModuleSource(chunk.code, chunk) : Object.freeze({ code: String(chunk.code || ''), imports: Object.freeze([]), exports: Object.freeze([]) });
    const evalBindings = isModule ? Object.freeze({ ...bindings, __venomImport, __venomExport }) : bindings;
__VENOM_TURBOJS_FALLBACK_BLOCK__
    const g = bindings.globalThis || globalThis;
    g.__venomTurboJsModuleProbe = (g.__venomTurboJsModuleProbe || 0) + 1;
    let frozenNamespace = null;
    if (isModule) {
      frozenNamespace = Object.freeze({ ...moduleNamespace });
      const cacheKey = normalizeModuleSpecifier(chunk.source || String(chunk.order >>> 0), chunk.source || context.source || '');
      moduleNamespaceCache.set(cacheKey, frozenNamespace);
      if (chunk.source) moduleNamespaceCache.set(String(chunk.source), frozenNamespace);
      boundedPush(moduleExecutions, Object.freeze({ id: moduleExecutions.length + 1, source: String(chunk.source || ''), route: String(chunk.route || context.route || '/'), imports: Object.freeze(moduleImportRecords.slice()), exports: Object.freeze(moduleExportRecords.slice()), cacheKey }), maxExecutionRecords);
    }
    const result = Object.freeze({
      executed: true,
      engineModule: true,
      moduleVersion,
      mode,
      context: context.key,
      fallback,
      turboJsWasm: !!wasmResult,
      wasmReport: wasmResult ? wasmResult.report : null,
      wasmExecutionRecord: wasmResult ? wasmResult.executionRecord : null,
      hostBridgeTelemetry: wasmResult ? wasmResult.hostBridgeTelemetry : null,
      hostBridgeParity: wasmResult ? wasmResult.hostBridgeParity : null,
      fallbackRequired: wasmResult ? wasmResult.fallbackRequired : false,
      module: isModule,
      moduleImports: moduleImportRecords.length,
      moduleExports: moduleExportRecords.length,
      moduleCacheSize: moduleNamespaceCache.size,
      resolverAuditCount: resolverAuditRecords.length,
      consoleEvents: consoleEvents.length,
      resultRecordCount: resultRecords.length,
      capabilities: bindings.__venomCapabilities ? bindings.__venomCapabilities.length : 0,
    });
    boundedPush(executions, result, maxExecutionRecords);
    return result;
  }

  function abiTable() {
    return cachedAbiTable || Object.freeze({ abi: config.runtimeAbi && config.runtimeAbi.abi ? config.runtimeAbi.abi : 8, packageVersion: config.runtimeAbi && config.runtimeAbi.packageVersion ? config.runtimeAbi.packageVersion : 32, entryCount: config.runtimeAbi && config.runtimeAbi.entryCount ? config.runtimeAbi.entryCount : 0, tableHash: config.runtimeAbi && config.runtimeAbi.tableHash ? config.runtimeAbi.tableHash : '', table: config.runtimeAbi && config.runtimeAbi.table ? config.runtimeAbi.table : '' });
  }

  function hostImportTable() {
    return cachedHostImportTable || Object.freeze({ importCount: config.hostImports && config.hostImports.importCount ? config.hostImports.importCount : 0, tableHash: config.hostImports && config.hostImports.tableHash ? config.hostImports.tableHash : '', table: config.hostImports && config.hostImports.table ? config.hostImports.table : '', design: config.hostImports && config.hostImports.design ? config.hostImports.design : '' });
  }

  function parityProbe() {
    return cachedParityProbe || Object.freeze({ expected: config.parityProbe && config.parityProbe.expected ? config.parityProbe.expected : 'turbojs:3', native: config.parityProbe && config.parityProbe.native ? config.parityProbe.native : '', wasm: config.parityProbe && config.parityProbe.wasm ? config.parityProbe.wasm : '', matched: false, loaded: false });
  }

  function bytecodeManifest() {
    return cachedBytecodeManifest || Object.freeze({ version: config.bytecodeManifest && config.bytecodeManifest.version ? config.bytecodeManifest.version : 0, format: config.bytecodeManifest && config.bytecodeManifest.format ? config.bytecodeManifest.format : '', chunkCount: config.bytecodeManifest && config.bytecodeManifest.chunkCount ? config.bytecodeManifest.chunkCount : 0, loaded: false });
  }

  function moduleResolver() {
    return Object.freeze({ version: config.moduleResolver && config.moduleResolver.version ? config.moduleResolver.version : 0, abi: config.moduleResolver && config.moduleResolver.abi ? config.moduleResolver.abi : 0, mode: config.moduleResolver && config.moduleResolver.mode ? config.moduleResolver.mode : '' });
  }

  function exceptionAbi() {
    return Object.freeze({ version: config.exceptionAbi && config.exceptionAbi.version ? config.exceptionAbi.version : 0, abi: config.exceptionAbi && config.exceptionAbi.abi ? config.exceptionAbi.abi : 0, schema: config.exceptionAbi && config.exceptionAbi.schema ? config.exceptionAbi.schema : '' });
  }

  function hostTrapPolicy() {
    return cachedHostTrapPolicy || Object.freeze({ version: config.hostTrapPolicy && config.hostTrapPolicy.version ? config.hostTrapPolicy.version : 0, policy: config.hostTrapPolicy && config.hostTrapPolicy.policy ? config.hostTrapPolicy.policy : '', unknownImport: config.hostTrapPolicy && config.hostTrapPolicy.unknownImport ? config.hostTrapPolicy.unknownImport : '', loaded: false });
  }

  function executionLifecycle() {
    return Object.freeze({ version: config.executionLifecycle && config.executionLifecycle.version ? config.executionLifecycle.version : 0, states: config.executionLifecycle && config.executionLifecycle.states ? config.executionLifecycle.states : 'created|configured|loaded|executing|trapped|disposed', strictRelease: !!(config.executionLifecycle && config.executionLifecycle.strictRelease) });
  }

  function scriptBufferPolicy() {
    return Object.freeze({ version: config.scriptBufferPolicy && config.scriptBufferPolicy.version ? config.scriptBufferPolicy.version : 0, maxScriptBytes: config.scriptBufferPolicy && config.scriptBufferPolicy.maxScriptBytes ? config.scriptBufferPolicy.maxScriptBytes : 786432, validateHashBeforeExecute: !(config.scriptBufferPolicy && config.scriptBufferPolicy.validateHashBeforeExecute === false) });
  }

  function contextLimitPolicy() {
    return Object.freeze({ version: config.contextLimitPolicy && config.contextLimitPolicy.version ? config.contextLimitPolicy.version : 0, maxHeapBytes: config.contextLimitPolicy && config.contextLimitPolicy.maxHeapBytes ? config.contextLimitPolicy.maxHeapBytes : heapLimit, maxStackBytes: config.contextLimitPolicy && config.contextLimitPolicy.maxStackBytes ? config.contextLimitPolicy.maxStackBytes : stackLimit, maxHostCalls: config.contextLimitPolicy && config.contextLimitPolicy.maxHostCalls ? config.contextLimitPolicy.maxHostCalls : 4096 });
  }

  function hostCallDispatch() {
    return Object.freeze({ version: config.hostCallDispatch && config.hostCallDispatch.version ? config.hostCallDispatch.version : 0, entryCount: config.hostCallDispatch && config.hostCallDispatch.entryCount ? config.hostCallDispatch.entryCount : 0, unknownHostCall: config.hostCallDispatch && config.hostCallDispatch.unknownHostCall ? config.hostCallDispatch.unknownHostCall : 'deny', calls: config.hostCallDispatch && config.hostCallDispatch.calls ? config.hostCallDispatch.calls : Object.freeze([]) });
  }

  function parityContract() {
    return Object.freeze({ version: config.parityContract && config.parityContract.version ? config.parityContract.version : 0, compare: config.parityContract && config.parityContract.compare ? config.parityContract.compare : '', releaseOnMismatch: config.parityContract && config.parityContract.releaseOnMismatch ? config.parityContract.releaseOnMismatch : 'fail-closed' });
  }

  function releaseFailClosed() {
    return Object.freeze({ version: config.releaseFailClosed && config.releaseFailClosed.version ? config.releaseFailClosed.version : 0, releasePolicy: config.releaseFailClosed && config.releaseFailClosed.releasePolicy ? config.releaseFailClosed.releasePolicy : 'fail-closed', failOn: config.releaseFailClosed && config.releaseFailClosed.failOn ? config.releaseFailClosed.failOn : '' });
  }

  function moduleGraph() {
    return Object.freeze({ version: config.moduleGraph && config.moduleGraph.version ? config.moduleGraph.version : 0, moduleCount: config.moduleGraph && config.moduleGraph.moduleCount ? config.moduleGraph.moduleCount : 0, modules: config.moduleGraph && config.moduleGraph.modules ? config.moduleGraph.modules : Object.freeze([]), cacheSize: moduleNamespaceCache.size, executions: moduleExecutions.length, resolverAudits: resolverAuditRecords.length, wasm: cachedModuleGraph || null });
  }

  function moduleCacheSnapshot() {
    const entries = [];
    for (const [key, namespace] of moduleNamespaceCache.entries()) entries.push(Object.freeze({ key, exports: Object.freeze(Object.keys(namespace || {})) }));
    return Object.freeze({ version: config.moduleCache && config.moduleCache.version ? config.moduleCache.version : 0, size: moduleNamespaceCache.size, entries: Object.freeze(entries) });
  }

  function resolverAudit() {
    return Object.freeze({ version: config.resolverAudit && config.resolverAudit.version ? config.resolverAudit.version : 0, count: resolverAuditRecords.length, records: Object.freeze(resolverAuditRecords.slice()) });
  }

  function interopFallback() {
    return Object.freeze({ version: config.interopFallback && config.interopFallback.version ? config.interopFallback.version : 0, kind: config.interopFallback && config.interopFallback.fallbackKind ? config.interopFallback.fallbackKind : 'host-esm-transform-prototype', releaseBehavior: config.interopFallback && config.interopFallback.releaseBehavior ? config.interopFallback.releaseBehavior : 'fail-closed-if-required-contract-missing' });
  }

  function executionJournal() {
    return Object.freeze({ version: config.executionJournal && config.executionJournal.version ? config.executionJournal.version : 0, records: Object.freeze(executions.slice()), wasm: cachedExecutionJournal || null });
  }

  function checkpointPolicy() {
    return Object.freeze({ version: config.checkpointPolicy && config.checkpointPolicy.version ? config.checkpointPolicy.version : 0, maxCheckpoints: config.checkpointPolicy && config.checkpointPolicy.maxCheckpoints ? config.checkpointPolicy.maxCheckpoints : 64, capture: config.checkpointPolicy && config.checkpointPolicy.capture ? config.checkpointPolicy.capture : '', wasm: cachedCheckpointPolicy || null });
  }

  function replayCursor() {
    return Object.freeze({ version: config.replayCursor && config.replayCursor.version ? config.replayCursor.version : 0, sequence: executions.length, wasm: cachedReplayCursor || null });
  }

  function resumeState() {
    return Object.freeze({ version: config.resumeState && config.resumeState.version ? config.resumeState.version : 0, contexts: contexts.size, executions: executions.length, wasm: cachedResumeState || null });
  }

  function determinismAudit() {
    return Object.freeze({ version: config.determinismAudit && config.determinismAudit.version ? config.determinismAudit.version : 0, executions: executions.length, hostImports: config.hostImports && config.hostImports.importCount ? config.hostImports.importCount : 0, wasm: cachedDeterminismAudit || null });
  }

  return Object.freeze({
    kind: 'venom.turbojs.engine.module',
    version: moduleVersion,
    mode,
    fallback,
    wasmUrl,
    createContext,
    destroyContext,
    contextSnapshot,
    status,
    executionSnapshot() { return Object.freeze({ count: executions.length, executions: Object.freeze(executions.slice()), resultRecords: Object.freeze(resultRecords.slice()) }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; return count; },
    fallbackPolicy() { return Object.freeze({ mode: fallback, configured: !!config.fallbackPolicy, strictRelease: !!(config.fallbackPolicy && config.fallbackPolicy.strictRelease) }); },
    abiTable,
    hostImportTable,
    parityProbe,
    bytecodeManifest,
    moduleResolver,
    exceptionAbi,
    hostTrapPolicy,
    executionLifecycle,
    scriptBufferPolicy,
    contextLimitPolicy,
    hostCallDispatch,
    parityContract,
    releaseFailClosed,
    moduleGraph,
    moduleCacheSnapshot,
    resolverAudit,
    interopFallback,
    executionJournal,
    checkpointPolicy,
    replayCursor,
    resumeState,
    determinismAudit,
    snapshotPolicy() { return cachedSnapshotPolicy || Object.freeze({ loaded: false }); },
    snapshotRecord() { return cachedSnapshotRecord || Object.freeze({ loaded: false }); },
    replayValidation() { return cachedReplayValidation || Object.freeze({ loaded: false }); },
    determinismLedger() { return cachedDeterminismLedger || Object.freeze({ loaded: false }); },
    auditSeal() { return cachedAuditSeal || Object.freeze({ loaded: false }); },
    executionCommit() { return cachedExecutionCommit || Object.freeze({ loaded: false }); },
    rollbackPolicy() { return cachedRollbackPolicy || Object.freeze({ loaded: false }); },
    hostCallReceipts() { return cachedHostReceipts || Object.freeze({ loaded: false }); },
    releaseAcceptance() { return cachedReleaseAcceptance || Object.freeze({ loaded: false }); },
    commitAudit() { return cachedCommitAudit || Object.freeze({ loaded: false }); },
    capabilityPolicy() { return cachedCapabilityPolicy || Object.freeze({ loaded: false }); },
    hostIoPolicy() { return cachedHostIoPolicy || Object.freeze({ loaded: false }); },
    permissionSeal() { return cachedPermissionSeal || Object.freeze({ loaded: false }); },
    policyReceipts() { return cachedPolicyReceipts || Object.freeze({ loaded: false }); },
    releaseGate() { return cachedReleaseGate || Object.freeze({ loaded: false }); },
    hostIoDecision() { return cachedHostIoDecision || Object.freeze({ loaded: false }); },
    hostIoDenyTrace() { return cachedHostIoDenyTrace || Object.freeze({ loaded: false }); },
    capabilityLedger() { return cachedCapabilityLedger || Object.freeze({ loaded: false }); },
    policySealAudit() { return cachedPolicySealAudit || Object.freeze({ loaded: false }); },
    runtimeDenylist() { return cachedRuntimeDenylist || Object.freeze({ loaded: false }); },
    executeChunk,
  });
}
)TJSENGINE";
  {
    const std::string required_begin = "  const RELEASE_ABI_REQUIRED_EXPORTS = Object.freeze([";
    const std::string required_end = "  ]);";
    const auto begin = js.find(required_begin);
    const auto end = begin == std::string::npos ? std::string::npos : js.find(required_end, begin);
    if (begin == std::string::npos || end == std::string::npos) throw std::runtime_error("TurboJS ABI export marker missing");
    std::ostringstream generated;
    generated << required_begin << "\n";
    for (const auto name : generated::turbojs_wasm_abi::required_exports) generated << "    '" << name << "',\n";
    generated << required_end;
    js.replace(begin, end + required_end.size() - begin, generated.str());

    const std::string fingerprint_marker = "  const RELEASE_ABI_FINGERPRINT = 0;";
    const auto fingerprint_pos = js.find(fingerprint_marker);
    if (fingerprint_pos == std::string::npos) throw std::runtime_error("TurboJS ABI fingerprint marker missing");
    js.replace(
        fingerprint_pos,
        fingerprint_marker.size(),
        "  const RELEASE_ABI_FINGERPRINT = " +
            std::to_string(generated::turbojs_wasm_abi::release_abi_fingerprint) + ";");

    const std::string approved_begin = "  const RELEASE_ABI_APPROVED_TOOLCHAIN_EXPORTS = new Map([";
    const auto approved_pos = js.find(approved_begin);
    const auto approved_end = approved_pos == std::string::npos ? std::string::npos : js.find("  ]);", approved_pos);
    if (approved_pos == std::string::npos || approved_end == std::string::npos) throw std::runtime_error("TurboJS toolchain ABI marker missing");
    std::ostringstream approved;
    approved << approved_begin << "\n";
    for (const auto name : generated::turbojs_wasm_abi::allowed_toolchain_exports) {
      const auto kind = name == "__indirect_function_table" ? "table" : "function";
      approved << "    ['" << name << "', '" << kind << "'],\n";
    }
    approved << "  ]);";
    js.replace(approved_pos, approved_end + 5u - approved_pos, approved.str());
  }

  const std::string fallback_marker = "__VENOM_TURBOJS_FALLBACK_BLOCK__";
  const auto fallback_pos = js.find(fallback_marker);
  if (fallback_pos == std::string::npos) throw std::runtime_error("TurboJS module generation marker missing");
  const std::string fallback_block = protected_release
      ? "    throw new Error('TurboJS/WASM execution failed; protected host fallback is unavailable');\n"
      : "    const names = Object.keys(evalBindings).filter((name) => !sourceDeclaresInjectedBinding(transformed.code, name));\n"
        "    const values = names.map((name) => evalBindings[name]);\n"
        "    const wrapped = '\"use strict\";\\n' + transformed.code + '\\n//# sourceURL=venom-tjs-module://' + safeSourceUrl(chunk.source);\n"
        "    const fn = new Function(...names, wrapped);\n"
        "    fn(...values);\n";
  js.replace(fallback_pos, fallback_marker.size(), fallback_block);
  if (protected_release) {
    const std::string release_flag = "  const protectedRelease = false;";
    const auto release_flag_pos = js.find(release_flag);
    if (release_flag_pos == std::string::npos) throw std::runtime_error("protected TurboJS module release flag marker missing");
    js.replace(release_flag_pos, release_flag.size(), "  const protectedRelease = true;");

    const std::string full_surface = R"SURFACE(  return Object.freeze({
    kind: 'venom.turbojs.engine.module',
    version: moduleVersion,
    mode,
    fallback,
    wasmUrl,
    createContext,
    destroyContext,
    contextSnapshot,
    status,
    executionSnapshot() { return Object.freeze({ count: executions.length, executions: Object.freeze(executions.slice()), resultRecords: Object.freeze(resultRecords.slice()) }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; return count; },
    fallbackPolicy() { return Object.freeze({ mode: fallback, configured: !!config.fallbackPolicy, strictRelease: !!(config.fallbackPolicy && config.fallbackPolicy.strictRelease) }); },
    abiTable,
    hostImportTable,
    parityProbe,
    bytecodeManifest,
    moduleResolver,
    exceptionAbi,
    hostTrapPolicy,
    executionLifecycle,
    scriptBufferPolicy,
    contextLimitPolicy,
    hostCallDispatch,
    parityContract,
    releaseFailClosed,
    moduleGraph,
    moduleCacheSnapshot,
    resolverAudit,
    interopFallback,
    executionJournal,
    checkpointPolicy,
    replayCursor,
    resumeState,
    determinismAudit,
    snapshotPolicy() { return cachedSnapshotPolicy || Object.freeze({ loaded: false }); },
    snapshotRecord() { return cachedSnapshotRecord || Object.freeze({ loaded: false }); },
    replayValidation() { return cachedReplayValidation || Object.freeze({ loaded: false }); },
    determinismLedger() { return cachedDeterminismLedger || Object.freeze({ loaded: false }); },
    auditSeal() { return cachedAuditSeal || Object.freeze({ loaded: false }); },
    executionCommit() { return cachedExecutionCommit || Object.freeze({ loaded: false }); },
    rollbackPolicy() { return cachedRollbackPolicy || Object.freeze({ loaded: false }); },
    hostCallReceipts() { return cachedHostReceipts || Object.freeze({ loaded: false }); },
    releaseAcceptance() { return cachedReleaseAcceptance || Object.freeze({ loaded: false }); },
    commitAudit() { return cachedCommitAudit || Object.freeze({ loaded: false }); },
    capabilityPolicy() { return cachedCapabilityPolicy || Object.freeze({ loaded: false }); },
    hostIoPolicy() { return cachedHostIoPolicy || Object.freeze({ loaded: false }); },
    permissionSeal() { return cachedPermissionSeal || Object.freeze({ loaded: false }); },
    policyReceipts() { return cachedPolicyReceipts || Object.freeze({ loaded: false }); },
    releaseGate() { return cachedReleaseGate || Object.freeze({ loaded: false }); },
    hostIoDecision() { return cachedHostIoDecision || Object.freeze({ loaded: false }); },
    hostIoDenyTrace() { return cachedHostIoDenyTrace || Object.freeze({ loaded: false }); },
    capabilityLedger() { return cachedCapabilityLedger || Object.freeze({ loaded: false }); },
    policySealAudit() { return cachedPolicySealAudit || Object.freeze({ loaded: false }); },
    runtimeDenylist() { return cachedRuntimeDenylist || Object.freeze({ loaded: false }); },
    executeChunk,
  });
)SURFACE";
    const auto surface_pos = js.rfind(full_surface);
    if (surface_pos == std::string::npos) throw std::runtime_error("protected TurboJS module surface marker missing");
    const std::string minimal_surface = R"SURFACE(  return Object.freeze({
    kind: 'venom.turbojs.engine.module', version: moduleVersion, mode, wasmUrl,
    createContext, destroyContext, contextSnapshot, status,
    executionSnapshot() { return Object.freeze({ count: executions.length }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; return count; },
    executeChunk,
  });
)SURFACE";
    js.replace(surface_pos, full_surface.size(), minimal_surface);
  }
  return js;
}


} // namespace venom::compiler
