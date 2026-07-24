export function createVenomTurboJsEngineModule(config = {}) {
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

