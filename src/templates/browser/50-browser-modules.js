function installVenomHostBridge(root, pkg, routes, assetManifest, runtimePolicy, hostBridgePlan, fetchBridgePlan, asyncQueuePlan, timerBridgePlan, eventQueuePlan, turboJsBridgePlan, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsEngineModulePlan = null, turboJsEngineModuleUrl = null, turboJsContextLifecyclePlan = null, hostCapabilitiesPlan = null, turboJsAdapterDiagnosticsPlan = null, turboJsWasmRuntimePlan = null, turboJsWasmUrl = null, turboJsSourceTransferPlan = null, turboJsConsoleBridgePlan = null, turboJsExecutionRecordsPlan = null, turboJsResultBridgePlan = null, turboJsFallbackPolicyPlan = null, turboJsRuntimeAbiPlan = null, turboJsHostImportsPlan = null, turboJsHeapLimitsPlan = null, turboJsScriptBufferPlan = null, turboJsConsoleAbiPlan = null, turboJsParityProbePlan = null, turboJsReleaseFallbackPlan = null, turboJsBytecodeManifestPlan = null, turboJsModuleResolverPlan = null, turboJsExceptionAbiPlan = null, turboJsHostTrapPolicyPlan = null, turboJsExecutionLifecyclePlan = null, turboJsScriptBufferPolicyPlan = null, turboJsContextLimitPolicyPlan = null, turboJsHostCallDispatchPlan = null, turboJsParityContractPlan = null, turboJsReleaseFailClosedPlan = null, turboJsModuleGraphPlan = null, turboJsModuleExecutionPlan = null, turboJsModuleCachePlan = null, turboJsResolverAuditPlan = null, turboJsInteropFallbackPlan = null, turboJsExecutionJournalPlan = null, turboJsCheckpointPolicyPlan = null, turboJsReplayCursorPlan = null, turboJsResumeStatePlan = null, turboJsDeterminismAuditPlan = null, turboJsSnapshotPolicyPlan = null, turboJsSnapshotRecordsPlan = null, turboJsReplayValidationPlan = null, turboJsDeterminismLedgerPlan = null, turboJsAuditSealPlan = null, turboJsExecutionCommitPlan = null, turboJsRollbackPolicyPlan = null, turboJsHostCallReceiptsPlan = null, turboJsReleaseAcceptancePlan = null, turboJsCommitAuditPlan = null, turboJsCapabilityPolicyPlan = null, turboJsHostIoPolicyPlan = null, turboJsPermissionSealPlan = null, turboJsPolicyReceiptsPlan = null, turboJsReleaseGatePlan = null, turboJsHostIoDecisionPlan = null, turboJsHostIoDenyTracePlan = null, turboJsCapabilityLedgerPlan = null, turboJsPolicySealAuditPlan = null, turboJsRuntimeDenylistPlan = null) {
  const asyncQueue = createAsyncHostQueue(fetchBridgePlan, asyncQueuePlan, timerBridgePlan, turboJsBridgePlan);
  const eventQueue = createEventQueue(eventQueuePlan);
  const domHandles = createDomHandleRegistry(root, asyncQueuePlan && asyncQueuePlan.maxDomHandles ? asyncQueuePlan.maxDomHandles : 4096);
  let bridgeRef = null;
  const turboJsEngine = createTurboJsEngine(asyncQueue, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsBridgePlan, turboJsEngineModulePlan, turboJsEngineModuleUrl, turboJsContextLifecyclePlan, hostCapabilitiesPlan, turboJsAdapterDiagnosticsPlan, turboJsWasmRuntimePlan, turboJsWasmUrl, turboJsSourceTransferPlan, turboJsConsoleBridgePlan, turboJsExecutionRecordsPlan, turboJsResultBridgePlan, turboJsFallbackPolicyPlan, turboJsRuntimeAbiPlan, turboJsHostImportsPlan, turboJsHeapLimitsPlan, turboJsScriptBufferPlan, turboJsConsoleAbiPlan, turboJsParityProbePlan, turboJsReleaseFallbackPlan, turboJsBytecodeManifestPlan, turboJsModuleResolverPlan, turboJsExceptionAbiPlan, turboJsHostTrapPolicyPlan, turboJsExecutionLifecyclePlan, turboJsScriptBufferPolicyPlan, turboJsContextLimitPolicyPlan, turboJsHostCallDispatchPlan, turboJsParityContractPlan, turboJsReleaseFailClosedPlan, turboJsModuleGraphPlan, turboJsModuleExecutionPlan, turboJsModuleCachePlan, turboJsResolverAuditPlan, turboJsInteropFallbackPlan, turboJsExecutionJournalPlan, turboJsCheckpointPolicyPlan, turboJsReplayCursorPlan, turboJsResumeStatePlan, turboJsDeterminismAuditPlan, turboJsSnapshotPolicyPlan, turboJsSnapshotRecordsPlan, turboJsReplayValidationPlan, turboJsDeterminismLedgerPlan, turboJsAuditSealPlan, turboJsExecutionCommitPlan, turboJsRollbackPolicyPlan, turboJsHostCallReceiptsPlan, turboJsReleaseAcceptancePlan, turboJsCommitAuditPlan, turboJsCapabilityPolicyPlan, turboJsHostIoPolicyPlan, turboJsPermissionSealPlan, turboJsPolicyReceiptsPlan, turboJsReleaseGatePlan, turboJsHostIoDecisionPlan, turboJsHostIoDenyTracePlan, turboJsCapabilityLedgerPlan, turboJsPolicySealAuditPlan, turboJsRuntimeDenylistPlan);
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
    turboJsBridgeVersion: turboJsBridgePlan ? turboJsBridgePlan.version : 0,
    turboJsBridgeMode: turboJsBridgePlan ? turboJsBridgePlan.mode : 'none',
    scriptIsolationVersion: scriptIsolationPlan ? scriptIsolationPlan.version : 0,
    scriptIsolationMode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'none',
    scriptPolicyVersion: scriptPolicyPlan ? scriptPolicyPlan.version : 0,
    scriptPolicyMode: scriptPolicyPlan ? 'capability-metadata' : 'none',
    turboJsChunkVersion: turboJsChunkPlan ? turboJsChunkPlan.version : 0,
    turboJsChunkMode: turboJsChunkPlan ? turboJsChunkPlan.mode : 'none',
    turboJsChunkCount: turboJsChunkPlan ? turboJsChunkPlan.chunkCount : 0,
    turboJsEngineVersion: turboJsEnginePlan ? turboJsEnginePlan.version : 0,
    turboJsEngineMode: turboJsEnginePlan ? turboJsEnginePlan.mode : 'none',
    turboJsEngineEnabled: turboJsEnginePlan ? !!turboJsEnginePlan.enabled : false,
    scriptEnginePolicyVersion: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.version : 0,
    scriptEngineFallback: scriptEnginePolicyPlan ? scriptEnginePolicyPlan.fallback : 'none',
    turboJsEngineModuleVersion: turboJsEngineModulePlan ? turboJsEngineModulePlan.version : 0,
    turboJsEngineModuleMode: turboJsEngineModulePlan && turboJsEngineModulePlan.enabled ? turboJsEngineModulePlan.executionMode : 'none',
    turboJsEngineModuleLoaded: !!turboJsEngineModuleUrl,
    turboJsEngineModuleUrl: turboJsEngineModuleUrl || '',
    turboJsContextLifecycleVersion: turboJsContextLifecyclePlan ? turboJsContextLifecyclePlan.version : 0,
    turboJsContextLifecycleMode: turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.enabled ? turboJsContextLifecyclePlan.model : 'none',
    hostCapabilitiesVersion: hostCapabilitiesPlan ? hostCapabilitiesPlan.version : 0,
    hostCapabilityCount: hostCapabilitiesPlan && hostCapabilitiesPlan.capabilities ? hostCapabilitiesPlan.capabilities.length : 0,
    turboJsAdapterDiagnosticsVersion: turboJsAdapterDiagnosticsPlan ? turboJsAdapterDiagnosticsPlan.version : 0,
    turboJsAdapterDiagnosticsMode: turboJsAdapterDiagnosticsPlan && turboJsAdapterDiagnosticsPlan.enabled ? 'enabled' : 'none',
    turboJsWasmRuntimeVersion: turboJsWasmRuntimePlan ? turboJsWasmRuntimePlan.version : 0,
    turboJsWasmRuntimeMode: turboJsWasmRuntimePlan && turboJsWasmRuntimePlan.enabled ? turboJsWasmRuntimePlan.executionMode : 'none',
    turboJsWasmUrl: turboJsWasmUrl || '',
    turboJsSourceTransferVersion: turboJsSourceTransferPlan ? turboJsSourceTransferPlan.version : 0,
    turboJsSourceTransferMode: turboJsSourceTransferPlan && turboJsSourceTransferPlan.enabled ? 'enabled' : 'none',
    turboJsConsoleBridgeVersion: turboJsConsoleBridgePlan ? turboJsConsoleBridgePlan.version : 0,
    turboJsConsoleBridgeMode: turboJsConsoleBridgePlan && turboJsConsoleBridgePlan.enabled ? turboJsConsoleBridgePlan.mode : 'none',
    turboJsExecutionRecordsVersion: turboJsExecutionRecordsPlan ? turboJsExecutionRecordsPlan.version : 0,
    turboJsExecutionRecordsMode: turboJsExecutionRecordsPlan && turboJsExecutionRecordsPlan.enabled ? turboJsExecutionRecordsPlan.retention : 'none',
    turboJsResultBridgeVersion: turboJsResultBridgePlan ? turboJsResultBridgePlan.version : 0,
    turboJsResultBridgeMode: turboJsResultBridgePlan && turboJsResultBridgePlan.enabled ? turboJsResultBridgePlan.format : 'none',
    turboJsFallbackPolicyVersion: turboJsFallbackPolicyPlan ? turboJsFallbackPolicyPlan.version : 0,
    turboJsFallbackPolicyMode: turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled ? turboJsFallbackPolicyPlan.mode : 'none',
    turboJsRuntimeAbi: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.abi : 0,
    turboJsRuntimeAbiHash: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.tableHash : '',
    turboJsHostImportCount: turboJsHostImportsPlan ? turboJsHostImportsPlan.importCount : 0,
    turboJsHeapLimit: turboJsHeapLimitsPlan ? turboJsHeapLimitsPlan.heapLimit : 0,
    turboJsScriptBufferCapacity: turboJsScriptBufferPlan ? turboJsScriptBufferPlan.capacity : 0,
    turboJsConsoleAbi: turboJsConsoleAbiPlan ? turboJsConsoleAbiPlan.abi : 0,
    turboJsParityProbeExpected: turboJsParityProbePlan ? turboJsParityProbePlan.expected : '',
    turboJsReleaseFallbackPolicy: turboJsReleaseFallbackPlan ? turboJsReleaseFallbackPlan.policy : '',
    turboJsBytecodeManifestVersion: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.version : 0,
    turboJsBytecodeManifestMode: turboJsBytecodeManifestPlan && turboJsBytecodeManifestPlan.enabled ? turboJsBytecodeManifestPlan.format : 'none',
    turboJsBytecodeChunkCount: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.chunkCount : 0,
    turboJsModuleResolverVersion: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.version : 0,
    turboJsModuleResolverMode: turboJsModuleResolverPlan && turboJsModuleResolverPlan.enabled ? turboJsModuleResolverPlan.mode : 'none',
    turboJsExceptionAbi: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.abi : 0,
    turboJsExceptionAbiVersion: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.version : 0,
    turboJsHostTrapPolicy: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.policy : '',
    turboJsHostTrapPolicyVersion: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.version : 0,
    turboJsExecutionLifecycleVersion: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.version : 0,
    turboJsExecutionLifecycleStates: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.states : '',
    turboJsScriptBufferPolicyVersion: turboJsScriptBufferPolicyPlan ? turboJsScriptBufferPolicyPlan.version : 0,
    turboJsContextLimitPolicyVersion: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.version : 0,
    turboJsHostCallDispatchVersion: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.version : 0,
    turboJsHostCallDispatchCount: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.entryCount : 0,
    turboJsParityContractVersion: turboJsParityContractPlan ? turboJsParityContractPlan.version : 0,
    turboJsReleaseFailClosedVersion: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.version : 0,
    turboJsModuleGraphVersion: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.version : 0,
    turboJsModuleGraphCount: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.moduleCount : 0,
    turboJsModuleExecutionVersion: turboJsModuleExecutionPlan ? turboJsModuleExecutionPlan.version : 0,
    turboJsModuleExecutionMode: turboJsModuleExecutionPlan && turboJsModuleExecutionPlan.enabled ? turboJsModuleExecutionPlan.mode : 'none',
    turboJsModuleCacheVersion: turboJsModuleCachePlan ? turboJsModuleCachePlan.version : 0,
    turboJsResolverAuditVersion: turboJsResolverAuditPlan ? turboJsResolverAuditPlan.version : 0,
    turboJsInteropFallbackVersion: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.version : 0,
    turboJsExecutionJournalVersion: turboJsExecutionJournalPlan ? turboJsExecutionJournalPlan.version : 0,
    turboJsCheckpointPolicyVersion: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.version : 0,
    turboJsReplayCursorVersion: turboJsReplayCursorPlan ? turboJsReplayCursorPlan.version : 0,
    turboJsResumeStateVersion: turboJsResumeStatePlan ? turboJsResumeStatePlan.version : 0,
    turboJsDeterminismAuditVersion: turboJsDeterminismAuditPlan ? turboJsDeterminismAuditPlan.version : 0,
    turboJsSnapshotPolicyVersion: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.version : 0,
    turboJsSnapshotRecordsVersion: turboJsSnapshotRecordsPlan ? turboJsSnapshotRecordsPlan.version : 0,
    turboJsReplayValidationVersion: turboJsReplayValidationPlan ? turboJsReplayValidationPlan.version : 0,
    turboJsDeterminismLedgerVersion: turboJsDeterminismLedgerPlan ? turboJsDeterminismLedgerPlan.version : 0,
    turboJsAuditSealVersion: turboJsAuditSealPlan ? turboJsAuditSealPlan.version : 0,
    turboJsExecutionCommitVersion: turboJsExecutionCommitPlan ? turboJsExecutionCommitPlan.version : 0,
    turboJsRollbackPolicyVersion: turboJsRollbackPolicyPlan ? turboJsRollbackPolicyPlan.version : 0,
    turboJsHostCallReceiptsVersion: turboJsHostCallReceiptsPlan ? turboJsHostCallReceiptsPlan.version : 0,
    turboJsReleaseAcceptanceVersion: turboJsReleaseAcceptancePlan ? turboJsReleaseAcceptancePlan.version : 0,
    turboJsCommitAuditVersion: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.version : 0,
    turboJsCapabilityPolicyVersion: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.version : 0,
    turboJsHostIoPolicyVersion: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.version : 0,
    turboJsPermissionSealVersion: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.version : 0,
    turboJsPolicyReceiptsVersion: turboJsPolicyReceiptsPlan ? turboJsPolicyReceiptsPlan.version : 0,
    turboJsReleaseGateVersion: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.version : 0,
    turboJsHostIoDecisionVersion: turboJsHostIoDecisionPlan ? turboJsHostIoDecisionPlan.version : 0,
    turboJsHostIoDenyTraceVersion: turboJsHostIoDenyTracePlan ? turboJsHostIoDenyTracePlan.version : 0,
    turboJsCapabilityLedgerVersion: turboJsCapabilityLedgerPlan ? turboJsCapabilityLedgerPlan.version : 0,
    turboJsPolicySealAuditVersion: turboJsPolicySealAuditPlan ? turboJsPolicySealAuditPlan.version : 0,
    turboJsRuntimeDenylistVersion: turboJsRuntimeDenylistPlan ? turboJsRuntimeDenylistPlan.version : 0,
    turboJsExecutionJournal() { return turboJsEngine.executionJournal ? turboJsEngine.executionJournal() : Object.freeze({ plan: turboJsExecutionJournalPlan || null, records: [] }); },
    turboJsCheckpointPolicy() { return turboJsEngine.checkpointPolicy ? turboJsEngine.checkpointPolicy() : Object.freeze({ plan: turboJsCheckpointPolicyPlan || null }); },
    turboJsReplayCursor() { return turboJsEngine.replayCursor ? turboJsEngine.replayCursor() : Object.freeze({ plan: turboJsReplayCursorPlan || null, sequence: 0 }); },
    turboJsResumeState() { return turboJsEngine.resumeState ? turboJsEngine.resumeState() : Object.freeze({ plan: turboJsResumeStatePlan || null }); },
    turboJsDeterminismAudit() { return turboJsEngine.determinismAudit ? turboJsEngine.determinismAudit() : Object.freeze({ plan: turboJsDeterminismAuditPlan || null }); },
    turboJsSnapshotPolicy() { return turboJsEngine.snapshotPolicy ? turboJsEngine.snapshotPolicy() : Object.freeze({ plan: turboJsSnapshotPolicyPlan || null }); },
    turboJsSnapshotRecord() { return turboJsEngine.snapshotRecord ? turboJsEngine.snapshotRecord() : Object.freeze({ plan: turboJsSnapshotRecordsPlan || null }); },
    turboJsReplayValidation() { return turboJsEngine.replayValidation ? turboJsEngine.replayValidation() : Object.freeze({ plan: turboJsReplayValidationPlan || null }); },
    turboJsDeterminismLedger() { return turboJsEngine.determinismLedger ? turboJsEngine.determinismLedger() : Object.freeze({ plan: turboJsDeterminismLedgerPlan || null }); },
    turboJsAuditSeal() { return turboJsEngine.auditSeal ? turboJsEngine.auditSeal() : Object.freeze({ plan: turboJsAuditSealPlan || null }); },
    turboJsExecutionCommit() { return turboJsEngine.executionCommit ? turboJsEngine.executionCommit() : Object.freeze({ plan: turboJsExecutionCommitPlan || null }); },
    turboJsRollbackPolicy() { return turboJsEngine.rollbackPolicy ? turboJsEngine.rollbackPolicy() : Object.freeze({ plan: turboJsRollbackPolicyPlan || null }); },
    turboJsHostCallReceipts() { return turboJsEngine.hostCallReceipts ? turboJsEngine.hostCallReceipts() : Object.freeze({ plan: turboJsHostCallReceiptsPlan || null }); },
    turboJsReleaseAcceptance() { return turboJsEngine.releaseAcceptance ? turboJsEngine.releaseAcceptance() : Object.freeze({ plan: turboJsReleaseAcceptancePlan || null }); },
    turboJsCommitAudit() { return turboJsEngine.commitAudit ? turboJsEngine.commitAudit() : Object.freeze({ plan: turboJsCommitAuditPlan || null }); },
    turboJsCapabilityPolicy() { return turboJsEngine.capabilityPolicy ? turboJsEngine.capabilityPolicy() : Object.freeze({ plan: turboJsCapabilityPolicyPlan || null }); },
    turboJsHostIoPolicy() { return turboJsEngine.hostIoPolicy ? turboJsEngine.hostIoPolicy() : Object.freeze({ plan: turboJsHostIoPolicyPlan || null }); },
    turboJsPermissionSeal() { return turboJsEngine.permissionSeal ? turboJsEngine.permissionSeal() : Object.freeze({ plan: turboJsPermissionSealPlan || null }); },
    turboJsPolicyReceipts() { return turboJsEngine.policyReceipts ? turboJsEngine.policyReceipts() : Object.freeze({ plan: turboJsPolicyReceiptsPlan || null }); },
    turboJsReleaseGate() { return turboJsEngine.releaseGate ? turboJsEngine.releaseGate() : Object.freeze({ plan: turboJsReleaseGatePlan || null }); },
    turboJsHostIoDecision() { return turboJsEngine.hostIoDecision ? turboJsEngine.hostIoDecision() : Object.freeze({ plan: turboJsHostIoDecisionPlan || null }); },
    turboJsHostIoDenyTrace() { return turboJsEngine.hostIoDenyTrace ? turboJsEngine.hostIoDenyTrace() : Object.freeze({ plan: turboJsHostIoDenyTracePlan || null }); },
    turboJsCapabilityLedger() { return turboJsEngine.capabilityLedger ? turboJsEngine.capabilityLedger() : Object.freeze({ plan: turboJsCapabilityLedgerPlan || null }); },
    turboJsPolicySealAudit() { return turboJsEngine.policySealAudit ? turboJsEngine.policySealAudit() : Object.freeze({ plan: turboJsPolicySealAuditPlan || null }); },
    turboJsRuntimeDenylist() { return turboJsEngine.runtimeDenylist ? turboJsEngine.runtimeDenylist() : Object.freeze({ plan: turboJsRuntimeDenylistPlan || null }); },
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
    callTurboJs(entry, payload) {
      return asyncQueue.callTurboJs(entry, payload);
    },
    createTurboJsContext(route, source, order) {
      return turboJsEngine.createContext(route, source, order);
    },
    executeTurboJsChunk(chunk, route) {
      return turboJsEngine.executeChunk(chunk, route, bridgeRef || this);
    },
    destroyTurboJsContext(route, source, order) {
      return turboJsEngine.destroyContext(route, source, order);
    },
    turboJsContextSnapshot() {
      return turboJsEngine.contextSnapshot();
    },
    turboJsEngineModuleStatus() {
      return turboJsEngine.moduleStatus();
    },
    turboJsExecutionSnapshot() {
      return turboJsEngine.executionSnapshot();
    },
    turboJsConsoleEvents() {
      return turboJsEngine.consoleEvents();
    },
    clearTurboJsConsoleEvents() {
      return turboJsEngine.clearConsoleEvents();
    },
    turboJsFallbackPolicy() {
      return turboJsEngine.fallbackPolicy();
    },
    turboJsAbiTable() {
      return turboJsEngine.abiTable();
    },
    turboJsHostImportTable() {
      return turboJsEngine.hostImportTable();
    },
    turboJsParityProbe() {
      return turboJsEngine.parityProbe();
    },
    turboJsBytecodeManifest() {
      return turboJsEngine.bytecodeManifest();
    },
    turboJsModuleResolver() {
      return turboJsEngine.moduleResolver();
    },
    turboJsExceptionAbi() {
      return turboJsEngine.exceptionAbi();
    },
    turboJsHostTrapPolicy() {
      return turboJsEngine.hostTrapPolicy();
    },
    turboJsExecutionLifecycle() {
      return turboJsEngine.executionLifecycle();
    },
    turboJsScriptBufferPolicy() {
      return turboJsEngine.scriptBufferPolicy();
    },
    turboJsContextLimitPolicy() {
      return turboJsEngine.contextLimitPolicy();
    },
    turboJsHostCallDispatch() {
      return turboJsEngine.hostCallDispatch();
    },
    turboJsParityContract() {
      return turboJsEngine.parityContract();
    },
    turboJsReleaseFailClosed() {
      return turboJsEngine.releaseFailClosed();
    },
    turboJsModuleGraph() {
      return turboJsEngine.moduleGraph();
    },
    turboJsModuleCacheSnapshot() {
      return turboJsEngine.moduleCacheSnapshot();
    },
    turboJsResolverAudit() {
      return turboJsEngine.resolverAudit();
    },
    turboJsInteropFallback() {
      return turboJsEngine.interopFallback();
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
      const contexts = turboJsEngine.destroyRoute(route);
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
  // Manifest V3 extension pages cannot execute blob-backed modules under their
  // mandatory CSP. Chrome-facing browser adapters are emitted as hardened,
  // physical extension resources and loaded by the generated route shell.
  // Keep their packaged copies for graph/audit metadata, but never execute
  // those copies through the website blob-module linker.
  if (typeof location !== 'undefined' && location.protocol === 'chrome-extension:') {
    return Object.freeze({ ...chunk, executed: false, external: true, browser: true, chromeExtension: true });
  }
  if (chunk && chunk.bytecodeBytes && chunk.bytecodeBytes.length) {
    throw new Error('browser script chunk was packaged as TurboJS bytecode: ' + (chunk.source || '<script>'));
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

async function executeScriptChunk(chunk, route, scriptIsolationPlan = null, scriptPolicyPlan = null, turboJsChunkPlan = null, turboJsEnginePlan = null, scriptEnginePolicyPlan = null, browserModuleLinker = null) {
  const bridge = globalThis.__venomRuntime || null;
  if (bridge && typeof bridge.checkScriptPolicy === 'function') bridge.checkScriptPolicy(chunk);
  if (bridge && typeof bridge.createScriptScope === 'function') bridge.createScriptScope(route && route.route ? route.route : chunk.route, chunk.source, chunk.order);
  const startRecord = bridge && typeof bridge.enqueueHostCall === 'function'
    ? bridge.enqueueHostCall(scriptPolicyPlan && scriptPolicyPlan.chunkStartHostCall ? scriptPolicyPlan.chunkStartHostCall : 'script.chunkStart', { route: chunk.route, source: chunk.source, order: chunk.order })
    : null;
  if (bridge && typeof bridge.callTurboJs === 'function') {
    const tjsBoundaryRecord = bridge.callTurboJs('chunk:' + chunk.order, { route: chunk.route, source: chunk.source, bytes: chunk.code ? chunk.code.length : 0, boundary: turboJsChunkPlan ? turboJsChunkPlan.mode : 'engine-input', engine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'bootstrap' });
    if (bridge && typeof bridge.settleHostCall === 'function' && tjsBoundaryRecord && tjsBoundaryRecord.id) {
      bridge.settleHostCall(tjsBoundaryRecord.id, 'fulfilled', { boundary: turboJsChunkPlan ? turboJsChunkPlan.mode : 'engine-input', engine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'bootstrap' });
    }
  }
  let executed;
  if ((chunk.flags & SCRIPT_FLAG.BROWSER) !== 0) executed = await executeBrowserScriptChunk(chunk, browserModuleLinker);
  else if (bridge && typeof bridge.executeTurboJsChunk === 'function') {
    executed = await bridge.executeTurboJsChunk(chunk, route);
  } else {
    const denyHostFallback = !!(scriptEnginePolicyPlan && String(scriptEnginePolicyPlan.fallback || '').includes('deny-host-js'));
    if (denyHostFallback) throw new Error('TurboJS/WASM backend unavailable; protected host JS fallback denied');
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
  if (bridge && startRecord) bridge.settleHostCall(startRecord.id, 'fulfilled', { executed: !!(executed && executed.executed), isolated: !!scriptIsolationPlan, engine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'none' });
  return Object.freeze({ ...executed, isolated: !!scriptIsolationPlan, isolationMode: scriptIsolationPlan ? scriptIsolationPlan.mode : 'none', turboJsBoundary: turboJsChunkPlan ? turboJsChunkPlan.mode : 'none', turboJsEngine: turboJsEnginePlan ? turboJsEnginePlan.mode : 'none' });
}

async function executeScriptsForRoute(route, jsBundle, scriptIsolationPlan = null, scriptPolicyPlan = null, turboJsChunkPlan = null, turboJsEnginePlan = null, scriptEnginePolicyPlan = null) {
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
      executed.push(await executeScriptChunk(chunk, route, scriptIsolationPlan, scriptPolicyPlan, turboJsChunkPlan, turboJsEnginePlan, scriptEnginePolicyPlan, browserModuleLinker));
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

