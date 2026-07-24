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
const turboJsEngineFile = assetFiles.find((name) => /^turbojs-engine(\.[a-f0-9]+)?\.js$/.test(name));
if (!runtimeFile || !packageFile || !turboJsEngineFile) throw new Error('missing runtime/package/TurboJS engine assets');

const root = new ElementNode('div');
const runtime = await import(pathToFileURL(join(distDir, 'assets', runtimeFile)));
const assetBaseUrl = pathToFileURL(join(distDir, 'assets') + '/').href;
const result = await runtime.bootVenom({ root, packageUrl: './' + packageFile, assetBaseUrl, turboJsEngineUrl: pathToFileURL(join(distDir, 'assets', turboJsEngineFile)).href });
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
const earlyTurboJsSnapshot = globalThis.__venomRuntime && typeof globalThis.__venomRuntime.turboJsExecutionSnapshot === 'function'
  ? globalThis.__venomRuntime.turboJsExecutionSnapshot()
  : null;
const earlyTurboJsRecords = earlyTurboJsSnapshot && Array.isArray(earlyTurboJsSnapshot.records) ? earlyTurboJsSnapshot.records : [];
const turboJsWasmAccepted = earlyTurboJsRecords.some((entry) => entry && entry.kind === 'turbojs.executionRecord' && entry.wasmAccepted === true);
if (result.scripts < 1 || result.executedScripts < 1 || (globalThis.__venomExampleScript !== 1 && !turboJsWasmAccepted)) {
  throw new Error(`runtime did not execute packaged JS chunk or accept TurboJS WASM boundary: scripts=${result.scripts} executed=${result.executedScripts} marker=${globalThis.__venomExampleScript} wasmAccepted=${turboJsWasmAccepted}`);
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
if (result.turboJsBridgeVersion !== 10 || result.turboJsBridgeMode !== 'engine-backend-select') {
  throw new Error(`TurboJS bridge metadata was not parsed: version=${result.turboJsBridgeVersion} mode=${result.turboJsBridgeMode}`);
}

if (result.scriptIsolationVersion !== 4 || result.scriptIsolationMode !== 'route-scoped' || result.scriptPolicyVersion !== 4 || result.turboJsChunkVersion !== 7 || result.turboJsChunkMode !== 'engine-input' || result.turboJsEngineVersion !== 7 || result.turboJsEngineMode !== 'engine-backend-replacement-path') {
  throw new Error(`script isolation / TurboJS chunk metadata was not parsed: ${JSON.stringify({scriptIsolationVersion: result.scriptIsolationVersion, scriptIsolationMode: result.scriptIsolationMode, scriptPolicyVersion: result.scriptPolicyVersion, turboJsChunkVersion: result.turboJsChunkVersion, turboJsChunkMode: result.turboJsChunkMode})}`);
}
if (globalThis.__venomRuntime.scriptIsolationMode !== 'route-scoped' || globalThis.__venomRuntime.turboJsChunkMode !== 'engine-input' || globalThis.__venomRuntime.turboJsChunkCount < 1 || globalThis.__venomRuntime.turboJsEngineMode !== 'engine-backend-replacement-path') {
  throw new Error('Venom host bridge did not expose script isolation / TurboJS chunk metadata');
}

if (result.turboJsEngineModuleVersion !== 10 || result.turboJsEngineModuleMode !== 'turbojs-wasm-abi12-upstream-global-host-api-shims' || result.turboJsEngineModuleLoaded !== true) {
  throw new Error(`TurboJS engine module metadata was not active: ${JSON.stringify({version: result.turboJsEngineModuleVersion, mode: result.turboJsEngineModuleMode, loaded: result.turboJsEngineModuleLoaded})}`);
}
if (globalThis.__venomRuntime.turboJsEngineModuleMode !== 'turbojs-wasm-abi12-upstream-global-host-api-shims' || globalThis.__venomTurboJsModuleProbe < 1) {
  throw new Error('TurboJS engine module did not execute the route script chunk');
}
if (result.turboJsContextLifecycleVersion !== 4 || result.turboJsContextLifecycleMode !== 'route-scoped-reusable' || result.hostCapabilitiesVersion !== 2 || result.hostCapabilityCount < 7 || result.turboJsAdapterDiagnosticsVersion !== 4) {
  throw new Error(`TurboJS adapter lifecycle metadata was not active: ${JSON.stringify({context: result.turboJsContextLifecycleVersion, hostCapabilities: result.hostCapabilitiesVersion, diagnostics: result.turboJsAdapterDiagnosticsVersion})}`);
}
const contextSnapshot = globalThis.__venomRuntime.turboJsContextSnapshot();
if (!contextSnapshot || contextSnapshot.count < 1) {
  throw new Error(`TurboJS context lifecycle snapshot was not populated: ${JSON.stringify(contextSnapshot)}`);
}
const moduleStatus = globalThis.__venomRuntime.turboJsEngineModuleStatus();
if (!moduleStatus || moduleStatus.moduleLoaded !== true || moduleStatus.contextCount < 1) {
  throw new Error(`TurboJS engine module status was not populated: ${JSON.stringify(moduleStatus)}`);
}

if (result.turboJsExecutionRecordsVersion !== 4 || result.turboJsResultBridgeVersion !== 4 || result.turboJsFallbackPolicyVersion !== 4) {
  throw new Error(`TurboJS result bridge metadata was not active: ${JSON.stringify({executionRecords: result.turboJsExecutionRecordsVersion, resultBridge: result.turboJsResultBridgeVersion, fallbackPolicy: result.turboJsFallbackPolicyVersion})}`);
}
const tjsExecutionSnapshot = globalThis.__venomRuntime.turboJsExecutionSnapshot();
if (!tjsExecutionSnapshot || tjsExecutionSnapshot.count < 1) {
  throw new Error(`TurboJS execution records were not captured: ${JSON.stringify(tjsExecutionSnapshot)}`);
}
if (typeof globalThis.__venomRuntime.turboJsFallbackPolicy !== 'function' || globalThis.__venomRuntime.turboJsFallbackPolicy().mode !== 'explicit-policy-gated') {
  throw new Error('TurboJS fallback policy was not exposed through the host bridge');
}
if (typeof globalThis.__venomRuntime.turboJsAbiTable !== 'function' || globalThis.__venomRuntime.turboJsAbiTable().abi < 12 || globalThis.__venomRuntime.turboJsRuntimeAbi < 12) {
  throw new Error('TurboJS runtime ABI table was not exposed through the host bridge');
}
if (typeof globalThis.__venomRuntime.turboJsHostImportTable !== 'function' || globalThis.__venomRuntime.turboJsHostImportTable().importCount < 1 || globalThis.__venomRuntime.turboJsHostImportCount < 1) {
  throw new Error('TurboJS host import table was not exposed through the host bridge');
}
if (typeof globalThis.__venomRuntime.turboJsParityProbe !== 'function' || !globalThis.__venomRuntime.turboJsParityProbe()) {
  throw new Error('TurboJS parity probe was not exposed through the host bridge');
}
if (globalThis.__venomRuntime.turboJsHeapLimit < 1024 || globalThis.__venomRuntime.turboJsScriptBufferCapacity < 1024 || globalThis.__venomRuntime.turboJsConsoleAbi < 1) {
  throw new Error('TurboJS heap/script-buffer/console ABI metadata was not exposed through the host bridge');
}
if (globalThis.__venomRuntime.turboJsBytecodeManifestVersion < 1 || globalThis.__venomRuntime.turboJsModuleResolverVersion !== 1 || globalThis.__venomRuntime.turboJsExceptionAbi < 1 || globalThis.__venomRuntime.turboJsHostTrapPolicyVersion !== 1) {
  throw new Error(`TurboJS bytecode/module/exception/trap metadata was not exposed: ${JSON.stringify({bytecode: globalThis.__venomRuntime.turboJsBytecodeManifestVersion, moduleResolver: globalThis.__venomRuntime.turboJsModuleResolverVersion, exceptionAbi: globalThis.__venomRuntime.turboJsExceptionAbi, trapPolicy: globalThis.__venomRuntime.turboJsHostTrapPolicyVersion})}`);
}

if (globalThis.__venomRuntime.turboJsExecutionLifecycleVersion !== 1 || globalThis.__venomRuntime.turboJsHostCallDispatchCount < 34 || globalThis.__venomRuntime.turboJsReleaseFailClosedVersion !== 1) {
  throw new Error(`TurboJS v0.37 execution-pipeline metadata was not exposed: ${JSON.stringify({lifecycle: globalThis.__venomRuntime.turboJsExecutionLifecycleVersion, hostDispatch: globalThis.__venomRuntime.turboJsHostCallDispatchCount, release: globalThis.__venomRuntime.turboJsReleaseFailClosedVersion})}`);
}

if (globalThis.__venomRuntime.turboJsModuleGraphVersion !== 1 || globalThis.__venomRuntime.turboJsModuleExecutionVersion !== 1 || globalThis.__venomRuntime.turboJsModuleCacheVersion !== 1 || globalThis.__venomRuntime.turboJsResolverAuditVersion !== 1 || globalThis.__venomRuntime.turboJsInteropFallbackVersion !== 1) {
  throw new Error(`TurboJS module graph/cache/audit/fallback metadata was not exposed: ${JSON.stringify({moduleGraph: globalThis.__venomRuntime.turboJsModuleGraphVersion, moduleExecution: globalThis.__venomRuntime.turboJsModuleExecutionVersion, moduleCache: globalThis.__venomRuntime.turboJsModuleCacheVersion, resolverAudit: globalThis.__venomRuntime.turboJsResolverAuditVersion, interopFallback: globalThis.__venomRuntime.turboJsInteropFallbackVersion})}`);
}
if (typeof globalThis.__venomRuntime.turboJsModuleGraph !== 'function' || typeof globalThis.__venomRuntime.turboJsModuleCacheSnapshot !== 'function' || typeof globalThis.__venomRuntime.turboJsResolverAudit !== 'function' || typeof globalThis.__venomRuntime.turboJsInteropFallback !== 'function') {
  throw new Error('TurboJS module graph/cache/audit/fallback helpers were not exposed through the host bridge');
}
const tjsModuleGraph = globalThis.__venomRuntime.turboJsModuleGraph();
if (!tjsModuleGraph || tjsModuleGraph.version !== 1 || tjsModuleGraph.executions < 0) {
  throw new Error(`TurboJS module graph helper returned invalid data: ${JSON.stringify(tjsModuleGraph)}`);
}
if (globalThis.__venomRuntime.turboJsExecutionJournalVersion !== 1 || globalThis.__venomRuntime.turboJsCheckpointPolicyVersion !== 1 || globalThis.__venomRuntime.turboJsReplayCursorVersion !== 1 || globalThis.__venomRuntime.turboJsResumeStateVersion !== 1 || globalThis.__venomRuntime.turboJsDeterminismAuditVersion !== 1) {
  throw new Error(`TurboJS replay/checkpoint metadata was not exposed: ${JSON.stringify({journal: globalThis.__venomRuntime.turboJsExecutionJournalVersion, checkpoint: globalThis.__venomRuntime.turboJsCheckpointPolicyVersion, replay: globalThis.__venomRuntime.turboJsReplayCursorVersion, resume: globalThis.__venomRuntime.turboJsResumeStateVersion, audit: globalThis.__venomRuntime.turboJsDeterminismAuditVersion})}`);
}
if (typeof globalThis.__venomRuntime.turboJsExecutionJournal !== 'function' || typeof globalThis.__venomRuntime.turboJsCheckpointPolicy !== 'function' || typeof globalThis.__venomRuntime.turboJsReplayCursor !== 'function' || typeof globalThis.__venomRuntime.turboJsResumeState !== 'function' || typeof globalThis.__venomRuntime.turboJsDeterminismAudit !== 'function') {
  throw new Error('TurboJS replay/checkpoint helpers were not exposed through the host bridge');
}
if (globalThis.__venomRuntime.turboJsSnapshotPolicyVersion !== 1 || globalThis.__venomRuntime.turboJsSnapshotRecordsVersion !== 1 || globalThis.__venomRuntime.turboJsReplayValidationVersion !== 1 || globalThis.__venomRuntime.turboJsDeterminismLedgerVersion !== 1 || globalThis.__venomRuntime.turboJsAuditSealVersion !== 1) {
  throw new Error(`TurboJS snapshot/audit-seal metadata was not exposed: ${JSON.stringify({snapshotPolicy: globalThis.__venomRuntime.turboJsSnapshotPolicyVersion, snapshotRecords: globalThis.__venomRuntime.turboJsSnapshotRecordsVersion, replayValidation: globalThis.__venomRuntime.turboJsReplayValidationVersion, ledger: globalThis.__venomRuntime.turboJsDeterminismLedgerVersion, auditSeal: globalThis.__venomRuntime.turboJsAuditSealVersion})}`);
}
if (globalThis.__venomRuntime.turboJsExecutionCommitVersion !== 1 || globalThis.__venomRuntime.turboJsRollbackPolicyVersion !== 1 || globalThis.__venomRuntime.turboJsHostCallReceiptsVersion !== 1 || globalThis.__venomRuntime.turboJsReleaseAcceptanceVersion !== 1 || globalThis.__venomRuntime.turboJsCommitAuditVersion !== 1) {
  throw new Error(`TurboJS commit/rollback/release metadata was not exposed: ${JSON.stringify({commit: globalThis.__venomRuntime.turboJsExecutionCommitVersion, rollback: globalThis.__venomRuntime.turboJsRollbackPolicyVersion, receipts: globalThis.__venomRuntime.turboJsHostCallReceiptsVersion, acceptance: globalThis.__venomRuntime.turboJsReleaseAcceptanceVersion, audit: globalThis.__venomRuntime.turboJsCommitAuditVersion})}`);
}
if (typeof globalThis.__venomRuntime.turboJsSnapshotPolicy !== 'function' || typeof globalThis.__venomRuntime.turboJsAuditSeal !== 'function' || typeof globalThis.__venomRuntime.turboJsExecutionCommit !== 'function' || typeof globalThis.__venomRuntime.turboJsRollbackPolicy !== 'function' || typeof globalThis.__venomRuntime.turboJsHostCallReceipts !== 'function' || typeof globalThis.__venomRuntime.turboJsReleaseAcceptance !== 'function' || typeof globalThis.__venomRuntime.turboJsCommitAudit !== 'function') {
  throw new Error('TurboJS snapshot/commit/release helpers were not exposed through the host bridge');
}
if (globalThis.__venomRuntime.turboJsCapabilityPolicyVersion !== 1 || globalThis.__venomRuntime.turboJsHostIoPolicyVersion !== 1 || globalThis.__venomRuntime.turboJsPermissionSealVersion !== 1 || globalThis.__venomRuntime.turboJsPolicyReceiptsVersion !== 1 || globalThis.__venomRuntime.turboJsReleaseGateVersion !== 1) {
  throw new Error(`TurboJS capability/I-O/release-gate metadata was not exposed: ${JSON.stringify({capability: globalThis.__venomRuntime.turboJsCapabilityPolicyVersion, io: globalThis.__venomRuntime.turboJsHostIoPolicyVersion, seal: globalThis.__venomRuntime.turboJsPermissionSealVersion, receipts: globalThis.__venomRuntime.turboJsPolicyReceiptsVersion, gate: globalThis.__venomRuntime.turboJsReleaseGateVersion})}`);
}
if (typeof globalThis.__venomRuntime.turboJsCapabilityPolicy !== 'function' || typeof globalThis.__venomRuntime.turboJsHostIoPolicy !== 'function' || typeof globalThis.__venomRuntime.turboJsPermissionSeal !== 'function' || typeof globalThis.__venomRuntime.turboJsPolicyReceipts !== 'function' || typeof globalThis.__venomRuntime.turboJsReleaseGate !== 'function') {
  throw new Error('TurboJS capability/I-O/release-gate helpers were not exposed through the host bridge');
}
if (globalThis.__venomRuntime.turboJsHostIoDecisionVersion !== 1 || globalThis.__venomRuntime.turboJsHostIoDenyTraceVersion !== 1 || globalThis.__venomRuntime.turboJsCapabilityLedgerVersion !== 1 || globalThis.__venomRuntime.turboJsPolicySealAuditVersion !== 1 || globalThis.__venomRuntime.turboJsRuntimeDenylistVersion !== 1) {
  throw new Error(`TurboJS host I/O decision/deny/audit metadata was not exposed: ${JSON.stringify({decision: globalThis.__venomRuntime.turboJsHostIoDecisionVersion, deny: globalThis.__venomRuntime.turboJsHostIoDenyTraceVersion, ledger: globalThis.__venomRuntime.turboJsCapabilityLedgerVersion, audit: globalThis.__venomRuntime.turboJsPolicySealAuditVersion, denylist: globalThis.__venomRuntime.turboJsRuntimeDenylistVersion})}`);
}
if (typeof globalThis.__venomRuntime.turboJsHostIoDecision !== 'function' || typeof globalThis.__venomRuntime.turboJsHostIoDenyTrace !== 'function' || typeof globalThis.__venomRuntime.turboJsCapabilityLedger !== 'function' || typeof globalThis.__venomRuntime.turboJsPolicySealAudit !== 'function' || typeof globalThis.__venomRuntime.turboJsRuntimeDenylist !== 'function') {
  throw new Error('TurboJS host I/O decision/deny/audit helpers were not exposed through the host bridge');
}
const tjsConsoleEvents = globalThis.__venomRuntime.turboJsConsoleEvents();
if (!Array.isArray(tjsConsoleEvents) || tjsConsoleEvents.length < 1) {
  throw new Error(`TurboJS console bridge did not capture console events: ${JSON.stringify(tjsConsoleEvents)}`);
}

if (!globalThis.__venomRuntime || globalThis.__venomRuntime.fetchBridgeMode !== 'async-host-call' || globalThis.__venomRuntime.asyncHostQueueMode !== 'enabled') {
  throw new Error('Venom host bridge did not expose fetch/async queue mode');
}
if (globalThis.__venomRuntime.timerBridgeMode !== 'async-host-call' || globalThis.__venomRuntime.eventQueueMode !== 'enabled' || globalThis.__venomRuntime.turboJsBridgeMode !== 'engine-backend-select') {
  throw new Error('Venom host bridge did not expose timer/event/TurboJS bridge modes');
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
const tjsCall = globalThis.__venomRuntime.callTurboJs('route.main', { route: result.route });
if (!tjsCall || tjsCall.state !== 'pending' || tjsCall.mode !== 'engine-backend-select') {
  throw new Error(`Venom TurboJS bridge placeholder did not enqueue a call: ${JSON.stringify(tjsCall)}`);
}
globalThis.__venomRuntime.settleHostCall(tjsCall.id, 'fulfilled', { ok: true });

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
