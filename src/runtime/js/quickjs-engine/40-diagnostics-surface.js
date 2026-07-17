  function abiTable() {
    return cachedAbiTable || Object.freeze({ abi: config.runtimeAbi && config.runtimeAbi.abi ? config.runtimeAbi.abi : 8, packageVersion: config.runtimeAbi && config.runtimeAbi.packageVersion ? config.runtimeAbi.packageVersion : 32, entryCount: config.runtimeAbi && config.runtimeAbi.entryCount ? config.runtimeAbi.entryCount : 0, tableHash: config.runtimeAbi && config.runtimeAbi.tableHash ? config.runtimeAbi.tableHash : '', table: config.runtimeAbi && config.runtimeAbi.table ? config.runtimeAbi.table : '' });
  }

  function hostImportTable() {
    return cachedHostImportTable || Object.freeze({ importCount: config.hostImports && config.hostImports.importCount ? config.hostImports.importCount : 0, tableHash: config.hostImports && config.hostImports.tableHash ? config.hostImports.tableHash : '', table: config.hostImports && config.hostImports.table ? config.hostImports.table : '', design: config.hostImports && config.hostImports.design ? config.hostImports.design : '' });
  }

  function parityProbe() {
    return cachedParityProbe || Object.freeze({ expected: config.parityProbe && config.parityProbe.expected ? config.parityProbe.expected : 'quickjs:3', native: config.parityProbe && config.parityProbe.native ? config.parityProbe.native : '', wasm: config.parityProbe && config.parityProbe.wasm ? config.parityProbe.wasm : '', matched: false, loaded: false });
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
    kind: 'venom.quickjs.engine.module',
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
