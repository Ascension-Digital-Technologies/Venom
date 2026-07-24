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

