function installVenomHostBridge(root, pkg, routes, assetManifest, runtimePolicy, hostBridgePlan, fetchBridgePlan, asyncQueuePlan, timerBridgePlan, eventQueuePlan, quickJsBridgePlan, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan, quickJsEngineModulePlan = null, quickJsEngineModuleUrl = null, quickJsContextLifecyclePlan = null, hostCapabilitiesPlan = null, quickJsAdapterDiagnosticsPlan = null, quickJsWasmRuntimePlan = null, quickJsWasmUrl = null, quickJsSourceTransferPlan = null, quickJsConsoleBridgePlan = null, quickJsExecutionRecordsPlan = null, quickJsResultBridgePlan = null, quickJsFallbackPolicyPlan = null, quickJsRuntimeAbiPlan = null, quickJsHostImportsPlan = null, quickJsHeapLimitsPlan = null, quickJsScriptBufferPlan = null, quickJsConsoleAbiPlan = null, quickJsParityProbePlan = null, quickJsReleaseFallbackPlan = null, quickJsBytecodeManifestPlan = null, quickJsModuleResolverPlan = null, quickJsExceptionAbiPlan = null, quickJsHostTrapPolicyPlan = null, quickJsExecutionLifecyclePlan = null, quickJsScriptBufferPolicyPlan = null, quickJsContextLimitPolicyPlan = null, quickJsHostCallDispatchPlan = null, quickJsParityContractPlan = null, quickJsReleaseFailClosedPlan = null, quickJsModuleGraphPlan = null, quickJsModuleExecutionPlan = null, quickJsModuleCachePlan = null, quickJsResolverAuditPlan = null, quickJsInteropFallbackPlan = null, quickJsExecutionJournalPlan = null, quickJsCheckpointPolicyPlan = null, quickJsReplayCursorPlan = null, quickJsResumeStatePlan = null, quickJsDeterminismAuditPlan = null, quickJsSnapshotPolicyPlan = null, quickJsSnapshotRecordsPlan = null, quickJsReplayValidationPlan = null, quickJsDeterminismLedgerPlan = null, quickJsAuditSealPlan = null, quickJsExecutionCommitPlan = null, quickJsRollbackPolicyPlan = null, quickJsHostCallReceiptsPlan = null, quickJsReleaseAcceptancePlan = null, quickJsCommitAuditPlan = null, quickJsCapabilityPolicyPlan = null, quickJsHostIoPolicyPlan = null, quickJsPermissionSealPlan = null, quickJsPolicyReceiptsPlan = null, quickJsReleaseGatePlan = null, quickJsHostIoDecisionPlan = null, quickJsHostIoDenyTracePlan = null, quickJsCapabilityLedgerPlan = null, quickJsPolicySealAuditPlan = null, quickJsRuntimeDenylistPlan = null) {
  const asyncQueue = createAsyncHostQueue(fetchBridgePlan, asyncQueuePlan, timerBridgePlan, quickJsBridgePlan);
  const eventQueue = createEventQueue(eventQueuePlan);
  const domHandles = createDomHandleRegistry(root, asyncQueuePlan && asyncQueuePlan.maxDomHandles ? asyncQueuePlan.maxDomHandles : 4096);
  let bridgeRef = null;
  const quickJsEngine = createQuickJsEngine(asyncQueue, quickJsEnginePlan, scriptEnginePolicyPlan, quickJsBridgePlan, quickJsEngineModulePlan, quickJsEngineModuleUrl, quickJsContextLifecyclePlan, hostCapabilitiesPlan, quickJsAdapterDiagnosticsPlan, quickJsWasmRuntimePlan, quickJsWasmUrl, quickJsSourceTransferPlan, quickJsConsoleBridgePlan, quickJsExecutionRecordsPlan, quickJsResultBridgePlan, quickJsFallbackPolicyPlan, quickJsRuntimeAbiPlan, quickJsHostImportsPlan, quickJsHeapLimitsPlan, quickJsScriptBufferPlan, quickJsConsoleAbiPlan, quickJsParityProbePlan, quickJsReleaseFallbackPlan, quickJsBytecodeManifestPlan, quickJsModuleResolverPlan, quickJsExceptionAbiPlan, quickJsHostTrapPolicyPlan, quickJsExecutionLifecyclePlan, quickJsScriptBufferPolicyPlan, quickJsContextLimitPolicyPlan, quickJsHostCallDispatchPlan, quickJsParityContractPlan, quickJsReleaseFailClosedPlan, quickJsModuleGraphPlan, quickJsModuleExecutionPlan, quickJsModuleCachePlan, quickJsResolverAuditPlan, quickJsInteropFallbackPlan, quickJsExecutionJournalPlan, quickJsCheckpointPolicyPlan, quickJsReplayCursorPlan, quickJsResumeStatePlan, quickJsDeterminismAuditPlan, quickJsSnapshotPolicyPlan, quickJsSnapshotRecordsPlan, quickJsReplayValidationPlan, quickJsDeterminismLedgerPlan, quickJsAuditSealPlan, quickJsExecutionCommitPlan, quickJsRollbackPolicyPlan, quickJsHostCallReceiptsPlan, quickJsReleaseAcceptancePlan, quickJsCommitAuditPlan, quickJsCapabilityPolicyPlan, quickJsHostIoPolicyPlan, quickJsPermissionSealPlan, quickJsPolicyReceiptsPlan, quickJsReleaseGatePlan, quickJsHostIoDecisionPlan, quickJsHostIoDenyTracePlan, quickJsCapabilityLedgerPlan, quickJsPolicySealAuditPlan, quickJsRuntimeDenylistPlan);
  const bridge = Object.freeze({
    version: 1,
    packageVersion: pkg.version,
    packageFeatureTableVersion: pkg.features ? pkg.features.version : 0,
    packageFeatureCount: pkg.features ? pkg.features.featureCount : 0,
    packageFeatureRequiredInRelease: pkg.features ? pkg.features.requiredInRelease : 0,
    routeCount: routes.length,
    assetCount: assetManifest.size,
    runtimeHardened: runtimePolicy.runtimeHardened,
    failClosed: runtimePolicy.failClosed,
    hostBridgeVersion: hostBridgePlan ? hostBridgePlan.version : 0,
    hostCallCount: hostBridgePlan && hostBridgePlan.hostCalls ? hostBridgePlan.hostCalls.size : 0,
    eventBindingMode: hostBridgePlan && hostBridgePlan.inlineEventBindings ? 'inline-attribute' : 'none',
    fetchBridgeVersion: fetchBridgePlan ? fetchBridgePlan.version : 0,
    fetchBridgeMode: fetchBridgePlan && fetchBridgePlan.enabled ? 'async-host-call' : 'none',
    asyncHostQueueVersion: asyncQueuePlan ? asyncQueuePlan.version : 0,
    asyncHostQueueMode: asyncQueuePlan && asyncQueuePlan.enabled ? 'enabled' : 'none',
    timerBridgeVersion: timerBridgePlan ? timerBridgePlan.version : 0,
    timerBridgeMode: timerBridgePlan && timerBridgePlan.enabled ? 'async-host-call' : 'none',
    eventQueueVersion: eventQueuePlan ? eventQueuePlan.version : 0,
    eventQueueMode: eventQueuePlan && eventQueuePlan.enabled ? 'enabled' : 'none',
    quickJsBridgeVersion: quickJsBridgePlan ? quickJsBridgePlan.version : 0,
    quickJsBridgeMode: quickJsBridgePlan ? quickJsBridgePlan.mode : 'none',
    scriptIsolationVersion: scriptIsolationPlan ? scriptIsolationPlan.version : 0,
    scriptIsolationMode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'none',
    scriptPolicyVersion: scriptPolicyPlan ? scriptPolicyPlan.version : 0,
    scriptPolicyMode: scriptPolicyPlan ? 'capability-metadata' : 'none',
    quickJsChunkVersion: quickJsChunkPlan ? quickJsChunkPlan.version : 0,
    quickJsChunkMode: quickJsChunkPlan ? quickJsChunkPlan.mode : 'none',
    quickJsChunkCount: quickJsChunkPlan ? quickJsChunkPlan.chunkCount : 0,
    quickJsEngineVersion: quickJsEnginePlan ? quickJsEnginePlan.version : 0,
    quickJsEngineMode: quickJsEnginePlan ? quickJsEnginePlan.mode : 'none',
    quickJsEngineEnabled: quickJsEnginePlan ? !!quickJsEnginePlan.enabled : false,
    scriptEnginePolicyVersion: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.version : 0,
    scriptEngineFallback: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.fallback : 'none',
    quickJsEngineModuleVersion: quickJsEngineModulePlan ? quickJsEngineModulePlan.version : 0,
    quickJsEngineModuleMode: quickJsEngineModulePlan && quickJsEngineModulePlan.enabled ? quickJsEngineModulePlan.executionMode : 'none',
    quickJsEngineModuleLoaded: !!quickJsEngineModuleUrl,
    quickJsEngineModuleUrl: quickJsEngineModuleUrl || '',
    quickJsContextLifecycleVersion: quickJsContextLifecyclePlan ? quickJsContextLifecyclePlan.version : 0,
    quickJsContextLifecycleMode: quickJsContextLifecyclePlan && quickJsContextLifecyclePlan.enabled ? quickJsContextLifecyclePlan.model : 'none',
    hostCapabilitiesVersion: hostCapabilitiesPlan ? hostCapabilitiesPlan.version : 0,
    hostCapabilityCount: hostCapabilitiesPlan && hostCapabilitiesPlan.capabilities ? hostCapabilitiesPlan.capabilities.length : 0,
    quickJsAdapterDiagnosticsVersion: quickJsAdapterDiagnosticsPlan ? quickJsAdapterDiagnosticsPlan.version : 0,
    quickJsAdapterDiagnosticsMode: quickJsAdapterDiagnosticsPlan && quickJsAdapterDiagnosticsPlan.enabled ? 'enabled' : 'none',
    quickJsWasmRuntimeVersion: quickJsWasmRuntimePlan ? quickJsWasmRuntimePlan.version : 0,
    quickJsWasmRuntimeMode: quickJsWasmRuntimePlan && quickJsWasmRuntimePlan.enabled ? quickJsWasmRuntimePlan.executionMode : 'none',
    quickJsWasmUrl: quickJsWasmUrl || '',
    quickJsSourceTransferVersion: quickJsSourceTransferPlan ? quickJsSourceTransferPlan.version : 0,
    quickJsSourceTransferMode: quickJsSourceTransferPlan && quickJsSourceTransferPlan.enabled ? 'enabled' : 'none',
    quickJsConsoleBridgeVersion: quickJsConsoleBridgePlan ? quickJsConsoleBridgePlan.version : 0,
    quickJsConsoleBridgeMode: quickJsConsoleBridgePlan && quickJsConsoleBridgePlan.enabled ? quickJsConsoleBridgePlan.mode : 'none',
    quickJsExecutionRecordsVersion: quickJsExecutionRecordsPlan ? quickJsExecutionRecordsPlan.version : 0,
    quickJsExecutionRecordsMode: quickJsExecutionRecordsPlan && quickJsExecutionRecordsPlan.enabled ? quickJsExecutionRecordsPlan.retention : 'none',
    quickJsResultBridgeVersion: quickJsResultBridgePlan ? quickJsResultBridgePlan.version : 0,
    quickJsResultBridgeMode: quickJsResultBridgePlan && quickJsResultBridgePlan.enabled ? quickJsResultBridgePlan.format : 'none',
    quickJsFallbackPolicyVersion: quickJsFallbackPolicyPlan ? quickJsFallbackPolicyPlan.version : 0,
    quickJsFallbackPolicyMode: quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.enabled ? quickJsFallbackPolicyPlan.mode : 'none',
    quickJsRuntimeAbi: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.abi : 0,
    quickJsRuntimeAbiHash: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.tableHash : '',
    quickJsHostImportCount: quickJsHostImportsPlan ? quickJsHostImportsPlan.importCount : 0,
    quickJsHeapLimit: quickJsHeapLimitsPlan ? quickJsHeapLimitsPlan.heapLimit : 0,
    quickJsScriptBufferCapacity: quickJsScriptBufferPlan ? quickJsScriptBufferPlan.capacity : 0,
    quickJsConsoleAbi: quickJsConsoleAbiPlan ? quickJsConsoleAbiPlan.abi : 0,
    quickJsParityProbeExpected: quickJsParityProbePlan ? quickJsParityProbePlan.expected : '',
    quickJsReleaseFallbackPolicy: quickJsReleaseFallbackPlan ? quickJsReleaseFallbackPlan.policy : '',
    quickJsBytecodeManifestVersion: quickJsBytecodeManifestPlan ? quickJsBytecodeManifestPlan.version : 0,
    quickJsBytecodeManifestMode: quickJsBytecodeManifestPlan && quickJsBytecodeManifestPlan.enabled ? quickJsBytecodeManifestPlan.format : 'none',
    quickJsBytecodeChunkCount: quickJsBytecodeManifestPlan ? quickJsBytecodeManifestPlan.chunkCount : 0,
    quickJsModuleResolverVersion: quickJsModuleResolverPlan ? quickJsModuleResolverPlan.version : 0,
    quickJsModuleResolverMode: quickJsModuleResolverPlan && quickJsModuleResolverPlan.enabled ? quickJsModuleResolverPlan.mode : 'none',
    quickJsExceptionAbi: quickJsExceptionAbiPlan ? quickJsExceptionAbiPlan.abi : 0,
    quickJsExceptionAbiVersion: quickJsExceptionAbiPlan ? quickJsExceptionAbiPlan.version : 0,
    quickJsHostTrapPolicy: quickJsHostTrapPolicyPlan ? quickJsHostTrapPolicyPlan.policy : '',
    quickJsHostTrapPolicyVersion: quickJsHostTrapPolicyPlan ? quickJsHostTrapPolicyPlan.version : 0,
    quickJsExecutionLifecycleVersion: quickJsExecutionLifecyclePlan ? quickJsExecutionLifecyclePlan.version : 0,
    quickJsExecutionLifecycleStates: quickJsExecutionLifecyclePlan ? quickJsExecutionLifecyclePlan.states : '',
    quickJsScriptBufferPolicyVersion: quickJsScriptBufferPolicyPlan ? quickJsScriptBufferPolicyPlan.version : 0,
    quickJsContextLimitPolicyVersion: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.version : 0,
    quickJsHostCallDispatchVersion: quickJsHostCallDispatchPlan ? quickJsHostCallDispatchPlan.version : 0,
    quickJsHostCallDispatchCount: quickJsHostCallDispatchPlan ? quickJsHostCallDispatchPlan.entryCount : 0,
    quickJsParityContractVersion: quickJsParityContractPlan ? quickJsParityContractPlan.version : 0,
    quickJsReleaseFailClosedVersion: quickJsReleaseFailClosedPlan ? quickJsReleaseFailClosedPlan.version : 0,
    quickJsModuleGraphVersion: quickJsModuleGraphPlan ? quickJsModuleGraphPlan.version : 0,
    quickJsModuleGraphCount: quickJsModuleGraphPlan ? quickJsModuleGraphPlan.moduleCount : 0,
    quickJsModuleExecutionVersion: quickJsModuleExecutionPlan ? quickJsModuleExecutionPlan.version : 0,
    quickJsModuleExecutionMode: quickJsModuleExecutionPlan && quickJsModuleExecutionPlan.enabled ? quickJsModuleExecutionPlan.mode : 'none',
    quickJsModuleCacheVersion: quickJsModuleCachePlan ? quickJsModuleCachePlan.version : 0,
    quickJsResolverAuditVersion: quickJsResolverAuditPlan ? quickJsResolverAuditPlan.version : 0,
    quickJsInteropFallbackVersion: quickJsInteropFallbackPlan ? quickJsInteropFallbackPlan.version : 0,
    quickJsExecutionJournalVersion: quickJsExecutionJournalPlan ? quickJsExecutionJournalPlan.version : 0,
    quickJsCheckpointPolicyVersion: quickJsCheckpointPolicyPlan ? quickJsCheckpointPolicyPlan.version : 0,
    quickJsReplayCursorVersion: quickJsReplayCursorPlan ? quickJsReplayCursorPlan.version : 0,
    quickJsResumeStateVersion: quickJsResumeStatePlan ? quickJsResumeStatePlan.version : 0,
    quickJsDeterminismAuditVersion: quickJsDeterminismAuditPlan ? quickJsDeterminismAuditPlan.version : 0,
    quickJsSnapshotPolicyVersion: quickJsSnapshotPolicyPlan ? quickJsSnapshotPolicyPlan.version : 0,
    quickJsSnapshotRecordsVersion: quickJsSnapshotRecordsPlan ? quickJsSnapshotRecordsPlan.version : 0,
    quickJsReplayValidationVersion: quickJsReplayValidationPlan ? quickJsReplayValidationPlan.version : 0,
    quickJsDeterminismLedgerVersion: quickJsDeterminismLedgerPlan ? quickJsDeterminismLedgerPlan.version : 0,
    quickJsAuditSealVersion: quickJsAuditSealPlan ? quickJsAuditSealPlan.version : 0,
    quickJsExecutionCommitVersion: quickJsExecutionCommitPlan ? quickJsExecutionCommitPlan.version : 0,
    quickJsRollbackPolicyVersion: quickJsRollbackPolicyPlan ? quickJsRollbackPolicyPlan.version : 0,
    quickJsHostCallReceiptsVersion: quickJsHostCallReceiptsPlan ? quickJsHostCallReceiptsPlan.version : 0,
    quickJsReleaseAcceptanceVersion: quickJsReleaseAcceptancePlan ? quickJsReleaseAcceptancePlan.version : 0,
    quickJsCommitAuditVersion: quickJsCommitAuditPlan ? quickJsCommitAuditPlan.version : 0,
    quickJsCapabilityPolicyVersion: quickJsCapabilityPolicyPlan ? quickJsCapabilityPolicyPlan.version : 0,
    quickJsHostIoPolicyVersion: quickJsHostIoPolicyPlan ? quickJsHostIoPolicyPlan.version : 0,
    quickJsPermissionSealVersion: quickJsPermissionSealPlan ? quickJsPermissionSealPlan.version : 0,
    quickJsPolicyReceiptsVersion: quickJsPolicyReceiptsPlan ? quickJsPolicyReceiptsPlan.version : 0,
    quickJsReleaseGateVersion: quickJsReleaseGatePlan ? quickJsReleaseGatePlan.version : 0,
    quickJsHostIoDecisionVersion: quickJsHostIoDecisionPlan ? quickJsHostIoDecisionPlan.version : 0,
    quickJsHostIoDenyTraceVersion: quickJsHostIoDenyTracePlan ? quickJsHostIoDenyTracePlan.version : 0,
    quickJsCapabilityLedgerVersion: quickJsCapabilityLedgerPlan ? quickJsCapabilityLedgerPlan.version : 0,
    quickJsPolicySealAuditVersion: quickJsPolicySealAuditPlan ? quickJsPolicySealAuditPlan.version : 0,
    quickJsRuntimeDenylistVersion: quickJsRuntimeDenylistPlan ? quickJsRuntimeDenylistPlan.version : 0,
    quickJsExecutionJournal() { return quickJsEngine.executionJournal ? quickJsEngine.executionJournal() : Object.freeze({ plan: quickJsExecutionJournalPlan || null, records: [] }); },
    quickJsCheckpointPolicy() { return quickJsEngine.checkpointPolicy ? quickJsEngine.checkpointPolicy() : Object.freeze({ plan: quickJsCheckpointPolicyPlan || null }); },
    quickJsReplayCursor() { return quickJsEngine.replayCursor ? quickJsEngine.replayCursor() : Object.freeze({ plan: quickJsReplayCursorPlan || null, sequence: 0 }); },
    quickJsResumeState() { return quickJsEngine.resumeState ? quickJsEngine.resumeState() : Object.freeze({ plan: quickJsResumeStatePlan || null }); },
    quickJsDeterminismAudit() { return quickJsEngine.determinismAudit ? quickJsEngine.determinismAudit() : Object.freeze({ plan: quickJsDeterminismAuditPlan || null }); },
    quickJsSnapshotPolicy() { return quickJsEngine.snapshotPolicy ? quickJsEngine.snapshotPolicy() : Object.freeze({ plan: quickJsSnapshotPolicyPlan || null }); },
    quickJsSnapshotRecord() { return quickJsEngine.snapshotRecord ? quickJsEngine.snapshotRecord() : Object.freeze({ plan: quickJsSnapshotRecordsPlan || null }); },
    quickJsReplayValidation() { return quickJsEngine.replayValidation ? quickJsEngine.replayValidation() : Object.freeze({ plan: quickJsReplayValidationPlan || null }); },
    quickJsDeterminismLedger() { return quickJsEngine.determinismLedger ? quickJsEngine.determinismLedger() : Object.freeze({ plan: quickJsDeterminismLedgerPlan || null }); },
    quickJsAuditSeal() { return quickJsEngine.auditSeal ? quickJsEngine.auditSeal() : Object.freeze({ plan: quickJsAuditSealPlan || null }); },
    quickJsExecutionCommit() { return quickJsEngine.executionCommit ? quickJsEngine.executionCommit() : Object.freeze({ plan: quickJsExecutionCommitPlan || null }); },
    quickJsRollbackPolicy() { return quickJsEngine.rollbackPolicy ? quickJsEngine.rollbackPolicy() : Object.freeze({ plan: quickJsRollbackPolicyPlan || null }); },
    quickJsHostCallReceipts() { return quickJsEngine.hostCallReceipts ? quickJsEngine.hostCallReceipts() : Object.freeze({ plan: quickJsHostCallReceiptsPlan || null }); },
    quickJsReleaseAcceptance() { return quickJsEngine.releaseAcceptance ? quickJsEngine.releaseAcceptance() : Object.freeze({ plan: quickJsReleaseAcceptancePlan || null }); },
    quickJsCommitAudit() { return quickJsEngine.commitAudit ? quickJsEngine.commitAudit() : Object.freeze({ plan: quickJsCommitAuditPlan || null }); },
    quickJsCapabilityPolicy() { return quickJsEngine.capabilityPolicy ? quickJsEngine.capabilityPolicy() : Object.freeze({ plan: quickJsCapabilityPolicyPlan || null }); },
    quickJsHostIoPolicy() { return quickJsEngine.hostIoPolicy ? quickJsEngine.hostIoPolicy() : Object.freeze({ plan: quickJsHostIoPolicyPlan || null }); },
    quickJsPermissionSeal() { return quickJsEngine.permissionSeal ? quickJsEngine.permissionSeal() : Object.freeze({ plan: quickJsPermissionSealPlan || null }); },
    quickJsPolicyReceipts() { return quickJsEngine.policyReceipts ? quickJsEngine.policyReceipts() : Object.freeze({ plan: quickJsPolicyReceiptsPlan || null }); },
    quickJsReleaseGate() { return quickJsEngine.releaseGate ? quickJsEngine.releaseGate() : Object.freeze({ plan: quickJsReleaseGatePlan || null }); },
    quickJsHostIoDecision() { return quickJsEngine.hostIoDecision ? quickJsEngine.hostIoDecision() : Object.freeze({ plan: quickJsHostIoDecisionPlan || null }); },
    quickJsHostIoDenyTrace() { return quickJsEngine.hostIoDenyTrace ? quickJsEngine.hostIoDenyTrace() : Object.freeze({ plan: quickJsHostIoDenyTracePlan || null }); },
    quickJsCapabilityLedger() { return quickJsEngine.capabilityLedger ? quickJsEngine.capabilityLedger() : Object.freeze({ plan: quickJsCapabilityLedgerPlan || null }); },
    quickJsPolicySealAudit() { return quickJsEngine.policySealAudit ? quickJsEngine.policySealAudit() : Object.freeze({ plan: quickJsPolicySealAuditPlan || null }); },
    quickJsRuntimeDenylist() { return quickJsEngine.runtimeDenylist ? quickJsEngine.runtimeDenylist() : Object.freeze({ plan: quickJsRuntimeDenylistPlan || null }); },
    asyncHostQueueSnapshot() {
      return asyncQueue.snapshot();
    },
    currentRouteGeneration() { return asyncQueue.snapshot().generation >>> 0; },
    isRouteGenerationActive(generation) { return (generation >>> 0) === (asyncQueue.snapshot().generation >>> 0); },
    activateRouteGeneration(generation) {
      const active = asyncQueue.setRouteGeneration(generation, 'route-activation');
      domHandles.nextGeneration();
      return active;
    },
    enqueueHostCall(kind, payload) {
      return asyncQueue.enqueue(kind, payload);
    },
    settleHostCall(id, state, value) {
      return asyncQueue.settle(id, state, value);
    },
    requestFetch(input, init) {
      return asyncQueue.requestFetch(input, init);
    },
    fetch(input, init) {
      return asyncQueue.requestFetch(input, init);
    },
    scheduleTimer(callback, delay, repeat) {
      return asyncQueue.scheduleTimer(callback, delay, repeat);
    },
    cancelTimer(id) {
      return asyncQueue.cancelTimer(id);
    },
    eventQueueSnapshot() {
      return eventQueue.snapshot();
    },
    callQuickJs(entry, payload) {
      return asyncQueue.callQuickJs(entry, payload);
    },
    createQuickJsContext(route, source, order) {
      return quickJsEngine.createContext(route, source, order);
    },
    executeQuickJsChunk(chunk, route) {
      return quickJsEngine.executeChunk(chunk, route, bridgeRef || this);
    },
    destroyQuickJsContext(route, source, order) {
      return quickJsEngine.destroyContext(route, source, order);
    },
    quickJsContextSnapshot() {
      return quickJsEngine.contextSnapshot();
    },
    quickJsEngineModuleStatus() {
      return quickJsEngine.moduleStatus();
    },
    quickJsExecutionSnapshot() {
      return quickJsEngine.executionSnapshot();
    },
    quickJsConsoleEvents() {
      return quickJsEngine.consoleEvents();
    },
    clearQuickJsConsoleEvents() {
      return quickJsEngine.clearConsoleEvents();
    },
    quickJsFallbackPolicy() {
      return quickJsEngine.fallbackPolicy();
    },
    quickJsAbiTable() {
      return quickJsEngine.abiTable();
    },
    quickJsHostImportTable() {
      return quickJsEngine.hostImportTable();
    },
    quickJsParityProbe() {
      return quickJsEngine.parityProbe();
    },
    quickJsBytecodeManifest() {
      return quickJsEngine.bytecodeManifest();
    },
    quickJsModuleResolver() {
      return quickJsEngine.moduleResolver();
    },
    quickJsExceptionAbi() {
      return quickJsEngine.exceptionAbi();
    },
    quickJsHostTrapPolicy() {
      return quickJsEngine.hostTrapPolicy();
    },
    quickJsExecutionLifecycle() {
      return quickJsEngine.executionLifecycle();
    },
    quickJsScriptBufferPolicy() {
      return quickJsEngine.scriptBufferPolicy();
    },
    quickJsContextLimitPolicy() {
      return quickJsEngine.contextLimitPolicy();
    },
    quickJsHostCallDispatch() {
      return quickJsEngine.hostCallDispatch();
    },
    quickJsParityContract() {
      return quickJsEngine.parityContract();
    },
    quickJsReleaseFailClosed() {
      return quickJsEngine.releaseFailClosed();
    },
    quickJsModuleGraph() {
      return quickJsEngine.moduleGraph();
    },
    quickJsModuleCacheSnapshot() {
      return quickJsEngine.moduleCacheSnapshot();
    },
    quickJsResolverAudit() {
      return quickJsEngine.resolverAudit();
    },
    quickJsInteropFallback() {
      return quickJsEngine.interopFallback();
    },
    createScriptScope(route, source, order) {
      const record = asyncQueue.enqueue('script.scopeCreate', { route: String(route || ''), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(record.id, 'fulfilled', { isolated: true });
      return Object.freeze({ route: String(route || ''), source: String(source || ''), order: order >>> 0, mode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'route-scoped' });
    },
    checkScriptPolicy(chunk) {
      const record = asyncQueue.enqueue(scriptPolicyPlan && scriptPolicyPlan.policyCheckHostCall ? scriptPolicyPlan.policyCheckHostCall : 'script.policyCheck', { source: chunk && chunk.source ? String(chunk.source) : '', route: chunk && chunk.route ? String(chunk.route) : '' });
      asyncQueue.settle(record.id, 'fulfilled', { allowed: true });
      return Object.freeze({ allowed: true, capabilities: scriptPolicyPlan ? scriptPolicyPlan.defaultCapabilities : '' });
    },
    domRootHandle() { return domHandles.rootHandle(); },
    domHandleSnapshot() { return domHandles.snapshot(); },
    registerDomHandle(node) { return domHandles.register(node); },
    resolveDomHandle(handle) { return domHandles.resolve(handle); },
    revokeDomHandle(handle) { return domHandles.revoke(handle); },
    advanceDomGeneration() { return domHandles.nextGeneration(); },
    teardownRoute(reason = 'route-teardown') {
      const route = globalThis.location && globalThis.location.pathname ? normalizeRoute(globalThis.location.pathname) : '/';
      const contexts = quickJsEngine.destroyRoute(route);
      const events = eventQueue.clear();
      asyncQueue.resetRoute(reason);
      return Object.freeze({ route, contexts: contexts.destroyed, events, reason: String(reason) });
    },
    domSetAttribute(handle, name, value) {
      authorizeHostCapability(hostBridgePlan, 2, String(name || '').length + String(value || '').length, 0);
      const node = domHandles.resolve(handle);
      if (!(node instanceof Element)) throw new Error('DOM attribute target denied');
      if (isUnsafeDomAttribute(name, value)) throw new Error('unsafe DOM attribute denied');
      node.setAttribute(String(name), String(value));
      return true;
    },
    domAppendChild(parentHandle, childHandle) {
      authorizeHostCapability(hostBridgePlan, 3, 8, 0);
      const parent = domHandles.resolve(parentHandle);
      const child = domHandles.resolve(childHandle);
      if (!parent || typeof parent.appendChild !== 'function') throw new Error('DOM append target denied');
      parent.appendChild(child);
      return true;
    },
    dispatchEventBinding(record) {
      const generation = record && record.routeGeneration !== undefined ? record.routeGeneration >>> 0 : asyncQueue.snapshot().generation >>> 0;
      if (generation !== (asyncQueue.snapshot().generation >>> 0)) throw new Error('stale route generation rejected for event dispatch');
      const queued = eventQueue.push(record || {});
      const asyncRecord = asyncQueue.enqueue(eventQueuePlan && eventQueuePlan.dispatchHostCall ? eventQueuePlan.dispatchHostCall : 'event.dispatch', {
        eventName: record && record.eventName ? String(record.eventName) : '',
        attrName: record && record.attrName ? String(record.attrName) : '',
      });
      asyncQueue.settle(asyncRecord.id, 'fulfilled', { handled: true });
      return Object.freeze({
        eventName: record && record.eventName ? String(record.eventName) : '',
        attrName: record && record.attrName ? String(record.attrName) : '',
        queued: queued.queued,
        handled: true,
      });
    },
    root,
  });
  bridgeRef = bridge;
  Object.defineProperty(globalThis, '__venomRuntime', {
    value: bridge,
    configurable: false,
    enumerable: false,
    writable: false,
  });
  return bridge;
}

function safeSourceUrl(source) {
  return String(source || 'chunk').replace(/[^a-zA-Z0-9_./#:-]/g, '_');
}

function injectRemoteScript(chunk) {
  return new Promise((resolve, reject) => {
    const source = String(chunk && chunk.source ? chunk.source : '');
    if (/^(?:https?:)?\/\//i.test(source)) {
      reject(new Error('unvendored network script denied by production runtime: ' + source));
      return;
    }
    if (!document.createElement) {
      reject(new Error('document.createElement is required for remote script chunks'));
      return;
    }
    const script = document.createElement('script');
    if ((chunk.flags & SCRIPT_FLAG.MODULE) !== 0) script.type = 'module';
    if ((chunk.flags & SCRIPT_FLAG.ASYNC) !== 0) script.async = true;
    if ((chunk.flags & SCRIPT_FLAG.DEFER) !== 0) script.defer = true;
    script.src = chunk.source;
    script.onload = () => resolve(chunk);
    script.onerror = () => reject(new Error('failed to load remote script: ' + chunk.source));
    const target = document.head || document.body || document.documentElement;
    if (!target || !target.appendChild) {
      reject(new Error('no document insertion target for remote script'));
      return;
    }
    target.appendChild(script);
  });
}

function removeInjectedScript(script) {
  if (!script) return;
  if (typeof script.remove === 'function') { script.remove(); return; }
  const parent = script.parentNode;
  if (parent && typeof parent.removeChild === 'function') parent.removeChild(script);
}

function normalizeBrowserModulePath(value) {
  const parts = [];
  for (const part of String(value || '').replace(/\\/g, '/').split('/')) {
    if (!part || part === '.') continue;
    if (part === '..') { if (parts.length) parts.pop(); continue; }
    parts.push(part);
  }
  return parts.join('/');
}

function browserModuleDirectory(value) {
  const normalized = normalizeBrowserModulePath(value);
  const slash = normalized.lastIndexOf('/');
  return slash < 0 ? '' : normalized.slice(0, slash + 1);
}


function parseBrowserModuleIdentity(source) {
  const match = String(source || '').match(/\/\*@venom-module-source-v1(?:\r?\n)([0-9a-f]+)\r?\n\*\/(?:\r?\n)?/i);
  if (!match || (match[1].length & 1) !== 0) return '';
  const bytes = new Uint8Array(match[1].length / 2);
  for (let i = 0; i < bytes.length; ++i) bytes[i] = Number.parseInt(match[1].slice(i * 2, i * 2 + 2), 16);
  return normalizeBrowserModulePath(textDecoder.decode(bytes));
}

function parseBrowserModuleMap(source) {
  const match = String(source || '').match(/\/\*@venom-module-map-v1(?:\r?\n)([\s\S]*?)\*\/(?:\r?\n)?/);
  const map = new Map();
  if (!match) return map;
  const decodeHex = (value) => {
    if (!value || (value.length & 1) !== 0 || !/^[0-9a-f]+$/i.test(value)) return '';
    const bytes = new Uint8Array(value.length / 2);
    for (let i = 0; i < bytes.length; ++i) bytes[i] = Number.parseInt(value.slice(i * 2, i * 2 + 2), 16);
    return textDecoder.decode(bytes);
  };
  for (const line of match[1].split('\n')) {
    if (!line) continue;
    const fields = line.split('\t');
    if (fields.length !== 2) continue;
    const specifier = decodeHex(fields[0]);
    const target = decodeHex(fields[1]);
    if (specifier && target) map.set(specifier, normalizeBrowserModulePath(target));
  }
  return map;
}

function createBrowserModuleLinker(chunks, routeName) {
  const modules = new Map();
  const importMaps = new Map();
  const urls = new Map();
  const state = new Map();
  const authoredAliases = new Map();
  const ambiguousAliases = new Set();
  const route = String(routeName || '');
  for (const chunk of chunks || []) {
    if (!chunk || (chunk.flags & SCRIPT_FLAG.BROWSER) === 0 || (chunk.flags & SCRIPT_FLAG.MODULE) === 0) continue;
    const routeMatches = chunk.route === route || chunk.route === '*' || chunk.route === '';
    const dependencyChunk = (chunk.flags & SCRIPT_FLAG.DEPENDENCY) !== 0;
    if (!routeMatches && !dependencyChunk) continue;
    const normalizedSource = normalizeBrowserModulePath(chunk.source);
    modules.set(normalizedSource, chunk);
    importMaps.set(normalizedSource, parseBrowserModuleMap(chunk.code));
    const authoredSource = parseBrowserModuleIdentity(chunk.code);
    if (authoredSource) {
      const prior = authoredAliases.get(authoredSource);
      if (prior && prior !== normalizedSource) {
        authoredAliases.delete(authoredSource);
        ambiguousAliases.add(authoredSource);
      } else if (!ambiguousAliases.has(authoredSource)) {
        authoredAliases.set(authoredSource, normalizedSource);
      }
    }
  }

  const resolve = (referrer, specifier) => {
    if (!specifier || specifier[0] !== '.') return '';
    const normalizedReferrer = normalizeBrowserModulePath(referrer);
    const mapped = importMaps.get(normalizedReferrer);
    const opaqueTarget = mapped && mapped.get(specifier);
    if (opaqueTarget && modules.has(opaqueTarget)) return opaqueTarget;

    // Production packaging can replace source labels at more than one stage. If
    // the compiler-authored map is present but attached to a different opaque
    // alias, accept it only when the specifier resolves uniquely. This remains
    // fail-closed for ambiguous graphs.
    let uniqueMappedTarget = '';
    for (const candidateMap of importMaps.values()) {
      const candidate = candidateMap && candidateMap.get(specifier);
      if (!candidate || !modules.has(candidate)) continue;
      if (uniqueMappedTarget && uniqueMappedTarget !== candidate) {
        uniqueMappedTarget = '';
        break;
      }
      uniqueMappedTarget = candidate;
    }
    if (uniqueMappedTarget) return uniqueMappedTarget;
    const base = normalizeBrowserModulePath(browserModuleDirectory(referrer) + specifier);
    const candidates = [base];
    if (!/\.[A-Za-z0-9]+$/.test(base)) {
      for (const ext of ['.js','.mjs','.cjs','.jsx','.ts','.tsx','.mts','.cts']) candidates.push(base + ext);
      for (const ext of ['.js','.mjs','.jsx','.ts','.tsx','.mts']) candidates.push(base + '/index' + ext);
    } else if (base.endsWith('.js')) {
      const stem = base.slice(0, -3);
      candidates.push(stem + '.ts', stem + '.tsx', stem + '.mts');
    }
    for (const candidate of candidates) {
      if (modules.has(candidate)) return candidate;
      const aliased = authoredAliases.get(candidate);
      if (aliased && modules.has(aliased)) return aliased;
    }
    return '';
  };

  const rewrite = (source, referrer, buildUrl) => {
    const patterns = [
      /((?:^|[;\n\r])\s*import\s+(?:[^'";]+?\s+from\s*)?)(['"])([^'"]+)\2/g,
      /(\bexport\s+(?:\*|\{[^}]*\})\s+from\s*)(['"])([^'"]+)\2/g
    ];
    let output = source;
    for (const pattern of patterns) {
      output = output.replace(pattern, (whole, prefix, quote, specifier) => {
        if (!String(specifier).startsWith('.')) return whole;
        const dependency = resolve(referrer, specifier);
        if (!dependency) throw new Error('browser module dependency not found: ' + referrer + ' -> ' + specifier);
        return prefix + quote + buildUrl(dependency) + quote;
      });
    }
    return output;
  };

  const buildUrl = (sourceName) => {
    const normalized = normalizeBrowserModulePath(sourceName);
    if (urls.has(normalized)) return urls.get(normalized);
    if (state.get(normalized) === 1) throw new Error('browser module cycle requires bundling support: ' + normalized);
    const chunk = modules.get(normalized);
    if (!chunk) throw new Error('browser module is unavailable: ' + normalized);
    state.set(normalized, 1);
    const linked = rewrite(String(chunk.code || ''), normalized, buildUrl) + '\n//# sourceURL=venom-browser://' + safeSourceUrl(normalized);
    const url = URL.createObjectURL(new Blob([linked], { type: 'text/javascript' }));
    urls.set(normalized, url);
    state.set(normalized, 2);
    return url;
  };

  return {
    urlFor(chunk) { return buildUrl(normalizeBrowserModulePath(chunk && chunk.source)); },
    dispose() { for (const url of urls.values()) URL.revokeObjectURL(url); urls.clear(); }
  };
}

async function executeBrowserScriptChunk(chunk, moduleLinker = null) {
  if (chunk && chunk.bytecodeBytes && chunk.bytecodeBytes.length) {
    throw new Error('browser script chunk was packaged as QuickJS bytecode: ' + (chunk.source || '<script>'));
  }
  if (!chunk || !chunk.code || !String(chunk.code).trim()) return Object.freeze({ ...chunk, executed: false, empty: true, browser: true });
  if (typeof document === 'undefined' || !document.createElement) throw new Error('browser script execution requires document.createElement');
  const script = document.createElement('script');
  const isModule = (chunk.flags & SCRIPT_FLAG.MODULE) !== 0;
  if (isModule) script.type = 'module';
  script.async = false;
  const source = String(chunk.code) + '\n//# sourceURL=venom-browser://' + safeSourceUrl(chunk.source);
  const target = document.head || document.body || document.documentElement;
  if (!target || !target.appendChild) throw new Error('no document insertion target for browser script');
  if (isModule) {
    const linkedUrl = moduleLinker ? moduleLinker.urlFor(chunk) : '';
    const ownedUrl = !linkedUrl;
    const url = linkedUrl || URL.createObjectURL(new Blob([source], { type: 'text/javascript' }));
    try { await new Promise((resolve, reject) => { script.src=url; script.onload=resolve; script.onerror=()=>reject(new Error('browser module execution failed: '+chunk.source)); target.appendChild(script); }); }
    finally { if (ownedUrl) URL.revokeObjectURL(url); removeInjectedScript(script); }
  } else { script.textContent = source; target.appendChild(script); removeInjectedScript(script); }
  return Object.freeze({ ...chunk, executed: true, browser: true });
}

async function executeScriptChunk(chunk, route, scriptIsolationPlan = null, scriptPolicyPlan = null, quickJsChunkPlan = null, quickJsEnginePlan = null, scriptEnginePolicyPlan = null, browserModuleLinker = null) {
  const bridge = globalThis.__venomRuntime || null;
  if (bridge && typeof bridge.checkScriptPolicy === 'function') bridge.checkScriptPolicy(chunk);
  if (bridge && typeof bridge.createScriptScope === 'function') bridge.createScriptScope(route && route.route ? route.route : chunk.route, chunk.source, chunk.order);
  const startRecord = bridge && typeof bridge.enqueueHostCall === 'function'
    ? bridge.enqueueHostCall(scriptPolicyPlan && scriptPolicyPlan.chunkStartHostCall ? scriptPolicyPlan.chunkStartHostCall : 'script.chunkStart', { route: chunk.route, source: chunk.source, order: chunk.order })
    : null;
  if (bridge && typeof bridge.callQuickJs === 'function') {
    const qjsBoundaryRecord = bridge.callQuickJs('chunk:' + chunk.order, { route: chunk.route, source: chunk.source, bytes: chunk.code ? chunk.code.length : 0, boundary: quickJsChunkPlan ? quickJsChunkPlan.mode : 'engine-input', engine: quickJsEnginePlan ? quickJsEnginePlan.mode : 'bootstrap' });
    if (bridge && typeof bridge.settleHostCall === 'function' && qjsBoundaryRecord && qjsBoundaryRecord.id) {
      bridge.settleHostCall(qjsBoundaryRecord.id, 'fulfilled', { boundary: quickJsChunkPlan ? quickJsChunkPlan.mode : 'engine-input', engine: quickJsEnginePlan ? quickJsEnginePlan.mode : 'bootstrap' });
    }
  }
  let executed;
  if ((chunk.flags & SCRIPT_FLAG.BROWSER) !== 0) executed = await executeBrowserScriptChunk(chunk, browserModuleLinker);
  else if (bridge && typeof bridge.executeQuickJsChunk === 'function') {
    executed = await bridge.executeQuickJsChunk(chunk, route);
  } else {
    const denyHostFallback = !!(scriptEnginePolicyPlan && String(scriptEnginePolicyPlan.fallback || '').includes('deny-host-js'));
    if (denyHostFallback) throw new Error('QuickJS/WASM backend unavailable; protected host JS fallback denied');
    const fallbackRecord = bridge && typeof bridge.enqueueHostCall === 'function'
      ? bridge.enqueueHostCall(scriptEnginePolicyPlan && scriptEnginePolicyPlan.fallback ? 'script.engineFallback' : 'script.engineFallback', { route: chunk.route, source: chunk.source, order: chunk.order })
      : null;
    if ((chunk.flags & SCRIPT_FLAG.REMOTE_URL) !== 0) {
      executed = await injectRemoteScript(chunk);
    } else if ((!chunk.code || !String(chunk.code).trim()) && !(chunk.bytecodeBytes && chunk.bytecodeBytes.length)) {
      executed = Object.freeze({ ...chunk, executed: false, empty: true });
    } else {
__VENOM_RUNTIME_LEGACY_FALLBACK_BLOCK__
    }
    if (bridge && fallbackRecord) bridge.settleHostCall(fallbackRecord.id, 'fulfilled', { executed: !!executed.executed });
  }
  if (bridge && startRecord) bridge.settleHostCall(startRecord.id, 'fulfilled', { executed: !!(executed && executed.executed), isolated: !!scriptIsolationPlan, engine: quickJsEnginePlan ? quickJsEnginePlan.mode : 'none' });
  return Object.freeze({ ...executed, isolated: !!scriptIsolationPlan, isolationMode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'none', quickJsBoundary: quickJsChunkPlan ? quickJsChunkPlan.mode : 'none', quickJsEngine: quickJsEnginePlan ? quickJsEnginePlan.mode : 'none' });
}

async function executeScriptsForRoute(route, jsBundle, scriptIsolationPlan = null, scriptPolicyPlan = null, quickJsChunkPlan = null, quickJsEnginePlan = null, scriptEnginePolicyPlan = null) {
  assertRuntimeIntegrity(activeRuntimeOpcodeMap);
  if (!jsBundle || !Array.isArray(jsBundle.chunks)) return [];
  const selected = jsBundle.chunks.filter((chunk) => ((chunk.route === route.route || chunk.route === '*' || chunk.route === '') && (chunk.flags & SCRIPT_FLAG.DEPENDENCY) === 0));
  const executed = [];

  // Venom reconstructs route DOM after the browser's native DOMContentLoaded/load
  // lifecycle may already have completed. Browser-side application scripts still
  // expect those startup hooks. Capture late registrations during route script
  // execution and replay them once, in document order, after all route scripts load.
  const lateLifecycleHandlers = [];
  const originalDocumentAdd = document.addEventListener;
  const originalWindowAdd = globalThis.addEventListener;
  document.addEventListener = function(type, listener, options) {
    if (type === 'DOMContentLoaded' && document.readyState !== 'loading' && typeof listener === 'function') {
      lateLifecycleHandlers.push({ target: document, type, listener, options });
      return;
    }
    return originalDocumentAdd.call(this, type, listener, options);
  };
  globalThis.addEventListener = function(type, listener, options) {
    if (type === 'load' && document.readyState === 'complete' && typeof listener === 'function') {
      lateLifecycleHandlers.push({ target: globalThis, type, listener, options });
      return;
    }
    return originalWindowAdd.call(this, type, listener, options);
  };

  const browserModuleLinker = createBrowserModuleLinker(jsBundle.chunks, route.route);
  try {
    for (const chunk of selected) {
      // Keep script execution deterministic: execute in document order.
      executed.push(await executeScriptChunk(chunk, route, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan, browserModuleLinker));
    }
  } finally {
    browserModuleLinker.dispose();
    document.addEventListener = originalDocumentAdd;
    globalThis.addEventListener = originalWindowAdd;
  }

  for (const record of lateLifecycleHandlers) {
    const event = new Event(record.type);
    try {
      record.listener.call(record.target, event);
    } catch (error) {
      console.error('[venom] route lifecycle handler failed', error);
    }
  }
  return executed;
}

__VENOM_BROWSER_ASSET_RUNTIME__

