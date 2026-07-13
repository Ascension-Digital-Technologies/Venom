import { readFile, readdir } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import { resolve, join } from 'node:path';

const distDir = resolve(process.argv[2] || 'dist');
const expectedMode = process.argv[3] || 'js';

class ElementNode {
  constructor(tagName) {
    this.tagName = tagName;
    this.children = [];
    this.attributes = new Map();
    this.listeners = new Map();
  }
  appendChild(node) {
    this.children.push(node);
    return node;
  }
  replaceChildren() {
    this.children = [];
  }
  setAttribute(name, value) {
    this.attributes.set(name, value);
  }
  addEventListener(name, handler) {
    const list = this.listeners.get(name) || [];
    list.push(handler);
    this.listeners.set(name, list);
  }
  dispatchEvent(event) {
    const list = this.listeners.get(event && event.type ? event.type : '') || [];
    for (const handler of list) handler.call(this, event);
    return list.length > 0;
  }
  closest() {
    return null;
  }
  toText() {
    return this.children.map((child) => child.toText()).join('');
  }
}

class TextNode {
  constructor(text) {
    this.text = text;
  }
  toText() {
    return this.text;
  }
}


function findFirst(node, tagName) {
  if (node.tagName === tagName) return node;
  for (const child of node.children || []) {
    const found = findFirst(child, tagName);
    if (found) return found;
  }
  return null;
}

globalThis.Element = ElementNode;
globalThis.document = {
  createElement: (tag) => new ElementNode(tag),
  createTextNode: (text) => new TextNode(text),
  addEventListener: () => {},
};
globalThis.window = { addEventListener: () => {} };
globalThis.history = { pushState: () => {} };
globalThis.location = { pathname: '/' };
globalThis.fetch = async (url) => {
  let filePath;
  const raw = String(url);
  if (raw.startsWith('file://')) {
    filePath = new URL(raw);
  } else {
    const clean = raw.replace(/^\.\//, '');
    filePath = join(distDir, 'assets', clean);
  }
  const bytes = await readFile(filePath);
  return {
    ok: true,
    status: 200,
    arrayBuffer: async () => bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength),
  };
};

const assetFiles = await readdir(join(distDir, 'assets'));
const runtimeFile = assetFiles.find((name) => expectedMode === 'wasm' ? /^runtime-js-bridge(\.[a-f0-9]+)?\.js$/.test(name) : /^runtime(\.[a-f0-9]+)?\.js$/.test(name));
const packageFile = assetFiles.find((name) => /^app(\.[a-f0-9]+)?\.vbc$/.test(name));
const quickJsEngineFile = assetFiles.find((name) => /^quickjs-engine(\.[a-f0-9]+)?\.js$/.test(name));
if (!runtimeFile || !packageFile || !quickJsEngineFile) throw new Error('missing runtime/package/QuickJS engine assets');

const root = new ElementNode('div');
const runtime = await import(pathToFileURL(join(distDir, 'assets', runtimeFile)));
const assetBaseUrl = pathToFileURL(join(distDir, 'assets') + '/').href;
const result = await runtime.bootVenom({ root, packageUrl: './' + packageFile, assetBaseUrl, quickJsEngineUrl: pathToFileURL(join(distDir, 'assets', quickJsEngineFile)).href });
const text = root.toText();

if (result.packageVersion !== 40) {
  throw new Error(`expected package v38, got ${result.packageVersion}`);
}
if ((result.runtimeMode || 'js') !== expectedMode) {
  throw new Error(`expected runtime mode ${expectedMode}, got ${result.runtimeMode || 'js'}`);
}
if (expectedMode === 'wasm') {
  if (result.wasmRuntimeAbi !== 1) {
    throw new Error(`expected WASM runtime ABI 1, got ${result.wasmRuntimeAbi}`);
  }
  if (!result.wasmExecution || result.wasmExecution.version !== 40 || result.wasmExecution.executed < 1) {
    throw new Error(`WASM runtime did not execute package VM bytecode: ${JSON.stringify(result.wasmExecution)}`);
  }
  if (!String(result.wasmExecution.text || '').includes('Hello from Venom')) {
    throw new Error(`WASM runtime report did not contain expected text: ${JSON.stringify(result.wasmExecution)}`);
  }
  if (!Array.isArray(result.wasmExecution.domOps) || result.wasmExecution.domOps.length < 1 || result.wasmExecution.binaryDomOps !== true || result.wasmExecution.domOpBytes < 16) {
    throw new Error(`WASM runtime report did not include binary DOM ops: ${JSON.stringify(result.wasmExecution)}`);
  }
}

if (result.route !== '/') {
  throw new Error(`expected root route, got ${result.route}`);
}
if (!text.includes('Hello from Venom') || !text.includes('This is the home route.')) {
  throw new Error(`runtime did not render expected route text: ${text}`);
}
const expectedRuntimeMarker = expectedMode === 'wasm' ? 'wasm-bin-dom' : 'vm';
if (root.attributes.get('data-venom-runtime') !== expectedRuntimeMarker) {
  throw new Error(`runtime did not mark ${expectedRuntimeMarker} execution; got ${root.attributes.get('data-venom-runtime')}`);
}
if (expectedMode === 'wasm' && root.attributes.get('data-venom-wasm-runtime') !== 'executed') {
  throw new Error('runtime did not mark WASM VM execution');
}
if (expectedMode === 'wasm' && Number(root.attributes.get('data-venom-wasm-dom-ops') || '0') < 1) {
  throw new Error('runtime did not apply WASM DOM operation stream');
}
if (expectedMode === 'wasm' && Number(root.attributes.get('data-venom-wasm-dom-bytes') || '0') < 16) {
  throw new Error('runtime did not expose WASM binary DOM operation byte count');
}
const earlyQuickJsSnapshot = globalThis.__venomRuntime && typeof globalThis.__venomRuntime.quickJsExecutionSnapshot === 'function'
  ? globalThis.__venomRuntime.quickJsExecutionSnapshot()
  : null;
const earlyQuickJsRecords = earlyQuickJsSnapshot && Array.isArray(earlyQuickJsSnapshot.records) ? earlyQuickJsSnapshot.records : [];
const quickJsWasmAccepted = earlyQuickJsRecords.some((entry) => entry && entry.kind === 'quickjs.executionRecord' && entry.wasmAccepted === true);
if (result.scripts < 1 || result.executedScripts < 1 || (globalThis.__venomExampleScript !== 1 && !quickJsWasmAccepted)) {
  throw new Error(`runtime did not execute packaged JS chunk or accept QuickJS WASM boundary: scripts=${result.scripts} executed=${result.executedScripts} marker=${globalThis.__venomExampleScript} wasmAccepted=${quickJsWasmAccepted}`);
}
if (!globalThis.__venomRuntime || globalThis.__venomRuntime.packageVersion !== 40) {
  throw new Error('runtime did not install the Venom host bridge');
}
if (Object.getOwnPropertyDescriptor(globalThis, '__venomRuntime')?.configurable !== false) {
  throw new Error('Venom host bridge is not locked down');
}

const img = findFirst(root, 'img');
if (!img || !String(img.attributes.get('src') || '').includes('assets_logo')) {
  throw new Error(`runtime did not remap image asset source: ${img && img.attributes.get('src')}`);
}

if (result.hostBridgeVersion !== 13 || result.hostCallCount < 88) {
  throw new Error(`host bridge metadata was not parsed: version=${result.hostBridgeVersion} calls=${result.hostCallCount}`);
}

if (result.fetchBridgeVersion !== 1 || result.fetchBridgeMode !== 'async-host-call') {
  throw new Error(`fetch bridge metadata was not parsed: version=${result.fetchBridgeVersion} mode=${result.fetchBridgeMode}`);
}
if (result.asyncHostQueueVersion !== 10 || result.asyncHostQueueMode !== 'enabled') {
  throw new Error(`async host queue metadata was not parsed: version=${result.asyncHostQueueVersion} mode=${result.asyncHostQueueMode}`);
}
if (result.timerBridgeVersion !== 1 || result.timerBridgeMode !== 'async-host-call') {
  throw new Error(`timer bridge metadata was not parsed: version=${result.timerBridgeVersion} mode=${result.timerBridgeMode}`);
}
if (result.eventQueueVersion !== 1 || result.eventQueueMode !== 'enabled') {
  throw new Error(`event queue metadata was not parsed: version=${result.eventQueueVersion} mode=${result.eventQueueMode}`);
}
if (result.quickJsBridgeVersion !== 10 || result.quickJsBridgeMode !== 'engine-backend-select') {
  throw new Error(`QuickJS bridge metadata was not parsed: version=${result.quickJsBridgeVersion} mode=${result.quickJsBridgeMode}`);
}

if (result.scriptIsolationVersion !== 4 || result.scriptIsolationMode !== 'route-scoped' || result.scriptPolicyVersion !== 4 || result.quickJsChunkVersion !== 7 || result.quickJsChunkMode !== 'engine-input' || result.quickJsEngineVersion !== 7 || result.quickJsEngineMode !== 'engine-backend-replacement-path') {
  throw new Error(`script isolation / QuickJS chunk metadata was not parsed: ${JSON.stringify({scriptIsolationVersion: result.scriptIsolationVersion, scriptIsolationMode: result.scriptIsolationMode, scriptPolicyVersion: result.scriptPolicyVersion, quickJsChunkVersion: result.quickJsChunkVersion, quickJsChunkMode: result.quickJsChunkMode})}`);
}
if (globalThis.__venomRuntime.scriptIsolationMode !== 'route-scoped' || globalThis.__venomRuntime.quickJsChunkMode !== 'engine-input' || globalThis.__venomRuntime.quickJsChunkCount < 1 || globalThis.__venomRuntime.quickJsEngineMode !== 'engine-backend-replacement-path') {
  throw new Error('Venom host bridge did not expose script isolation / QuickJS chunk metadata');
}

if (result.quickJsEngineModuleVersion !== 10 || result.quickJsEngineModuleMode !== 'quickjs-wasm-abi12-upstream-global-host-api-shims' || result.quickJsEngineModuleLoaded !== true) {
  throw new Error(`QuickJS engine module metadata was not active: ${JSON.stringify({version: result.quickJsEngineModuleVersion, mode: result.quickJsEngineModuleMode, loaded: result.quickJsEngineModuleLoaded})}`);
}
if (globalThis.__venomRuntime.quickJsEngineModuleMode !== 'quickjs-wasm-abi12-upstream-global-host-api-shims' || globalThis.__venomQuickJsModuleProbe < 1) {
  throw new Error('QuickJS engine module did not execute the route script chunk');
}
if (result.quickJsContextLifecycleVersion !== 4 || result.quickJsContextLifecycleMode !== 'route-scoped-reusable' || result.hostCapabilitiesVersion !== 2 || result.hostCapabilityCount < 7 || result.quickJsAdapterDiagnosticsVersion !== 4) {
  throw new Error(`QuickJS adapter lifecycle metadata was not active: ${JSON.stringify({context: result.quickJsContextLifecycleVersion, hostCapabilities: result.hostCapabilitiesVersion, diagnostics: result.quickJsAdapterDiagnosticsVersion})}`);
}
const contextSnapshot = globalThis.__venomRuntime.quickJsContextSnapshot();
if (!contextSnapshot || contextSnapshot.count < 1) {
  throw new Error(`QuickJS context lifecycle snapshot was not populated: ${JSON.stringify(contextSnapshot)}`);
}
const moduleStatus = globalThis.__venomRuntime.quickJsEngineModuleStatus();
if (!moduleStatus || moduleStatus.moduleLoaded !== true || moduleStatus.contextCount < 1) {
  throw new Error(`QuickJS engine module status was not populated: ${JSON.stringify(moduleStatus)}`);
}

if (result.quickJsExecutionRecordsVersion !== 4 || result.quickJsResultBridgeVersion !== 4 || result.quickJsFallbackPolicyVersion !== 4) {
  throw new Error(`QuickJS result bridge metadata was not active: ${JSON.stringify({executionRecords: result.quickJsExecutionRecordsVersion, resultBridge: result.quickJsResultBridgeVersion, fallbackPolicy: result.quickJsFallbackPolicyVersion})}`);
}
const qjsExecutionSnapshot = globalThis.__venomRuntime.quickJsExecutionSnapshot();
if (!qjsExecutionSnapshot || qjsExecutionSnapshot.count < 1) {
  throw new Error(`QuickJS execution records were not captured: ${JSON.stringify(qjsExecutionSnapshot)}`);
}
if (typeof globalThis.__venomRuntime.quickJsFallbackPolicy !== 'function' || globalThis.__venomRuntime.quickJsFallbackPolicy().mode !== 'explicit-policy-gated') {
  throw new Error('QuickJS fallback policy was not exposed through the host bridge');
}
if (typeof globalThis.__venomRuntime.quickJsAbiTable !== 'function' || globalThis.__venomRuntime.quickJsAbiTable().abi < 12 || globalThis.__venomRuntime.quickJsRuntimeAbi < 12) {
  throw new Error('QuickJS runtime ABI table was not exposed through the host bridge');
}
if (typeof globalThis.__venomRuntime.quickJsHostImportTable !== 'function' || globalThis.__venomRuntime.quickJsHostImportTable().importCount < 1 || globalThis.__venomRuntime.quickJsHostImportCount < 1) {
  throw new Error('QuickJS host import table was not exposed through the host bridge');
}
if (typeof globalThis.__venomRuntime.quickJsParityProbe !== 'function' || !globalThis.__venomRuntime.quickJsParityProbe()) {
  throw new Error('QuickJS parity probe was not exposed through the host bridge');
}
if (globalThis.__venomRuntime.quickJsHeapLimit < 1024 || globalThis.__venomRuntime.quickJsScriptBufferCapacity < 1024 || globalThis.__venomRuntime.quickJsConsoleAbi < 1) {
  throw new Error('QuickJS heap/script-buffer/console ABI metadata was not exposed through the host bridge');
}
if (globalThis.__venomRuntime.quickJsBytecodeManifestVersion < 1 || globalThis.__venomRuntime.quickJsModuleResolverVersion !== 1 || globalThis.__venomRuntime.quickJsExceptionAbi < 1 || globalThis.__venomRuntime.quickJsHostTrapPolicyVersion !== 1) {
  throw new Error(`QuickJS bytecode/module/exception/trap metadata was not exposed: ${JSON.stringify({bytecode: globalThis.__venomRuntime.quickJsBytecodeManifestVersion, moduleResolver: globalThis.__venomRuntime.quickJsModuleResolverVersion, exceptionAbi: globalThis.__venomRuntime.quickJsExceptionAbi, trapPolicy: globalThis.__venomRuntime.quickJsHostTrapPolicyVersion})}`);
}

if (globalThis.__venomRuntime.quickJsExecutionLifecycleVersion !== 1 || globalThis.__venomRuntime.quickJsHostCallDispatchCount < 34 || globalThis.__venomRuntime.quickJsReleaseFailClosedVersion !== 1) {
  throw new Error(`QuickJS v0.37 execution-pipeline metadata was not exposed: ${JSON.stringify({lifecycle: globalThis.__venomRuntime.quickJsExecutionLifecycleVersion, hostDispatch: globalThis.__venomRuntime.quickJsHostCallDispatchCount, release: globalThis.__venomRuntime.quickJsReleaseFailClosedVersion})}`);
}

if (globalThis.__venomRuntime.quickJsModuleGraphVersion !== 1 || globalThis.__venomRuntime.quickJsModuleExecutionVersion !== 1 || globalThis.__venomRuntime.quickJsModuleCacheVersion !== 1 || globalThis.__venomRuntime.quickJsResolverAuditVersion !== 1 || globalThis.__venomRuntime.quickJsInteropFallbackVersion !== 1) {
  throw new Error(`QuickJS module graph/cache/audit/fallback metadata was not exposed: ${JSON.stringify({moduleGraph: globalThis.__venomRuntime.quickJsModuleGraphVersion, moduleExecution: globalThis.__venomRuntime.quickJsModuleExecutionVersion, moduleCache: globalThis.__venomRuntime.quickJsModuleCacheVersion, resolverAudit: globalThis.__venomRuntime.quickJsResolverAuditVersion, interopFallback: globalThis.__venomRuntime.quickJsInteropFallbackVersion})}`);
}
if (typeof globalThis.__venomRuntime.quickJsModuleGraph !== 'function' || typeof globalThis.__venomRuntime.quickJsModuleCacheSnapshot !== 'function' || typeof globalThis.__venomRuntime.quickJsResolverAudit !== 'function' || typeof globalThis.__venomRuntime.quickJsInteropFallback !== 'function') {
  throw new Error('QuickJS module graph/cache/audit/fallback helpers were not exposed through the host bridge');
}
const qjsModuleGraph = globalThis.__venomRuntime.quickJsModuleGraph();
if (!qjsModuleGraph || qjsModuleGraph.version !== 1 || qjsModuleGraph.executions < 0) {
  throw new Error(`QuickJS module graph helper returned invalid data: ${JSON.stringify(qjsModuleGraph)}`);
}
if (globalThis.__venomRuntime.quickJsExecutionJournalVersion !== 1 || globalThis.__venomRuntime.quickJsCheckpointPolicyVersion !== 1 || globalThis.__venomRuntime.quickJsReplayCursorVersion !== 1 || globalThis.__venomRuntime.quickJsResumeStateVersion !== 1 || globalThis.__venomRuntime.quickJsDeterminismAuditVersion !== 1) {
  throw new Error(`QuickJS replay/checkpoint metadata was not exposed: ${JSON.stringify({journal: globalThis.__venomRuntime.quickJsExecutionJournalVersion, checkpoint: globalThis.__venomRuntime.quickJsCheckpointPolicyVersion, replay: globalThis.__venomRuntime.quickJsReplayCursorVersion, resume: globalThis.__venomRuntime.quickJsResumeStateVersion, audit: globalThis.__venomRuntime.quickJsDeterminismAuditVersion})}`);
}
if (typeof globalThis.__venomRuntime.quickJsExecutionJournal !== 'function' || typeof globalThis.__venomRuntime.quickJsCheckpointPolicy !== 'function' || typeof globalThis.__venomRuntime.quickJsReplayCursor !== 'function' || typeof globalThis.__venomRuntime.quickJsResumeState !== 'function' || typeof globalThis.__venomRuntime.quickJsDeterminismAudit !== 'function') {
  throw new Error('QuickJS replay/checkpoint helpers were not exposed through the host bridge');
}
if (globalThis.__venomRuntime.quickJsSnapshotPolicyVersion !== 1 || globalThis.__venomRuntime.quickJsSnapshotRecordsVersion !== 1 || globalThis.__venomRuntime.quickJsReplayValidationVersion !== 1 || globalThis.__venomRuntime.quickJsDeterminismLedgerVersion !== 1 || globalThis.__venomRuntime.quickJsAuditSealVersion !== 1) {
  throw new Error(`QuickJS snapshot/audit-seal metadata was not exposed: ${JSON.stringify({snapshotPolicy: globalThis.__venomRuntime.quickJsSnapshotPolicyVersion, snapshotRecords: globalThis.__venomRuntime.quickJsSnapshotRecordsVersion, replayValidation: globalThis.__venomRuntime.quickJsReplayValidationVersion, ledger: globalThis.__venomRuntime.quickJsDeterminismLedgerVersion, auditSeal: globalThis.__venomRuntime.quickJsAuditSealVersion})}`);
}
if (globalThis.__venomRuntime.quickJsExecutionCommitVersion !== 1 || globalThis.__venomRuntime.quickJsRollbackPolicyVersion !== 1 || globalThis.__venomRuntime.quickJsHostCallReceiptsVersion !== 1 || globalThis.__venomRuntime.quickJsReleaseAcceptanceVersion !== 1 || globalThis.__venomRuntime.quickJsCommitAuditVersion !== 1) {
  throw new Error(`QuickJS commit/rollback/release metadata was not exposed: ${JSON.stringify({commit: globalThis.__venomRuntime.quickJsExecutionCommitVersion, rollback: globalThis.__venomRuntime.quickJsRollbackPolicyVersion, receipts: globalThis.__venomRuntime.quickJsHostCallReceiptsVersion, acceptance: globalThis.__venomRuntime.quickJsReleaseAcceptanceVersion, audit: globalThis.__venomRuntime.quickJsCommitAuditVersion})}`);
}
if (typeof globalThis.__venomRuntime.quickJsSnapshotPolicy !== 'function' || typeof globalThis.__venomRuntime.quickJsAuditSeal !== 'function' || typeof globalThis.__venomRuntime.quickJsExecutionCommit !== 'function' || typeof globalThis.__venomRuntime.quickJsRollbackPolicy !== 'function' || typeof globalThis.__venomRuntime.quickJsHostCallReceipts !== 'function' || typeof globalThis.__venomRuntime.quickJsReleaseAcceptance !== 'function' || typeof globalThis.__venomRuntime.quickJsCommitAudit !== 'function') {
  throw new Error('QuickJS snapshot/commit/release helpers were not exposed through the host bridge');
}
if (globalThis.__venomRuntime.quickJsCapabilityPolicyVersion !== 1 || globalThis.__venomRuntime.quickJsHostIoPolicyVersion !== 1 || globalThis.__venomRuntime.quickJsPermissionSealVersion !== 1 || globalThis.__venomRuntime.quickJsPolicyReceiptsVersion !== 1 || globalThis.__venomRuntime.quickJsReleaseGateVersion !== 1) {
  throw new Error(`QuickJS capability/I-O/release-gate metadata was not exposed: ${JSON.stringify({capability: globalThis.__venomRuntime.quickJsCapabilityPolicyVersion, io: globalThis.__venomRuntime.quickJsHostIoPolicyVersion, seal: globalThis.__venomRuntime.quickJsPermissionSealVersion, receipts: globalThis.__venomRuntime.quickJsPolicyReceiptsVersion, gate: globalThis.__venomRuntime.quickJsReleaseGateVersion})}`);
}
if (typeof globalThis.__venomRuntime.quickJsCapabilityPolicy !== 'function' || typeof globalThis.__venomRuntime.quickJsHostIoPolicy !== 'function' || typeof globalThis.__venomRuntime.quickJsPermissionSeal !== 'function' || typeof globalThis.__venomRuntime.quickJsPolicyReceipts !== 'function' || typeof globalThis.__venomRuntime.quickJsReleaseGate !== 'function') {
  throw new Error('QuickJS capability/I-O/release-gate helpers were not exposed through the host bridge');
}
if (globalThis.__venomRuntime.quickJsHostIoDecisionVersion !== 1 || globalThis.__venomRuntime.quickJsHostIoDenyTraceVersion !== 1 || globalThis.__venomRuntime.quickJsCapabilityLedgerVersion !== 1 || globalThis.__venomRuntime.quickJsPolicySealAuditVersion !== 1 || globalThis.__venomRuntime.quickJsRuntimeDenylistVersion !== 1) {
  throw new Error(`QuickJS host I/O decision/deny/audit metadata was not exposed: ${JSON.stringify({decision: globalThis.__venomRuntime.quickJsHostIoDecisionVersion, deny: globalThis.__venomRuntime.quickJsHostIoDenyTraceVersion, ledger: globalThis.__venomRuntime.quickJsCapabilityLedgerVersion, audit: globalThis.__venomRuntime.quickJsPolicySealAuditVersion, denylist: globalThis.__venomRuntime.quickJsRuntimeDenylistVersion})}`);
}
if (typeof globalThis.__venomRuntime.quickJsHostIoDecision !== 'function' || typeof globalThis.__venomRuntime.quickJsHostIoDenyTrace !== 'function' || typeof globalThis.__venomRuntime.quickJsCapabilityLedger !== 'function' || typeof globalThis.__venomRuntime.quickJsPolicySealAudit !== 'function' || typeof globalThis.__venomRuntime.quickJsRuntimeDenylist !== 'function') {
  throw new Error('QuickJS host I/O decision/deny/audit helpers were not exposed through the host bridge');
}
const qjsConsoleEvents = globalThis.__venomRuntime.quickJsConsoleEvents();
if (!Array.isArray(qjsConsoleEvents) || qjsConsoleEvents.length < 1) {
  throw new Error(`QuickJS console bridge did not capture console events: ${JSON.stringify(qjsConsoleEvents)}`);
}

if (!globalThis.__venomRuntime || globalThis.__venomRuntime.fetchBridgeMode !== 'async-host-call' || globalThis.__venomRuntime.asyncHostQueueMode !== 'enabled') {
  throw new Error('Venom host bridge did not expose fetch/async queue mode');
}
if (globalThis.__venomRuntime.timerBridgeMode !== 'async-host-call' || globalThis.__venomRuntime.eventQueueMode !== 'enabled' || globalThis.__venomRuntime.quickJsBridgeMode !== 'engine-backend-select') {
  throw new Error('Venom host bridge did not expose timer/event/QuickJS bridge modes');
}
const queued = globalThis.__venomRuntime.enqueueHostCall('test.echo', { ok: true });
if (!queued || queued.state !== 'pending') {
  throw new Error('Venom async host-call queue did not create a pending record');
}
globalThis.__venomRuntime.settleHostCall(queued.id, 'fulfilled', { ok: true });
const queueSnapshot = globalThis.__venomRuntime.asyncHostQueueSnapshot();
if (queueSnapshot.pending !== 0 || queueSnapshot.completed < 1) {
  throw new Error(`Venom async host-call queue snapshot is invalid: ${JSON.stringify(queueSnapshot)}`);
}
const timer = globalThis.__venomRuntime.scheduleTimer(() => { globalThis.__venomTimerProbe = 1; }, 1, false);
if (!timer || timer.state !== 'scheduled') {
  throw new Error('Venom timer bridge did not schedule a timer');
}
globalThis.__venomRuntime.cancelTimer(timer.id);
const qjsCall = globalThis.__venomRuntime.callQuickJs('route.main', { route: result.route });
if (!qjsCall || qjsCall.state !== 'pending' || qjsCall.mode !== 'engine-backend-select') {
  throw new Error(`Venom QuickJS bridge placeholder did not enqueue a call: ${JSON.stringify(qjsCall)}`);
}
globalThis.__venomRuntime.settleHostCall(qjsCall.id, 'fulfilled', { ok: true });

if (globalThis.__venomRuntime.eventBindingMode !== 'inline-attribute') {
  throw new Error(`host bridge did not expose inline event binding mode: ${globalThis.__venomRuntime.eventBindingMode}`);
}
const button = findFirst(root, 'button');
if (!button || button.attributes.get('data-venom-event-click') !== 'inline') {
  throw new Error('runtime did not bind inline click event metadata');
}
button.dispatchEvent({ type: 'click' });
if (globalThis.__venomInlineClick !== 1) {
  throw new Error(`inline click handler did not execute through host bridge: ${globalThis.__venomInlineClick}`);
}
const eventSnapshot = globalThis.__venomRuntime.eventQueueSnapshot();
if (!eventSnapshot || eventSnapshot.size < 1 || !eventSnapshot.last || eventSnapshot.last.eventName !== 'click') {
  throw new Error(`Venom event queue did not record inline event dispatch: ${JSON.stringify(eventSnapshot)}`);
}
if (expectedMode === 'wasm') {
  const bindOps = result.wasmExecution.domOps.filter((item) => item && item.op === 'bindEvent');
  if (bindOps.length < 1 || result.wasmExecution.eventBindings < 1) {
    throw new Error(`WASM runtime did not emit bindEvent DOM ops: ${JSON.stringify(result.wasmExecution)}`);
  }
}

console.log(`runtime rendered ${result.route} from ${result.sections} sections and ${result.executedScripts} scripts: ${text}`);
