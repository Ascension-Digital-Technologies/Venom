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
