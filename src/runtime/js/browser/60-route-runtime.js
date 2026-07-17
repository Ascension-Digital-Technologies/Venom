function parseNumberLiteral(value) {
  const text = String(value || '').trim();
  if (/^0x[0-9a-f]+$/i.test(text)) return Number.parseInt(text.slice(2), 16) >>> 0;
  return Number.parseInt(text, 10) >>> 0;
}

function parseWordLayout(value) {
  const layout = String(value || '').split(',').map((item) => item.trim().toLowerCase()).filter(Boolean);
  if (layout.length !== 4) throw new Error('invalid polymorphic word layout');
  const seen = new Set(layout);
  for (const required of ['opcode', 'a', 'b', 'c']) {
    if (!seen.has(required)) throw new Error('polymorphic word layout is missing ' + required);
  }
  return layout;
}

function parseOperandMasks(value) {
  const parts = String(value || '').split(',').map((item) => parseNumberLiteral(item));
  if (parts.length !== 3) throw new Error('invalid polymorphic operand mask list');
  return parts;
}

function parsePolymorphicMap(section) {
  const data = section.data;
  requireAscii(data, 'VPOL0010', 'polymorphic VM plan');
  if (data.length < 72) throw new Error('polymorphic VM plan is truncated');
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const version = readU32(view, 8);
  if (version !== 2) throw new Error('unsupported polymorphic VM plan version: ' + version);
  const physicalToLogical = new Map();
  const logicalToPhysical = new Map();
  const opcodeMask = readU32(view, 20);
  const operandMasks = [readU32(view, 24), readU32(view, 28), readU32(view, 32)];
  const wordNames = ['opcode', 'a', 'b', 'c'];
  const wordLayout = [];
  for (let i = 0; i < 4; ++i) {
    const id = data[36 + i];
    if (id > 3) throw new Error('invalid polymorphic VM word id');
    wordLayout.push(wordNames[id]);
  }
  const count = readU32(view, 48);
  const mapBase = 72;
  if (mapBase + count * 4 > data.length) throw new Error('polymorphic VM opcode map is truncated');
  for (let i = 0; i < count; ++i) {
    const base = mapBase + i * 4;
    const logical = view.getUint16(base, true);
    const physical = view.getUint16(base + 2, true);
    physicalToLogical.set(physical, logical);
    logicalToPhysical.set(logical, physical);
  }
  if (!physicalToLogical.has(LOGICAL.CREATE_ELEMENT) && physicalToLogical.size === 0) {
    throw new Error('missing polymorphic opcode map');
  }
  return { physicalToLogical, logicalToPhysical, wordLayout, opcodeMask, operandMasks, source: '' };
}

function decodeVmInstruction(vm, pc, opcodeMap) {
  const words = [
    readU32(vm.view, pc + 0),
    readU32(vm.view, pc + 4),
    readU32(vm.view, pc + 8),
    readU32(vm.view, pc + 12),
  ];
  let physical = 0;
  let a = 0;
  let b = 0;
  let c = 0;
  for (let i = 0; i < opcodeMap.wordLayout.length; ++i) {
    const field = opcodeMap.wordLayout[i];
    const value = words[i] >>> 0;
    if (field === 'opcode') physical = (value ^ opcodeMap.opcodeMask) >>> 0;
    else if (field === 'a') a = (value ^ opcodeMap.operandMasks[0]) >>> 0;
    else if (field === 'b') b = (value ^ opcodeMap.operandMasks[1]) >>> 0;
    else if (field === 'c') c = (value ^ opcodeMap.operandMasks[2]) >>> 0;
    else throw new Error('unknown polymorphic instruction field: ' + field);
  }
  return { physical, a, b, c };
}

function normalizeRoute(pathname) {
  let route = pathname || '/';
  if (!route.startsWith('/')) route = '/' + route;
  if (route.length > 1 && route.endsWith('/')) route = route.slice(0, -1);
  if (route === '/index.html' || route === '/index.htm') return '/';
  if (route.length > 5 && route.endsWith('.html')) route = route.slice(0, -5);
  if (route.length > 4 && route.endsWith('.htm')) route = route.slice(0, -4);
  if (route.length > 1 && route.endsWith('/index')) route = route.slice(0, -6) || '/';
  return route;
}

function selectRoute(routes, pathname) {
  if (venomRouteResolver) return venomRouteResolver(pathname);
  const wanted = normalizeRoute(pathname);
  return routes.find((route) => route.route === wanted) ||
         routes.find((route) => route.route === '/') ||
         routes[0];
}

function createDomMutationBatch(maxOperations = 256, maxStringBytes = 65536) {
  const OP_SET_ATTRIBUTE = 1, OP_APPEND_CHILD = 2, OP_APPEND_TEXT = 3;
  const packet = new Uint32Array(Math.max(1, maxOperations) * 4);
  const nodes = [], nodeIds = new WeakMap(), strings = [];
  let operationCount = 0, stringBytes = 0, flushCount = 0;
  function nodeId(node) { let id = nodeIds.get(node); if (id !== undefined) return id; id = nodes.length; nodes.push(node); nodeIds.set(node, id); return id; }
  function stringId(value) { const text = String(value == null ? '' : value); const bytes = new TextEncoder().encode(text).byteLength; if (bytes > maxStringBytes) throw new Error('DOM batch string budget exceeded'); if (operationCount > 0 && stringBytes + bytes > maxStringBytes) flush(); const id = strings.length; strings.push(text); stringBytes += bytes; return id; }
  function emit(op, a, b, c) { if (operationCount >= maxOperations) flush(); const o = operationCount * 4; packet[o] = op >>> 0; packet[o+1] = a >>> 0; packet[o+2] = b >>> 0; packet[o+3] = c >>> 0; operationCount++; }
  function flush() { if (!operationCount) return 0; for (let i=0;i<operationCount;i++) { const o=i*4, op=packet[o], a=packet[o+1], b=packet[o+2], c=packet[o+3]; if (op===OP_SET_ATTRIBUTE) { const target=nodes[a]; if (!(target instanceof Element)) throw new Error('DOM batch attribute target denied'); target.setAttribute(strings[b], strings[c]); } else if (op===OP_APPEND_CHILD) { const parent=nodes[a], child=nodes[b]; if (!parent || typeof parent.appendChild !== 'function' || !child) throw new Error('DOM batch append target denied'); parent.appendChild(child); } else if (op===OP_APPEND_TEXT) { const parent=nodes[a]; if (!parent || typeof parent.appendChild !== 'function') throw new Error('DOM batch text target denied'); parent.appendChild(document.createTextNode(strings[b])); } else throw new Error('unknown DOM batch opcode'); } const flushed=operationCount; packet.fill(0,0,operationCount*4); operationCount=0; stringBytes=0; strings.length=0; flushCount++; return flushed; }
  return Object.freeze({ queueSetAttribute(node,name,value){emit(OP_SET_ATTRIBUTE,nodeId(node),stringId(name),stringId(value));}, queueAppendChild(parent,child){emit(OP_APPEND_CHILD,nodeId(parent),nodeId(child),0);}, queueAppendText(parent,value){emit(OP_APPEND_TEXT,nodeId(parent),stringId(value),0);}, flush, stats(){return Object.freeze({pending:operationCount,flushes:flushCount,nodes:nodes.length});} });
}

function appendPending(stack, pending, domBatch = null) {
  if (!pending) throw new Error('VM append_child without pending node');
  if (domBatch) domBatch.queueAppendChild(stack[stack.length - 1], pending);
  else stack[stack.length - 1].appendChild(pending);
}

function currentElement(stack) {
  const element = stack[stack.length - 1];
  if (!(element instanceof Element)) throw new Error('VM current node is not an element');
  return element;
}

function executeRoute(route, vm, strings, opcodeMap, root, assetManifest = new Map(), assetBaseUrl = document.baseURI, hostBridgePlan = null) {
  if (!route) throw new Error('no route available');
  const start = vm.dataOffset + route.bytecodeOffset;
  const end = start + route.bytecodeSize;
  if (start < vm.dataOffset || end > vm.data.length) throw new Error('route bytecode range is invalid');

  const stack = [root];
  let pending = null;
  let pc = start;
  let executed = 0;
  const domBatch = createDomMutationBatch(256, 65536);

  root.replaceChildren();
  root.setAttribute('data-venom-route', route.route);
  root.setAttribute('data-venom-runtime', 'vm');

  while (pc < end) {
    const decoded = decodeVmInstruction(vm, pc, opcodeMap);
    const { physical, a, b, c } = decoded;
    const op = opcodeMap.physicalToLogical.get(physical);
    pc += vm.instructionSize;
    executed++;

    if (op === undefined) throw new Error('unknown physical opcode: ' + physical);
    if (op === LOGICAL.NOP || op === LOGICAL.LOAD_CONST || op === LOGICAL.CALL_QUICKJS) {
      continue;
    }
    if (op === LOGICAL.CREATE_ELEMENT) {
      if (pending) appendPending(stack, pending, domBatch);
      pending = document.createElement(strings[a] || 'div');
      continue;
    }
    if (op === LOGICAL.ENTER_ELEMENT) {
      if (!pending) throw new Error('VM enter_element without pending element');
      stack.push(pending);
      pending = null;
      continue;
    }
    if (op === LOGICAL.SET_ATTRIBUTE) {
      const target = pending || currentElement(stack);
      if (!(target instanceof Element)) throw new Error('VM set_attribute target is not an element');
      const attrName = strings[a] || '';
      let attrValue = strings[b] || '';
      if (shouldRemapAttribute(attrName)) {
        attrValue = attrName === 'srcset' ? resolveSrcset(route, attrValue, assetManifest, assetBaseUrl) : resolveAssetValue(route, attrValue, assetManifest, assetBaseUrl);
      }
      if (isUnsafeDomAttribute(attrName, attrValue)) throw new Error('unsafe DOM attribute denied');
      domBatch.queueSetAttribute(target, attrName, attrValue);
      if (isInlineEventAttribute(attrName)) bindInlineEventAttribute(target, attrName, attrValue, hostBridgePlan);
      continue;
    }
    if (op === LOGICAL.SET_TEXT) {
      if (pending) domBatch.queueAppendText(pending, strings[a] || '');
      else domBatch.queueAppendText(stack[stack.length - 1], strings[a] || '');
      continue;
    }
    if (op === LOGICAL.LEAVE_ELEMENT) {
      if (stack.length <= 1) throw new Error('VM leave_element tried to pop root');
      if (pending) appendPending(stack, pending, domBatch);
      pending = stack.pop();
      continue;
    }
    if (op === LOGICAL.APPEND_CHILD) {
      appendPending(stack, pending, domBatch);
      pending = null;
      continue;
    }
    if (op === LOGICAL.HALT) {
      break;
    }
    throw new Error('unhandled logical opcode: ' + op + ' a=' + a + ' b=' + b + ' c=' + c);
  }

  if (pending) appendPending(stack, pending, domBatch);
  domBatch.flush();
  root.setAttribute('data-venom-dom-batches', String(domBatch.stats().flushes));
  root.setAttribute('data-venom-instructions', String(executed));
}

function installNavigation(routes, routeRuntimeLoader, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan, scriptIsolationPlan = null, scriptPolicyPlan = null, quickJsChunkPlan = null, quickJsEnginePlan = null, scriptEnginePolicyPlan = null) {
  document.addEventListener('click', (event) => {
    const anchor = event.target && event.target.closest ? event.target.closest('a[href]') : null;
    if (!anchor) return;
    const url = new URL(anchor.getAttribute('href'), location.href);
    if (url.origin !== location.origin) return;
    const targetRoute = selectRoute(routes, url.pathname);
    if (!targetRoute || normalizeRoute(url.pathname) !== targetRoute.route) return;
    event.preventDefault();
    history.pushState({ venomRoute: targetRoute.route }, '', url.pathname);
    try {
      const bridge = globalThis.__venomRuntime || null;
      if (bridge && typeof bridge.teardownRoute === 'function') bridge.teardownRoute('navigation');
      routeRuntimeLoader.dispose();
      const runtime = routeRuntimeLoader(targetRoute);
      if (bridge && typeof bridge.activateRouteGeneration === 'function') bridge.activateRouteGeneration(runtime.routeGeneration);
      setActiveBrowserAssetRoute(runtime.route);
      executeRoute(runtime.route, runtime.vm, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan);
      executeScriptsForRoute(runtime.route, runtime.jsBundle, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan).catch((error) => console.error('[venom] route script failed', error));
    } catch (error) {
      console.error('[venom] route load failed', error);
      throw error;
    }
  });

  window.addEventListener('popstate', () => {
    const targetRoute = selectRoute(routes, location.pathname);
    const bridge = globalThis.__venomRuntime || null;
    if (bridge && typeof bridge.teardownRoute === 'function') bridge.teardownRoute('history-navigation');
    routeRuntimeLoader.dispose();
    const runtime = routeRuntimeLoader(targetRoute);
    if (bridge && typeof bridge.activateRouteGeneration === 'function') bridge.activateRouteGeneration(runtime.routeGeneration);
    setActiveBrowserAssetRoute(runtime.route);
    executeRoute(runtime.route, runtime.vm, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan);
    executeScriptsForRoute(runtime.route, runtime.jsBundle, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan).catch((error) => console.error('[venom] route script failed', error));
  });
}




function parsePackageBindingMetadata(section) {
  const plan = parseKeyValueMetadata(section, 'VENOM_PACKAGE_BINDING_V1');
  const assets = new Map();
  for (const line of plan.lines) {
    if (!line.startsWith('asset\t')) continue;
    const parts = line.split('\t');
    if (parts.length !== 4) throw new Error('invalid package binding asset row');
    assets.set(parts[1], Object.freeze({ role: parts[1], name: parts[2], sha256: parts[3] }));
  }
  return Object.freeze({
    version: plan.version,
    enabled: plan.enabled,
    tokenHash: plan.values.get('binding_token_sha256') || '',
    hardened: plan.values.get('hardened') === 'true',
    assetCount: Number.parseInt(plan.values.get('asset_count') || String(assets.size), 10) >>> 0,
    assets,
  });
}

async function fetchBytesForBinding(url, label) {
  if (!url) throw new Error('missing bound asset URL for ' + label);
  const response = await fetch(url, { cache: 'force-cache' });
  if (!response || !response.ok) throw new Error('failed to fetch bound asset ' + label + ': ' + (response ? response.status : 'no-response'));
  return new Uint8Array(await response.arrayBuffer());
}

async function verifyBoundAsset(binding, role, url) {
  const expected = binding.assets.get(role);
  if (!expected) throw new Error('package binding is missing asset role: ' + role);
  const bytes = await fetchBytesForBinding(url, role);
  const actual = sha256Hex(bytes);
  if (actual !== expected.sha256) throw new Error('bound asset hash mismatch: ' + role);
  return true;
}

async function verifyPackageBinding(pkg, options) {
  const section = findOptionalSection(pkg, SECTION.INTEGRITY, 'package-binding.vbind');
  const hardenedFlag = (pkg.flags & PACKAGE_FLAG.RUNTIME_HARDENED) !== 0;
  if (!section) {
    if (hardenedFlag || (options && options.hardened)) throw new Error('hardened package is missing package binding metadata');
    return Object.freeze({ enabled: false, verified: false });
  }
  const binding = parsePackageBindingMetadata(section);
  if (!binding.enabled) throw new Error('package binding metadata is disabled');
  if (binding.hardened && !hardenedFlag) throw new Error('package binding expects runtime-hardened package');
  if (binding.tokenHash) {
    const token = options && options.bindingToken ? String(options.bindingToken) : '';
    if (!token && options && options.hardened) throw new Error('loader did not provide package binding token');
    if (token && sha256Hex(textEncoder.encode(token)) !== binding.tokenHash) throw new Error('loader/package binding token mismatch');
  }
  if (binding.assets.size !== binding.assetCount) throw new Error('package binding asset count mismatch');
  await verifyBoundAsset(binding, 'runtime', options && options.runtimeUrl ? options.runtimeUrl : import.meta.url);
  if (options && options.runtimeWasmUrl) await verifyBoundAsset(binding, 'runtime_wasm', options.runtimeWasmUrl);
  if (options && options.styleUrl) await verifyBoundAsset(binding, 'style', options.styleUrl);
  if (options && options.quickJsEngineUrl) await verifyBoundAsset(binding, 'quickjs_engine', options.quickJsEngineUrl);
  if (options && options.quickJsWasmUrl) await verifyBoundAsset(binding, 'quickjs_wasm', options.quickJsWasmUrl);
  if (options && options.workerUrl) await verifyBoundAsset(binding, 'worker', options.workerUrl);
  return Object.freeze({ enabled: true, verified: true, assets: binding.assets.size });
}

export function renderVenomFailure(root, error) {
  const message = error && error.message ? error.message : String(error || 'unknown error');
  if (!root) return;
  if (typeof root.replaceChildren === 'function' && document && typeof document.createElement === 'function') {
    const panel = document.createElement('div');
    panel.setAttribute('data-venom-failure', 'true');
    panel.setAttribute('role', 'alert');
    panel.textContent = 'Venom boot failed: ' + message;
    root.replaceChildren(panel);
    return;
  }
  root.textContent = 'Venom boot failed: ' + message;
}

export async function bootVenom(options) {
  const root = options.root || document.body;
  if (!root) throw new Error('missing Venom root element');
  const reportBootPhase = options && typeof options.onBootPhase === 'function' ? options.onBootPhase : () => {};
  const phaseStarted = new Map();
  const startPhase = (phase) => { phaseStarted.set(phase, performance.now()); reportBootPhase(Object.freeze({ phase, state: 'start', at: Date.now() })); };
  const finishPhase = (phase, detail) => { const started = phaseStarted.get(phase); const durationMs = started === undefined ? 0 : Math.max(0, performance.now() - started); reportBootPhase(Object.freeze({ phase, state: 'complete', at: Date.now(), durationMs, detail: detail || null })); };

  startPhase('package-load');
  const pkg = await loadPackage(options.packageUrl, options.expectedPackageHash, options.packageBytes || null);
  finishPhase('package-load', { sections: pkg.sections.length, decodedSections: pkg.decodedSectionCount });
  startPhase('package-policy');
  const packageFeatures = verifyPackageFeatures(pkg);
  const packageBinding = await verifyPackageBinding(pkg, options);
  const runtimePolicy = enforceRuntimePolicy(pkg, parseRuntimePolicy(findOptionalSection(pkg, SECTION.INTEGRITY, 'runtime-policy.vhrd')), options);
  activeReleaseDiversification = parseReleaseDiversification(findOptionalSection(pkg, SECTION.INTEGRITY, 'release-diversification.vrd3'), runtimePolicy.releaseProfile);
  activeQuickJsAbiFingerprint = parseQuickJsAbiFingerprint(findOptionalSection(pkg, SECTION.INTEGRITY, 'quickjs-abi-fingerprint.vqaf'), runtimePolicy.releaseProfile);
  const strings = parseStringTable(findSection(pkg, SECTION.STRINGS, 'strings.vstr'));
  const routes = parseRouteTable(findSection(pkg, SECTION.ROUTES, 'routes.vbrt'), strings);
  const lazySectionsPlan = parseLazySectionsMetadata(findOptionalSection(pkg, SECTION.INTEGRITY, 'lazy-sections.vlazy'));
  const routeRuntimeLoader = makeRouteRuntimeLoader(pkg, lazySectionsPlan, runtimePolicy.failClosed);
  const opcodeMap = parsePolymorphicMap(findSection(pkg, SECTION.INTEGRITY, 'vm-polymorph.vpol'));
  activeRuntimeOpcodeMap = opcodeMap;
  activeRuntimeIntegritySeal = computeRuntimeIntegritySeal(opcodeMap);
  assertRuntimeIntegrity(opcodeMap);
  const assetManifest = parseAssetManifest(findOptionalSection(pkg, SECTION.ASSET_MANIFEST, 'asset-manifest.txt'));
  const hostBridgePlan = parseHostBridgeMetadata(findOptionalSection(pkg, SECTION.HOST_BRIDGE, 'host-calls.vhcb'));
  const fetchBridgePlan = parseFetchBridgeMetadata(findOptionalSection(pkg, SECTION.FETCH_BRIDGE, 'fetch-bridge.vfch'));
  const timerBridgePlan = parseTimerBridgeMetadata(findOptionalSection(pkg, SECTION.TIMER_BRIDGE, 'timer-bridge.vtmr'));
  const eventQueuePlan = parseEventQueueMetadata(findOptionalSection(pkg, SECTION.EVENT_QUEUE, 'event-queue.vevq'));
  const quickJsBridgePlan = parseQuickJsBridgeMetadata(findOptionalSection(pkg, SECTION.QUICKJS_BRIDGE, 'quickjs-bridge.vqjs'));
  const scriptIsolationPlan = parseScriptIsolationMetadata(findOptionalSection(pkg, SECTION.SCRIPT_ISOLATION, 'script-isolation.vsis'));
  const scriptPolicyPlan = parseScriptPolicyMetadata(findOptionalSection(pkg, SECTION.SCRIPT_POLICY, 'script-policy.vscp'));
  const quickJsChunkPlan = parseQuickJsChunkMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CHUNKS, 'quickjs-chunks.vqbc'));
  const quickJsEnginePlan = parseQuickJsEngineMetadata(findOptionalSection(pkg, SECTION.QUICKJS_ENGINE, 'quickjs-engine.vqse'));
  const scriptEnginePolicyPlan = parseScriptEnginePolicyMetadata(findOptionalSection(pkg, SECTION.SCRIPT_ENGINE_POLICY, 'script-engine-policy.vsep'));
  const quickJsEngineModulePlan = parseQuickJsEngineModuleMetadata(findOptionalSection(pkg, SECTION.QUICKJS_ENGINE_MODULE, 'quickjs-engine-module.vqem'));
  const quickJsContextLifecyclePlan = parseQuickJsContextLifecycleMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CONTEXT_LIFECYCLE, 'quickjs-context-lifecycle.vqcl'));
  const hostCapabilitiesPlan = parseHostCapabilitiesMetadata(findOptionalSection(pkg, SECTION.HOST_CAPABILITIES, 'host-capabilities.vhcap'));
  const quickJsAdapterDiagnosticsPlan = parseQuickJsAdapterDiagnosticsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_ADAPTER_DIAGNOSTICS, 'quickjs-adapter-diagnostics.vqad'));
  const quickJsWasmRuntimePlan = parseQuickJsWasmRuntimeMetadata(findOptionalSection(pkg, SECTION.QUICKJS_WASM_RUNTIME, 'quickjs-wasm-runtime.vqwr'));
  const quickJsSourceTransferPlan = parseQuickJsSourceTransferMetadata(findOptionalSection(pkg, SECTION.QUICKJS_SOURCE_TRANSFER, 'quickjs-source-transfer.vqst'));
  const quickJsConsoleBridgePlan = parseQuickJsConsoleBridgeMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CONSOLE_BRIDGE, 'quickjs-console-bridge.vqcb'));
  const quickJsExecutionRecordsPlan = parseQuickJsExecutionRecordsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EXECUTION_RECORDS, 'quickjs-execution-records.vqer'));
  const quickJsResultBridgePlan = parseQuickJsResultBridgeMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RESULT_BRIDGE, 'quickjs-result-bridge.vqrb'));
  const quickJsFallbackPolicyPlan = parseQuickJsFallbackPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_FALLBACK_POLICY, 'quickjs-fallback-policy.vqfp'));
  const quickJsRuntimeAbiPlan = parseQuickJsRuntimeAbiMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RUNTIME_ABI, 'quickjs-runtime-abi.vqra'));
  const quickJsHostImportsPlan = parseQuickJsHostImportsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_IMPORTS, 'quickjs-host-imports.vqhi'));
  const quickJsHeapLimitsPlan = parseQuickJsHeapLimitsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HEAP_LIMITS, 'quickjs-heap-limits.vqhl'));
  const quickJsScriptBufferPlan = parseQuickJsScriptBufferMetadata(findOptionalSection(pkg, SECTION.QUICKJS_SCRIPT_BUFFER, 'quickjs-script-buffer.vqsb'));
  const quickJsConsoleAbiPlan = parseQuickJsConsoleAbiMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CONSOLE_ABI, 'quickjs-console-abi.vqca'));
  const quickJsParityProbePlan = parseQuickJsParityProbeMetadata(findOptionalSection(pkg, SECTION.QUICKJS_PARITY_PROBE, 'quickjs-parity-probe.vqpp'));
  const quickJsReleaseFallbackPlan = parseQuickJsReleaseFallbackMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RELEASE_FALLBACK, 'quickjs-release-fallback.vqrf'));
  const quickJsBytecodeManifestPlan = parseQuickJsBytecodeManifestMetadata(findOptionalSection(pkg, SECTION.QUICKJS_BYTECODE_MANIFEST, 'quickjs-bytecode-manifest.vqbm'));
  const quickJsModuleResolverPlan = parseQuickJsModuleResolverMetadata(findOptionalSection(pkg, SECTION.QUICKJS_MODULE_RESOLVER, 'quickjs-module-resolver.vqmr'));
  const quickJsExceptionAbiPlan = parseQuickJsExceptionAbiMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EXCEPTION_ABI, 'quickjs-exception-abi.vqex'));
  const quickJsHostTrapPolicyPlan = parseQuickJsHostTrapPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_TRAP_POLICY, 'quickjs-host-trap-policy.vqtp'));
  const quickJsExecutionLifecyclePlan = parseQuickJsExecutionLifecycleMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EXECUTION_LIFECYCLE, 'quickjs-execution-lifecycle.vqel'));
  const quickJsScriptBufferPolicyPlan = parseQuickJsScriptBufferPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_SCRIPT_BUFFER_POLICY, 'quickjs-script-buffer-policy.vqsp'));
  const quickJsContextLimitPolicyPlan = parseQuickJsContextLimitPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CONTEXT_LIMIT_POLICY, 'quickjs-context-limit-policy.vqlp'));
  const quickJsHostCallDispatchPlan = parseQuickJsHostCallDispatchMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_CALL_DISPATCH, 'quickjs-host-call-dispatch.vqhd'));
  const quickJsParityContractPlan = parseQuickJsParityContractMetadata(findOptionalSection(pkg, SECTION.QUICKJS_PARITY_CONTRACT, 'quickjs-parity-contract.vqpc'));
  const quickJsReleaseFailClosedPlan = parseQuickJsReleaseFailClosedMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RELEASE_FAILCLOSED, 'quickjs-release-failclosed.vqfc'));
  const quickJsModuleGraphPlan = parseQuickJsModuleGraphMetadata(findOptionalSection(pkg, SECTION.QUICKJS_MODULE_GRAPH, 'quickjs-module-graph.vqmg'));
  const quickJsModuleExecutionPlan = parseQuickJsModuleExecutionMetadata(findOptionalSection(pkg, SECTION.QUICKJS_MODULE_EXECUTION, 'quickjs-module-execution.vqmx'));
  const quickJsModuleCachePlan = parseQuickJsModuleCacheMetadata(findOptionalSection(pkg, SECTION.QUICKJS_MODULE_CACHE, 'quickjs-module-cache.vqmc'));
  const quickJsResolverAuditPlan = parseQuickJsResolverAuditMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RESOLVER_AUDIT, 'quickjs-resolver-audit.vqra'));
  const quickJsInteropFallbackPlan = parseQuickJsInteropFallbackMetadata(findOptionalSection(pkg, SECTION.QUICKJS_INTEROP_FALLBACK, 'quickjs-interop-fallback.vqif'));
  const quickJsExecutionPipelinePlan = parseQuickJsExecutionPipelineMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EXECUTION_PIPELINE, 'quickjs-execution-pipeline.vqxp'));
  const quickJsScriptRecordsPlan = parseQuickJsScriptRecordsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_SCRIPT_RECORDS, 'quickjs-script-records.vqsr'));
  const quickJsEvalResultsPlan = parseQuickJsEvalResultsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EVAL_RESULTS, 'quickjs-eval-results.vqev'));
  const quickJsConsoleCapturePlan = parseQuickJsConsoleCaptureMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CONSOLE_CAPTURE, 'quickjs-console-capture.vqcc'));
  const quickJsFailureReportsPlan = parseQuickJsFailureReportsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_FAILURE_REPORTS, 'quickjs-failure-reports.vqfr'));
  const quickJsExecutionJournalPlan = parseQuickJsExecutionJournalMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EXECUTION_JOURNAL, 'quickjs-execution-journal.vqxj'));
  const quickJsCheckpointPolicyPlan = parseQuickJsCheckpointPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CHECKPOINT_POLICY, 'quickjs-checkpoint-policy.vqcp'));
  const quickJsReplayCursorPlan = parseQuickJsReplayCursorMetadata(findOptionalSection(pkg, SECTION.QUICKJS_REPLAY_CURSOR, 'quickjs-replay-cursor.vqrc'));
  const quickJsResumeStatePlan = parseQuickJsResumeStateMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RESUME_STATE, 'quickjs-resume-state.vqrs'));
  const quickJsDeterminismAuditPlan = parseQuickJsDeterminismAuditMetadata(findOptionalSection(pkg, SECTION.QUICKJS_DETERMINISM_AUDIT, 'quickjs-determinism-audit.vqda'));
  const quickJsSnapshotPolicyPlan = parseQuickJsSnapshotPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_SNAPSHOT_POLICY, 'quickjs-snapshot-policy.vqsk'));
  const quickJsSnapshotRecordsPlan = parseQuickJsSnapshotRecordsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_SNAPSHOT_RECORDS, 'quickjs-snapshot-records.vqsn'));
  const quickJsReplayValidationPlan = parseQuickJsReplayValidationMetadata(findOptionalSection(pkg, SECTION.QUICKJS_REPLAY_VALIDATION, 'quickjs-replay-validation.vqrv'));
  const quickJsDeterminismLedgerPlan = parseQuickJsDeterminismLedgerMetadata(findOptionalSection(pkg, SECTION.QUICKJS_DETERMINISM_LEDGER, 'quickjs-determinism-ledger.vqdl'));
  const quickJsAuditSealPlan = parseQuickJsAuditSealMetadata(findOptionalSection(pkg, SECTION.QUICKJS_AUDIT_SEAL, 'quickjs-audit-seal.vqas'));
  const quickJsExecutionCommitPlan = parseQuickJsExecutionCommitMetadata(findOptionalSection(pkg, SECTION.QUICKJS_EXECUTION_COMMIT, 'quickjs-execution-commit.vqxc'));
  const quickJsRollbackPolicyPlan = parseQuickJsRollbackPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_ROLLBACK_POLICY, 'quickjs-rollback-policy.vqrp'));
  const quickJsHostCallReceiptsPlan = parseQuickJsHostCallReceiptsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_CALL_RECEIPTS, 'quickjs-host-call-receipts.vqhr'));
  const quickJsReleaseAcceptancePlan = parseQuickJsReleaseAcceptanceMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RELEASE_ACCEPTANCE, 'quickjs-release-acceptance.vqac'));
  const quickJsCommitAuditPlan = parseQuickJsCommitAuditMetadata(findOptionalSection(pkg, SECTION.QUICKJS_COMMIT_AUDIT, 'quickjs-commit-audit.vqca'));
  const quickJsCapabilityPolicyPlan = parseQuickJsCapabilityPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CAPABILITY_POLICY, 'quickjs-capability-policy.vqcpol'));
  const quickJsHostIoPolicyPlan = parseQuickJsHostIoPolicyMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_IO_POLICY, 'quickjs-host-io-policy.vqio'));
  const quickJsPermissionSealPlan = parseQuickJsPermissionSealMetadata(findOptionalSection(pkg, SECTION.QUICKJS_PERMISSION_SEAL, 'quickjs-permission-seal.vqps'));
  const quickJsPolicyReceiptsPlan = parseQuickJsPolicyReceiptsMetadata(findOptionalSection(pkg, SECTION.QUICKJS_POLICY_RECEIPTS, 'quickjs-policy-receipts.vqpr'));
  const quickJsReleaseGatePlan = parseQuickJsReleaseGateMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RELEASE_GATE, 'quickjs-release-gate.vqrg')); 
  const quickJsHostIoDecisionPlan = parseQuickJsHostIoDecisionMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_IO_DECISION, 'quickjs-host-io-decision.vqid'));
  const quickJsHostIoDenyTracePlan = parseQuickJsHostIoDenyTraceMetadata(findOptionalSection(pkg, SECTION.QUICKJS_HOST_IO_DENY_TRACE, 'quickjs-host-io-deny-trace.vqdt'));
  const quickJsCapabilityLedgerPlan = parseQuickJsCapabilityLedgerMetadata(findOptionalSection(pkg, SECTION.QUICKJS_CAPABILITY_LEDGER, 'quickjs-capability-ledger.vqclg'));
  const quickJsPolicySealAuditPlan = parseQuickJsPolicySealAuditMetadata(findOptionalSection(pkg, SECTION.QUICKJS_POLICY_SEAL_AUDIT, 'quickjs-policy-seal-audit.vqpsa'));
  const quickJsRuntimeDenylistPlan = parseQuickJsRuntimeDenylistMetadata(findOptionalSection(pkg, SECTION.QUICKJS_RUNTIME_DENYLIST, 'quickjs-runtime-denylist.vqrd'));
  const asyncQueuePlan = parseAsyncHostQueueMetadata(findOptionalSection(pkg, SECTION.ASYNC_HOST_QUEUE, 'async-host-queue.vahq'));
  const assetBaseUrl = options.assetBaseUrl || new URL('./', options.packageUrl || document.baseURI).href;
  const quickJsEngineModuleUrl = options.quickJsEngineUrl || (quickJsEngineModulePlan && quickJsEngineModulePlan.assetName ? new URL(quickJsEngineModulePlan.assetName, assetBaseUrl).href : null);
  const quickJsWasmUrl = options.quickJsWasmUrl || (quickJsWasmRuntimePlan && quickJsWasmRuntimePlan.assetName ? new URL(quickJsWasmRuntimePlan.assetName, assetBaseUrl).href : null);
  const route = selectRoute(routes, location.pathname);
  finishPhase('package-policy', { route: route.route, runtimeAbi: pkg.runtimeAbi, failClosed: runtimePolicy.failClosed });

  startPhase('runtime-install');
  installVenomHostBridge(root, pkg, routes, assetManifest, runtimePolicy, hostBridgePlan, fetchBridgePlan, asyncQueuePlan, timerBridgePlan, eventQueuePlan, quickJsBridgePlan, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan, quickJsEngineModulePlan, quickJsEngineModuleUrl, quickJsContextLifecyclePlan, hostCapabilitiesPlan, quickJsAdapterDiagnosticsPlan, quickJsWasmRuntimePlan, quickJsWasmUrl, quickJsSourceTransferPlan, quickJsConsoleBridgePlan, quickJsExecutionRecordsPlan, quickJsResultBridgePlan, quickJsFallbackPolicyPlan, quickJsRuntimeAbiPlan, quickJsHostImportsPlan, quickJsHeapLimitsPlan, quickJsScriptBufferPlan, quickJsConsoleAbiPlan, quickJsParityProbePlan, quickJsReleaseFallbackPlan, quickJsBytecodeManifestPlan, quickJsModuleResolverPlan, quickJsExceptionAbiPlan, quickJsHostTrapPolicyPlan, quickJsExecutionLifecyclePlan, quickJsScriptBufferPolicyPlan, quickJsContextLimitPolicyPlan, quickJsHostCallDispatchPlan, quickJsParityContractPlan, quickJsReleaseFailClosedPlan, quickJsModuleGraphPlan, quickJsModuleExecutionPlan, quickJsModuleCachePlan, quickJsResolverAuditPlan, quickJsInteropFallbackPlan, quickJsExecutionJournalPlan, quickJsCheckpointPolicyPlan, quickJsReplayCursorPlan, quickJsResumeStatePlan, quickJsDeterminismAuditPlan, quickJsSnapshotPolicyPlan, quickJsSnapshotRecordsPlan, quickJsReplayValidationPlan, quickJsDeterminismLedgerPlan, quickJsAuditSealPlan, quickJsExecutionCommitPlan, quickJsRollbackPolicyPlan, quickJsHostCallReceiptsPlan, quickJsReleaseAcceptancePlan, quickJsCommitAuditPlan, quickJsCapabilityPolicyPlan, quickJsHostIoPolicyPlan, quickJsPermissionSealPlan, quickJsPolicyReceiptsPlan, quickJsReleaseGatePlan, quickJsHostIoDecisionPlan, quickJsHostIoDenyTracePlan, quickJsCapabilityLedgerPlan, quickJsPolicySealAuditPlan, quickJsRuntimeDenylistPlan);
  finishPhase('runtime-install', { protectedRuntime: quickJsWasmRuntimePlan.enabled, hostCapabilities: hostCapabilitiesPlan.capabilities.length });
  installBrowserAssetResolver(route, assetManifest, assetBaseUrl);
  startPhase('route-decode');
  const initialRuntime = routeRuntimeLoader(route);
  finishPhase('route-decode', { lazy: initialRuntime.lazy, modules: initialRuntime.jsBundle.chunks.length });
  const initialBridge = globalThis.__venomRuntime || null;
  if (initialBridge && typeof initialBridge.activateRouteGeneration === 'function') initialBridge.activateRouteGeneration(initialRuntime.routeGeneration);
  startPhase('route-render');
  executeRoute(initialRuntime.route, initialRuntime.vm, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan);
  finishPhase('route-render', { route: initialRuntime.route.route });
  startPhase('script-execution');
  const executedScripts = await executeScriptsForRoute(initialRuntime.route, initialRuntime.jsBundle, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan);
  finishPhase('script-execution', { executedScripts: executedScripts.length });
  startPhase('navigation-install');
  installNavigation(routes, routeRuntimeLoader, strings, opcodeMap, root, assetManifest, assetBaseUrl, hostBridgePlan, scriptIsolationPlan, scriptPolicyPlan, quickJsChunkPlan, quickJsEnginePlan, scriptEnginePolicyPlan);
  finishPhase('navigation-install', { routes: routes.length });

  return {
    packageVersion: pkg.version,
    packageFeatures,
    packageBindingVerified: packageBinding.verified,
    packageBindingAssets: packageBinding.assets || 0,
    runtimeAbi: pkg.runtimeAbi,
    sections: pkg.sections.length,
    assets: assetManifest.size,
    scripts: initialRuntime.jsBundle.chunks.length,
    executedScripts: executedScripts.length,
    route: initialRuntime.route.route,
    lazySections: lazySectionsPlan.enabled,
    lazyRouteSections: lazySectionsPlan.routeCount,
    lazyScriptSections: lazySectionsPlan.scriptRouteCount,
    initialRouteLazy: initialRuntime.lazy,
    decodedSectionsAtBoot: pkg.decodedSectionCount,
    runtimeHardened: runtimePolicy.runtimeHardened,
    failClosed: runtimePolicy.failClosed,
    runtimeMode: 'js',
    hostBridgeVersion: hostBridgePlan.version,
    hostCallCount: hostBridgePlan.hostCalls.size,
    fetchBridgeVersion: fetchBridgePlan.version,
    fetchBridgeMode: fetchBridgePlan.enabled ? 'async-host-call' : 'none',
    asyncHostQueueVersion: asyncQueuePlan.version,
    asyncHostQueueMode: asyncQueuePlan.enabled ? 'enabled' : 'none',
    timerBridgeVersion: timerBridgePlan.version,
    timerBridgeMode: timerBridgePlan.enabled ? 'async-host-call' : 'none',
    eventQueueVersion: eventQueuePlan.version,
    eventQueueMode: eventQueuePlan.enabled ? 'enabled' : 'none',
    quickJsBridgeVersion: quickJsBridgePlan.version,
    quickJsBridgeMode: quickJsBridgePlan.mode,
    scriptIsolationVersion: scriptIsolationPlan.version,
    scriptIsolationMode: scriptIsolationPlan.mode,
    scriptPolicyVersion: scriptPolicyPlan.version,
    scriptPolicyMode: 'capability-metadata',
    quickJsChunkVersion: quickJsChunkPlan.version,
    quickJsChunkMode: quickJsChunkPlan.mode,
    quickJsChunkCount: quickJsChunkPlan.chunkCount,
    quickJsEngineVersion: quickJsEnginePlan.version,
    quickJsEngineMode: quickJsEnginePlan.mode,
    quickJsEngineEnabled: quickJsEnginePlan.enabled,
    scriptEnginePolicyVersion: scriptEnginePolicyPlan.version,
    scriptEngineFallback: scriptEnginePolicyPlan.fallback,
    quickJsEngineModuleVersion: quickJsEngineModulePlan.version,
    quickJsEngineModuleMode: quickJsEngineModulePlan.enabled ? quickJsEngineModulePlan.executionMode : 'none',
    quickJsEngineModuleLoaded: !!quickJsEngineModuleUrl,
    quickJsContextLifecycleVersion: quickJsContextLifecyclePlan.version,
    quickJsContextLifecycleMode: quickJsContextLifecyclePlan.enabled ? quickJsContextLifecyclePlan.model : 'none',
    hostCapabilitiesVersion: hostCapabilitiesPlan.version,
    hostCapabilityCount: hostCapabilitiesPlan.capabilities.length,
    quickJsAdapterDiagnosticsVersion: quickJsAdapterDiagnosticsPlan.version,
    quickJsAdapterDiagnosticsMode: quickJsAdapterDiagnosticsPlan.enabled ? 'enabled' : 'none',
    quickJsSnapshotPolicyVersion: quickJsSnapshotPolicyPlan.version,
    quickJsSnapshotRecordsVersion: quickJsSnapshotRecordsPlan.version,
    quickJsReplayValidationVersion: quickJsReplayValidationPlan.version,
    quickJsDeterminismLedgerVersion: quickJsDeterminismLedgerPlan.version,
    quickJsAuditSealVersion: quickJsAuditSealPlan.version,
    quickJsExecutionCommitVersion: quickJsExecutionCommitPlan.version,
    quickJsRollbackPolicyVersion: quickJsRollbackPolicyPlan.version,
    quickJsHostCallReceiptsVersion: quickJsHostCallReceiptsPlan.version,
    quickJsReleaseAcceptanceVersion: quickJsReleaseAcceptancePlan.version,
    quickJsCommitAuditVersion: quickJsCommitAuditPlan.version,
    quickJsCapabilityPolicyVersion: quickJsCapabilityPolicyPlan.version,
    quickJsHostIoPolicyVersion: quickJsHostIoPolicyPlan.version,
    quickJsPermissionSealVersion: quickJsPermissionSealPlan.version,
    quickJsPolicyReceiptsVersion: quickJsPolicyReceiptsPlan.version,
    quickJsReleaseGateVersion: quickJsReleaseGatePlan.version,
    quickJsHostIoDecisionVersion: quickJsHostIoDecisionPlan.version,
    quickJsHostIoDenyTraceVersion: quickJsHostIoDenyTracePlan.version,
    quickJsCapabilityLedgerVersion: quickJsCapabilityLedgerPlan.version,
    quickJsPolicySealAuditVersion: quickJsPolicySealAuditPlan.version,
    quickJsRuntimeDenylistVersion: quickJsRuntimeDenylistPlan.version,
    quickJsWasmRuntimeVersion: quickJsWasmRuntimePlan.version,
    quickJsWasmRuntimeMode: quickJsWasmRuntimePlan.enabled ? quickJsWasmRuntimePlan.executionMode : 'none',
    quickJsWasmRuntimeLoaded: !!quickJsWasmUrl,
    quickJsSourceTransferVersion: quickJsSourceTransferPlan.version,
    quickJsSourceTransferMode: quickJsSourceTransferPlan.enabled ? 'enabled' : 'none',
    quickJsConsoleBridgeVersion: quickJsConsoleBridgePlan.version,
    quickJsConsoleBridgeMode: quickJsConsoleBridgePlan.enabled ? quickJsConsoleBridgePlan.mode : 'none',
    quickJsExecutionRecordsVersion: quickJsExecutionRecordsPlan.version,
    quickJsExecutionRecordsMode: quickJsExecutionRecordsPlan.enabled ? quickJsExecutionRecordsPlan.retention : 'none',
    quickJsResultBridgeVersion: quickJsResultBridgePlan.version,
    quickJsResultBridgeMode: quickJsResultBridgePlan.enabled ? quickJsResultBridgePlan.format : 'none',
    quickJsFallbackPolicyVersion: quickJsFallbackPolicyPlan.version,
    quickJsFallbackPolicyMode: quickJsFallbackPolicyPlan.enabled ? quickJsFallbackPolicyPlan.mode : 'none',
    quickJsRuntimeAbi: quickJsRuntimeAbiPlan.abi,
    quickJsHostImportCount: quickJsHostImportsPlan.importCount,
    quickJsBytecodeManifestVersion: quickJsBytecodeManifestPlan.version,
    quickJsBytecodeManifestMode: quickJsBytecodeManifestPlan.enabled ? quickJsBytecodeManifestPlan.format : 'none',
    quickJsModuleResolverVersion: quickJsModuleResolverPlan.version,
    quickJsModuleResolverMode: quickJsModuleResolverPlan.enabled ? quickJsModuleResolverPlan.mode : 'none',
    quickJsExceptionAbi: quickJsExceptionAbiPlan.abi,
    quickJsExceptionAbiVersion: quickJsExceptionAbiPlan.version,
    quickJsHostTrapPolicy: quickJsHostTrapPolicyPlan.policy,
    quickJsHostTrapPolicyVersion: quickJsHostTrapPolicyPlan.version,
    quickJsExecutionLifecycleVersion: quickJsExecutionLifecyclePlan.version,
    quickJsExecutionLifecycleStates: quickJsExecutionLifecyclePlan.states,
    quickJsScriptBufferPolicyVersion: quickJsScriptBufferPolicyPlan.version,
    quickJsContextLimitPolicyVersion: quickJsContextLimitPolicyPlan.version,
    quickJsHostCallDispatchVersion: quickJsHostCallDispatchPlan.version,
    quickJsHostCallDispatchCount: quickJsHostCallDispatchPlan.entryCount,
    quickJsParityContractVersion: quickJsParityContractPlan.version,
    quickJsReleaseFailClosedVersion: quickJsReleaseFailClosedPlan.version,
    quickJsModuleGraphVersion: quickJsModuleGraphPlan.version,
    quickJsModuleGraphCount: quickJsModuleGraphPlan.moduleCount,
    quickJsModuleExecutionVersion: quickJsModuleExecutionPlan.version,
    quickJsModuleExecutionMode: quickJsModuleExecutionPlan.enabled ? quickJsModuleExecutionPlan.mode : 'none',
    quickJsModuleCacheVersion: quickJsModuleCachePlan.version,
    quickJsResolverAuditVersion: quickJsResolverAuditPlan.version,
    quickJsInteropFallbackVersion: quickJsInteropFallbackPlan.version,
    quickJsExecutionPipelineVersion: quickJsExecutionPipelinePlan.version,
    quickJsScriptRecordsVersion: quickJsScriptRecordsPlan.version,
    quickJsEvalResultsVersion: quickJsEvalResultsPlan.version,
    quickJsConsoleCaptureVersion: quickJsConsoleCapturePlan.version,
    quickJsFailureReportsVersion: quickJsFailureReportsPlan.version,
    quickJsExecutionJournalVersion: quickJsExecutionJournalPlan.version,
    quickJsCheckpointPolicyVersion: quickJsCheckpointPolicyPlan.version,
    quickJsReplayCursorVersion: quickJsReplayCursorPlan.version,
    quickJsResumeStateVersion: quickJsResumeStatePlan.version,
    quickJsDeterminismAuditVersion: quickJsDeterminismAuditPlan.version,
    routes: routes.map((item) => item.route),
  };
}
