function createQuickJsEngine(asyncQueue, quickJsEnginePlan, scriptEnginePolicyPlan, quickJsBridgePlan, quickJsEngineModulePlan = null, quickJsEngineModuleUrl = null, quickJsContextLifecyclePlan = null, hostCapabilitiesPlan = null, quickJsAdapterDiagnosticsPlan = null, quickJsWasmRuntimePlan = null, quickJsWasmUrl = null, quickJsSourceTransferPlan = null, quickJsConsoleBridgePlan = null, quickJsExecutionRecordsPlan = null, quickJsResultBridgePlan = null, quickJsFallbackPolicyPlan = null, quickJsRuntimeAbiPlan = null, quickJsHostImportsPlan = null, quickJsHeapLimitsPlan = null, quickJsScriptBufferPlan = null, quickJsConsoleAbiPlan = null, quickJsParityProbePlan = null, quickJsReleaseFallbackPlan = null, quickJsBytecodeManifestPlan = null, quickJsModuleResolverPlan = null, quickJsExceptionAbiPlan = null, quickJsHostTrapPolicyPlan = null, quickJsExecutionLifecyclePlan = null, quickJsScriptBufferPolicyPlan = null, quickJsContextLimitPolicyPlan = null, quickJsHostCallDispatchPlan = null, quickJsParityContractPlan = null, quickJsReleaseFailClosedPlan = null, quickJsModuleGraphPlan = null, quickJsModuleExecutionPlan = null, quickJsModuleCachePlan = null, quickJsResolverAuditPlan = null, quickJsInteropFallbackPlan = null, quickJsExecutionJournalPlan = null, quickJsCheckpointPolicyPlan = null, quickJsReplayCursorPlan = null, quickJsResumeStatePlan = null, quickJsDeterminismAuditPlan = null, quickJsSnapshotPolicyPlan = null, quickJsSnapshotRecordsPlan = null, quickJsReplayValidationPlan = null, quickJsDeterminismLedgerPlan = null, quickJsAuditSealPlan = null, quickJsExecutionCommitPlan = null, quickJsRollbackPolicyPlan = null, quickJsHostCallReceiptsPlan = null, quickJsReleaseAcceptancePlan = null, quickJsCommitAuditPlan = null, quickJsCapabilityPolicyPlan = null, quickJsHostIoPolicyPlan = null, quickJsPermissionSealPlan = null, quickJsPolicyReceiptsPlan = null, quickJsReleaseGatePlan = null, quickJsHostIoDecisionPlan = null, quickJsHostIoDenyTracePlan = null, quickJsCapabilityLedgerPlan = null, quickJsPolicySealAuditPlan = null, quickJsRuntimeDenylistPlan = null) {
  const contexts = new Map();
  const enabled = !!(quickJsEnginePlan && quickJsEnginePlan.enabled);
  const mode = enabled ? (quickJsEnginePlan.mode || 'bootstrap') : 'disabled';
  let engineModulePromise = null;
  let engineModule = null;
  let engineModuleError = null;
  const executionRecords = [];
  const consoleEvents = [];
  const cachedSnapshotPolicy = null;
  const cachedSnapshotRecord = null;
  const cachedReplayValidation = null;
  const cachedDeterminismLedger = null;
  const cachedAuditSeal = null;

  function recordQuickJsExecution(kind, payload = {}) {
    const record = Object.freeze({ id: executionRecords.length + 1, kind: String(kind || 'quickjs.executionRecord'), at: Date.now ? Date.now() : 0, ...payload });
    executionRecords.push(record);
    if (asyncQueue && quickJsExecutionRecordsPlan && quickJsExecutionRecordsPlan.enabled) {
      const request = asyncQueue.enqueue(quickJsExecutionRecordsPlan.recordHostCall || 'quickjs.executionRecord', record);
      asyncQueue.settle(request.id, 'fulfilled', { recorded: record.id, kind: record.kind });
    }
    return record;
  }

  function recordResultBridge(payload = {}) {
    const record = Object.freeze({ id: executionRecords.length + 1, kind: 'quickjs.resultBridge', at: Date.now ? Date.now() : 0, ...payload });
    executionRecords.push(record);
    if (asyncQueue && quickJsResultBridgePlan && quickJsResultBridgePlan.enabled) {
      const request = asyncQueue.enqueue(quickJsResultBridgePlan.resultHostCall || 'quickjs.resultBridge', record);
      asyncQueue.settle(request.id, 'fulfilled', { bridged: record.id, ok: payload.ok !== false });
    }
    return record;
  }

  function makeConsoleBridgeConsole(baseConsole, chunk) {
    const source = chunk && chunk.source ? String(chunk.source) : '';
    const route = chunk && chunk.route ? String(chunk.route) : '';
    const order = chunk && chunk.order ? chunk.order >>> 0 : 0;
    const out = Object.create(null);
    for (const level of ['debug', 'log', 'info', 'warn', 'error']) {
      out[level] = (...args) => {
        const event = Object.freeze({ id: consoleEvents.length + 1, level, route, source, order, args: args.map((arg) => { try { return String(arg); } catch (_) { return '[unprintable]'; } }) });
        consoleEvents.push(event);
        if (asyncQueue && quickJsResultBridgePlan && quickJsResultBridgePlan.enabled) {
          const request = asyncQueue.enqueue(quickJsResultBridgePlan.consoleEventHostCall || 'quickjs.consoleEvent', event);
          asyncQueue.settle(request.id, 'fulfilled', { consoleEvent: event.id });
        }
        if (baseConsole && typeof baseConsole[level] === 'function') baseConsole[level](...args);
      };
    }
    return Object.freeze(out);
  }

  function fallbackAllowed(reason) {
    const mode = quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.enabled ? quickJsFallbackPolicyPlan.mode : 'legacy';
    const denyList = String((quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.denyWhen) || '') + '|' + String((quickJsReleaseFallbackPlan && quickJsReleaseFallbackPlan.denyWhen) || '');
    const strict = !!((quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.strictRelease) || (quickJsReleaseFallbackPlan && quickJsReleaseFallbackPlan.enabled && quickJsReleaseFallbackPlan.denyHostFallback));
    const wasmOrModuleFallback = reason === 'engine-module-unavailable-or-compatible-fallback' || reason === 'wasm-interpreter-unavailable' || reason === 'release-strict-no-fallback';
    const allowed = !(strict && wasmOrModuleFallback) && !denyList.split('|').filter(Boolean).includes(reason);
    if (asyncQueue && quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.enabled) {
      const request = asyncQueue.enqueue(quickJsFallbackPolicyPlan.policyCheckHostCall || 'quickjs.fallbackPolicyCheck', { reason, mode, strict, allowed });
      asyncQueue.settle(request.id, allowed ? 'fulfilled' : 'rejected', { allowed, reason, strict });
    }
    return allowed;
  }

  function contextKey(route, source, order, generation = asyncQueue.snapshot().generation) {
    return String(generation >>> 0) + '::' + String(route || '/') + '::' + String(source || '') + '::' + String(order >>> 0);
  }

  function createContext(route, source, order) {
    const key = contextKey(route, source, order);
    let ctx = contexts.get(key);
    if (!ctx) {
      const request = asyncQueue.enqueue(quickJsContextLifecyclePlan && quickJsContextLifecyclePlan.createHostCall ? quickJsContextLifecyclePlan.createHostCall : (quickJsEnginePlan && quickJsEnginePlan.contextCreateHostCall ? quickJsEnginePlan.contextCreateHostCall : 'quickjs.contextCreate'), { route: String(route || '/'), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(request.id, 'fulfilled', { context: key, mode, engineModule: !!quickJsEngineModuleUrl });
      ctx = Object.freeze({ key, generation: asyncQueue.snapshot().generation >>> 0, route: String(route || '/'), source: String(source || ''), order: order >>> 0, mode, createdAt: Date.now ? Date.now() : 0, reuseCount: 0 });
      contexts.set(key, ctx);
    } else if (quickJsContextLifecyclePlan && quickJsContextLifecyclePlan.enabled) {
      const request = asyncQueue.enqueue(quickJsContextLifecyclePlan.reuseHostCall || 'quickjs.contextReuse', { context: key, route: String(route || '/'), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(request.id, 'fulfilled', { context: key, reused: true });
    }
    return ctx;
  }

  function destroyContext(route, source, order) {
    const key = contextKey(route, source, order);
    const existed = contexts.delete(key);
    const request = asyncQueue.enqueue(quickJsContextLifecyclePlan && quickJsContextLifecyclePlan.destroyHostCall ? quickJsContextLifecyclePlan.destroyHostCall : 'quickjs.contextDestroy', { context: key, existed });
    asyncQueue.settle(request.id, 'fulfilled', { context: key, destroyed: existed });
    return Object.freeze({ context: key, destroyed: existed });
  }

  function destroyRoute(route) {
    const routeMarker = '::' + String(route || '/') + '::';
    let destroyed = 0;
    for (const key of Array.from(contexts.keys())) {
      if (!key.includes(routeMarker)) continue;
      contexts.delete(key);
      destroyed++;
    }
    return Object.freeze({ route: String(route || '/'), destroyed });
  }

  function contextSnapshot() {
    return Object.freeze({ count: contexts.size, keys: Object.freeze(Array.from(contexts.keys())), moduleLoaded: !!engineModule, moduleError: engineModuleError ? String(engineModuleError.message || engineModuleError) : '', wasmUrl: quickJsWasmUrl || '' });
  }

  function recordBrowserApiShimCall(kind, chunk, detail) {
    const payload = Object.freeze({
      kind: String(kind || 'unknown'),
      route: chunk && chunk.route ? String(chunk.route) : '',
      source: chunk && chunk.source ? String(chunk.source) : '',
      order: chunk && chunk.order ? chunk.order >>> 0 : 0,
      ...(detail || {}),
    });
    if (asyncQueue && quickJsResultBridgePlan && quickJsResultBridgePlan.enabled) {
      const request = asyncQueue.enqueue('quickjs.browserApiShim', payload);
      asyncQueue.settle(request.id, 'fulfilled', payload);
    }
    return payload;
  }

  const guardedMissingGlobalNames = Object.freeze(['fpCollect']);

  function createGuardedMissingGlobalFunction(name, nativeGlobal, bridge, chunk) {
    const shim = function venomMissingGlobalFunctionShim(...args) {
      const target = nativeGlobal || globalThis;
      let nativeFn = null;
      try { nativeFn = target && target[name]; } catch (_) { nativeFn = null; }
      if (typeof nativeFn !== 'function') { try { nativeFn = target && target.window && target.window[name]; } catch (_) { nativeFn = null; } }
      if (typeof nativeFn !== 'function') { try { nativeFn = target && target.self && target.self[name]; } catch (_) { nativeFn = null; } }
      if (typeof nativeFn === 'function' && nativeFn !== shim) {
        try {
          const result = nativeFn.apply(target, args);
          recordBrowserApiShimCall('global.' + name + '.forwarded', chunk, { api: name, actualArgs: args.length, result: 'forwarded' });
          return result;
        } catch (error) {
          recordBrowserApiShimCall('global.' + name + '.trapped', chunk, { api: name, actualArgs: args.length, result: null, message: error && error.message ? String(error.message) : String(error) });
          return null;
        }
      }
      recordBrowserApiShimCall('global.' + name + '.missing', chunk, { api: name, actualArgs: args.length, result: null });
      return null;
    };
    try { Object.defineProperty(shim, 'name', { value: 'venom_' + name + '_shim' }); } catch (_) {}
    return shim;
  }

  function createGuardedMissingGlobalShims(nativeGlobal, bridge, chunk) {
    const shims = Object.create(null);
    for (const name of guardedMissingGlobalNames) shims[name] = createGuardedMissingGlobalFunction(name, nativeGlobal, bridge, chunk);
    return Object.freeze(shims);
  }

  function getGuardedMissingGlobalShim(missingGlobalShims, prop) {
    const key = String(prop);
    return missingGlobalShims && Object.prototype.hasOwnProperty.call(missingGlobalShims, key) ? missingGlobalShims[key] : undefined;
  }

  function createNavigatorShim(nativeNavigator, bridge, chunk) {
    const target = nativeNavigator || Object.create(null);
    const mediaDeviceTarget = target && target.mediaDevices ? target.mediaDevices : Object.create(null);
    const legacyMediaAliases = ['getUserMedia', 'webkitGetUserMedia', 'mozGetUserMedia', 'msGetUserMedia'];
    function makeMediaError(api, message) {
      try {
        const error = new TypeError(message);
        error.name = 'TypeError';
        error.api = api;
        return error;
      } catch (_) {
        return { name: 'TypeError', api, message };
      }
    }
    function settleMediaErrorCallback(callback, error) {
      if (typeof callback !== 'function') return;
      try { callback(error); } catch (callbackError) {
        recordBrowserApiShimCall('navigator.getUserMedia.errorCallbackTrapped', chunk, { api: 'navigator.getUserMedia', result: false, message: callbackError && callbackError.message ? String(callbackError.message) : String(callbackError) });
      }
    }
    function findLegacyGetUserMedia(obj) {
      for (const name of legacyMediaAliases) {
        try {
          if (obj && typeof obj[name] === 'function') return { name, fn: obj[name] };
        } catch (_) {}
      }
      return { name: 'getUserMedia', fn: null };
    }
    function safeResolvedNullPromise() {
      return typeof Promise !== 'undefined' && Promise && typeof Promise.resolve === 'function' ? Promise.resolve(null) : null;
    }
    const sendBeaconShim = function venomSendBeaconShim(url, data) {
      if (arguments.length < 1) {
        recordBrowserApiShimCall('navigator.sendBeacon.arity', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: 0, result: false });
        return false;
      }
      const nativeSendBeacon = target && target.sendBeacon;
      if (typeof nativeSendBeacon !== 'function') {
        recordBrowserApiShimCall('navigator.sendBeacon.unavailable', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: arguments.length, result: false });
        return false;
      }
      try {
        const result = arguments.length > 1 ? nativeSendBeacon.call(target, url, data) : nativeSendBeacon.call(target, url);
        recordBrowserApiShimCall('navigator.sendBeacon.forwarded', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: arguments.length, result: !!result });
        return !!result;
      } catch (error) {
        recordBrowserApiShimCall('navigator.sendBeacon.trapped', chunk, { api: 'navigator.sendBeacon', requiredArgs: 1, actualArgs: arguments.length, result: false, message: error && error.message ? String(error.message) : String(error) });
        return false;
      }
    };
    const mediaDevicesGetUserMediaShim = function venomMediaDevicesGetUserMediaShim(constraints) {
      const api = 'navigator.mediaDevices.getUserMedia';
      if (arguments.length < 1 || constraints == null) {
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.arity', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null });
        return safeResolvedNullPromise();
      }
      const nativeMediaGetUserMedia = mediaDeviceTarget && mediaDeviceTarget.getUserMedia;
      if (typeof nativeMediaGetUserMedia !== 'function') {
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.unavailable', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null });
        return safeResolvedNullPromise();
      }
      try {
        const result = nativeMediaGetUserMedia.call(mediaDeviceTarget, constraints);
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.forwarded', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: 'promise' });
        return result && typeof result.then === 'function'
          ? result.catch((error) => {
              recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.trapped', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null, message: error && error.message ? String(error.message) : String(error) });
              return null;
            })
          : Promise.resolve(result || null);
      } catch (error) {
        recordBrowserApiShimCall('navigator.mediaDevices.getUserMedia.trapped', chunk, { api, requiredArgs: 1, actualArgs: arguments.length, result: null, message: error && error.message ? String(error.message) : String(error) });
        return safeResolvedNullPromise();
      }
    };
    function createMediaDevicesShim(nativeMediaDevices) {
      const mediaTarget = nativeMediaDevices || Object.create(null);
      if (typeof Proxy === 'undefined') {
        const shim = Object.create(mediaTarget || null);
        shim.getUserMedia = mediaDevicesGetUserMediaShim;
        return shim;
      }
      return new Proxy(mediaTarget, {
        get(obj, prop) {
          if (prop === 'getUserMedia') return mediaDevicesGetUserMediaShim;
          try {
            const value = obj ? obj[prop] : undefined;
            return typeof value === 'function' ? value.bind(obj) : value;
          } catch (_) {
            return undefined;
          }
        },
        has(obj, prop) {
          return prop === 'getUserMedia' || !!(obj && prop in obj);
        },
        set(obj, prop, value) {
          try { obj[prop] = value; return true; } catch (_) { return false; }
        },
      });
    }
    const mediaDevicesShim = createMediaDevicesShim(mediaDeviceTarget);
    const legacyGetUserMediaShim = function venomGetUserMediaShim(constraints, successCallback, errorCallback) {
      const api = 'navigator.getUserMedia';
      if (arguments.length < 3 || typeof successCallback !== 'function' || typeof errorCallback !== 'function') {
        recordBrowserApiShimCall('navigator.getUserMedia.arity', chunk, { api, requiredArgs: 3, actualArgs: arguments.length, result: false });
        settleMediaErrorCallback(errorCallback, makeMediaError(api, 'Venom guarded getUserMedia shim rejected missing constraints or callbacks'));
        return false;
      }
      const nativeLegacy = findLegacyGetUserMedia(target);
      if (typeof nativeLegacy.fn !== 'function') {
        recordBrowserApiShimCall('navigator.getUserMedia.unavailable', chunk, { api: nativeLegacy.name, requiredArgs: 3, actualArgs: arguments.length, result: false });
        settleMediaErrorCallback(errorCallback, makeMediaError(api, 'getUserMedia is unavailable in this runtime'));
        return false;
      }
      try {
        const result = nativeLegacy.fn.call(target, constraints, successCallback, errorCallback);
        recordBrowserApiShimCall('navigator.getUserMedia.forwarded', chunk, { api: nativeLegacy.name, requiredArgs: 3, actualArgs: arguments.length, result: true });
        return result;
      } catch (error) {
        recordBrowserApiShimCall('navigator.getUserMedia.trapped', chunk, { api: nativeLegacy.name, requiredArgs: 3, actualArgs: arguments.length, result: false, message: error && error.message ? String(error.message) : String(error) });
        settleMediaErrorCallback(errorCallback, error);
        return false;
      }
    };
    if (typeof Proxy === 'undefined') {
      const shim = Object.create(target || null);
      shim.sendBeacon = sendBeaconShim;
      shim.mediaDevices = mediaDevicesShim;
      for (const name of legacyMediaAliases) shim[name] = legacyGetUserMediaShim;
      return shim;
    }
    return new Proxy(target, {
      get(obj, prop) {
        if (prop === 'sendBeacon') return sendBeaconShim;
        if (prop === 'mediaDevices') return mediaDevicesShim;
        if (legacyMediaAliases.indexOf(String(prop)) !== -1) return legacyGetUserMediaShim;
        try {
          const value = obj ? obj[prop] : undefined;
          return typeof value === 'function' ? value.bind(obj) : value;
        } catch (_) {
          return undefined;
        }
      },
      has(obj, prop) {
        return prop === 'sendBeacon' || prop === 'mediaDevices' || legacyMediaAliases.indexOf(String(prop)) !== -1 || !!(obj && prop in obj);
      },
      set(obj, prop, value) {
        try { obj[prop] = value; return true; } catch (_) { return false; }
      },
    });
  }

  function createGlobalShim(nativeGlobal, navigatorShim, windowShim, missingGlobalShims) {
    const target = nativeGlobal || globalThis;
    if (typeof Proxy === 'undefined') return target;
    let proxy = null;
    proxy = new Proxy(target, {
      get(obj, prop) {
        if (prop === 'navigator') return navigatorShim;
        if (prop === 'window' || prop === 'self') return windowShim;
        if (prop === 'globalThis') return proxy;
        const missingShim = getGuardedMissingGlobalShim(missingGlobalShims, prop);
        if (missingShim) return missingShim;
        try {
          const value = obj ? obj[prop] : undefined;
          return typeof value === 'function' ? value.bind(obj) : value;
        } catch (_) {
          return undefined;
        }
      },
      has(obj, prop) {
        return prop === 'navigator' || prop === 'window' || prop === 'self' || prop === 'globalThis' || !!getGuardedMissingGlobalShim(missingGlobalShims, prop) || !!(obj && prop in obj);
      },
      set(obj, prop, value) {
        try { obj[prop] = value; return true; } catch (_) { return false; }
      },
    });
    return proxy;
  }

  function createWindowShim(nativeWindow, navigatorShim, missingGlobalShims) {
    const target = nativeWindow || globalThis;
    if (typeof Proxy === 'undefined') return target;
    let proxy = null;
    proxy = new Proxy(target, {
      get(obj, prop) {
        if (prop === 'navigator') return navigatorShim;
        if (prop === 'window' || prop === 'self' || prop === 'globalThis') return proxy;
        const missingShim = getGuardedMissingGlobalShim(missingGlobalShims, prop);
        if (missingShim) return missingShim;
        try {
          const value = obj ? obj[prop] : undefined;
          return typeof value === 'function' ? value.bind(obj) : value;
        } catch (_) {
          return undefined;
        }
      },
      has(obj, prop) {
        return prop === 'navigator' || prop === 'window' || prop === 'self' || prop === 'globalThis' || !!getGuardedMissingGlobalShim(missingGlobalShims, prop) || !!(obj && prop in obj);
      },
      set(obj, prop, value) {
        try { obj[prop] = value; return true; } catch (_) { return false; }
      },
    });
    return proxy;
  }


  function sourceDeclaresInjectedBinding(source, name) {
    const id = String(name || '');
    if (!/^[A-Za-z_$][\w$]*$/.test(id)) return false;
    const pattern = new RegExp('(^|[^A-Za-z0-9_$])(?:const|let|class|function)\\s+' + id + '\\b');
    return pattern.test(String(source || ''));
  }

  function capabilitySetForChunk(chunk) {
    if (!hostCapabilitiesPlan || !hostCapabilitiesPlan.enabled) return null;
    const records = Array.isArray(hostCapabilitiesPlan.chunkCapabilities) ? hostCapabilitiesPlan.chunkCapabilities : [];
    const order = chunk && chunk.order !== undefined ? chunk.order >>> 0 : 0;
    const route = chunk && chunk.route ? String(chunk.route) : '';
    const source = chunk && chunk.source ? String(chunk.source) : '';
    let record = records.find((item) => (item.order >>> 0) === order && (!item.route || item.route === route) && (!item.source || item.source === source));
    if (!record) record = records.find((item) => (item.order >>> 0) === order);
    const names = record && Array.isArray(record.capabilities) ? record.capabilities : hostCapabilitiesPlan.defaultCapabilities;
    return new Set(Array.isArray(names) ? names : []);
  }

  function capabilityDenied(name, chunk) {
    const source = chunk && chunk.source ? String(chunk.source) : '<unknown>';
    const error = new Error('VNM-CAP-1001: undeclared host capability "' + String(name) + '" for ' + source);
    error.name = 'VenomCapabilityError';
    error.code = 'VNM-CAP-1001';
    error.capability = String(name);
    return error;
  }

  function makeScopedRuntimeBridge(bridge, allowed, chunk) {
    if (!bridge) return Object.freeze({});
    const facade = Object.create(null);
    for (const name of ['version', 'packageVersion', 'runtimeHardened', 'failClosed', 'currentRouteGeneration', 'isRouteGenerationActive']) {
      const value = bridge[name];
      if (typeof value === 'function') facade[name] = value.bind(bridge);
      else if (value !== undefined) facade[name] = value;
    }
    const expose = (capability, names) => {
      if (!allowed || allowed.has(capability)) {
        for (const name of names) if (typeof bridge[name] === 'function') facade[name] = bridge[name].bind(bridge);
      }
    };
    expose('fetch', ['fetch', 'requestFetch']);
    expose('timers', ['scheduleTimer', 'cancelTimer']);
    expose('events', ['eventQueueSnapshot', 'dispatchEventBinding']);
    expose('document', ['domRootHandle', 'domHandleSnapshot', 'registerDomHandle', 'resolveDomHandle', 'revokeDomHandle', 'domSetAttribute', 'domAppendChild']);
    Object.defineProperty(facade, 'requireCapability', {
      value(name) {
        if (allowed && !allowed.has(String(name))) throw capabilityDenied(name, chunk);
        return true;
      },
      enumerable: false,
    });
    return Object.freeze(facade);
  }

  function makeBindings(bridge, chunk) {
    const bindings = Object.create(null);
    const allowed = capabilitySetForChunk(chunk);
    const permits = (name) => !allowed || allowed.has(name);
    const moduleEnabled = (name) => typeof __venomRuntimeModuleEnabled === 'function' && __venomRuntimeModuleEnabled(name);
    const exposeGlobal = (name, moduleName) => {
      if (!moduleEnabled(moduleName)) return;
      const value = globalThis[name];
      if (value !== undefined) bindings[name] = value;
    };
    const navigatorShim = createNavigatorShim(globalThis.navigator, bridge, chunk);
    const missingGlobalShims = createGuardedMissingGlobalShims(globalThis, bridge, chunk);
    const windowShim = createWindowShim(globalThis.window || globalThis, navigatorShim, missingGlobalShims);
    const globalShim = createGlobalShim(globalThis, navigatorShim, windowShim, missingGlobalShims);
    if (permits('window')) {
      bindings.globalThis = globalShim;
      bindings.window = windowShim;
      bindings.self = windowShim;
      for (const name of guardedMissingGlobalNames) bindings[name] = missingGlobalShims[name];
    }
    if (permits('navigator')) bindings.navigator = navigatorShim;
    if (permits('document')) bindings.document = globalThis.document;
    if (permits('__venomRuntime')) bindings.__venomRuntime = makeScopedRuntimeBridge(bridge, allowed, chunk);
    if (permits('console')) bindings.console = quickJsConsoleBridgePlan && quickJsConsoleBridgePlan.enabled ? makeConsoleBridgeConsole(globalThis.console, chunk) : globalThis.console;
    if (permits('fetch')) bindings.fetch = bridge && typeof bridge.fetch === 'function' ? bridge.fetch.bind(bridge) : globalThis.fetch;
    if (permits('timers')) {
      bindings.setTimeout = (callback, delay) => bridge && typeof bridge.scheduleTimer === 'function' ? bridge.scheduleTimer(callback, delay, false).id : globalThis.setTimeout(callback, delay);
      bindings.setInterval = (callback, delay) => bridge && typeof bridge.scheduleTimer === 'function' ? bridge.scheduleTimer(callback, delay, true).id : globalThis.setInterval(callback, delay);
      bindings.clearTimeout = (id) => bridge && typeof bridge.cancelTimer === 'function' ? bridge.cancelTimer(id) : globalThis.clearTimeout(id);
      bindings.clearInterval = (id) => bridge && typeof bridge.cancelTimer === 'function' ? bridge.cancelTimer(id) : globalThis.clearInterval(id);
    }
    if (permits('timers') && moduleEnabled('animation')) {
      bindings.requestAnimationFrame = typeof globalThis.requestAnimationFrame === 'function' ? globalThis.requestAnimationFrame.bind(globalThis) : (callback) => bindings.setTimeout(() => callback(Date.now()), 16);
      bindings.cancelAnimationFrame = typeof globalThis.cancelAnimationFrame === 'function' ? globalThis.cancelAnimationFrame.bind(globalThis) : bindings.clearTimeout;
    }
    if (permits('events')) for (const name of ['Event', 'CustomEvent']) exposeGlobal(name, 'events');
    for (const name of ['FormData', 'SubmitEvent']) exposeGlobal(name, 'forms');
    for (const name of ['MutationObserver', 'ResizeObserver', 'IntersectionObserver']) exposeGlobal(name, 'observers');
    if (permits('fetch')) for (const name of ['Headers', 'Request', 'Response', 'Blob', 'File', 'FileReader', 'URL', 'URLSearchParams', 'AbortController', 'XMLHttpRequest', 'WebSocket', 'EventSource']) exposeGlobal(name, 'network');
    bindings.__venomChunk = Object.freeze({ route: chunk.route, source: chunk.source, order: chunk.order, flags: chunk.flags });
    if (hostCapabilitiesPlan && hostCapabilitiesPlan.enabled) {
      const capabilityNames = allowed ? Array.from(allowed) : (hostCapabilitiesPlan.capabilities && hostCapabilitiesPlan.capabilities.length ? hostCapabilitiesPlan.capabilities.map((item) => item.name) : Object.keys(bindings));
      if (bridge && typeof bridge.enqueueHostCall === 'function') {
        const record = bridge.enqueueHostCall(hostCapabilitiesPlan.injectHostCall || 'quickjs.hostCapabilityInject', { route: chunk.route, source: chunk.source, order: chunk.order, capabilities: capabilityNames });
        if (bridge && typeof bridge.settleHostCall === 'function') bridge.settleHostCall(record.id, 'fulfilled', { injected: capabilityNames.length });
      }
      bindings.__venomCapabilities = Object.freeze(capabilityNames.slice());
    }
    return Object.freeze(bindings);
  }

  async function loadEngineModule() {
    if (!quickJsEngineModulePlan || !quickJsEngineModulePlan.enabled || !quickJsEngineModuleUrl) return null;
    if (engineModule) return engineModule;
    if (engineModuleError) return null;
    if (!engineModulePromise) {
      engineModulePromise = import(quickJsEngineModuleUrl).then((mod) => {
        const factoryName = quickJsEngineModulePlan.exportName || 'createVenomQuickJsEngineModule';
        const factory = mod && mod[factoryName];
        if (typeof factory !== 'function') throw new Error('QuickJS engine module missing export: ' + factoryName);
        engineModule = factory({
          version: quickJsEngineModulePlan.version,
          mode: quickJsEngineModulePlan.executionMode,
          fallback: quickJsEngineModulePlan.fallback,
          policy: scriptEnginePolicyPlan,
          bridge: quickJsBridgePlan,
          contextLifecycle: quickJsContextLifecyclePlan,
          hostCapabilities: hostCapabilitiesPlan,
          diagnostics: quickJsAdapterDiagnosticsPlan,
          wasmRuntime: quickJsWasmRuntimePlan,
          wasmUrl: quickJsWasmUrl,
          diversification: activeReleaseDiversification,
          abiFingerprint: activeQuickJsAbiFingerprint,
          sourceTransfer: quickJsSourceTransferPlan,
          consoleBridge: quickJsConsoleBridgePlan,
          executionRecords: quickJsExecutionRecordsPlan,
          resultBridge: quickJsResultBridgePlan,
          fallbackPolicy: quickJsFallbackPolicyPlan,
          runtimeAbi: quickJsRuntimeAbiPlan,
          hostImports: quickJsHostImportsPlan,
          heapLimits: quickJsHeapLimitsPlan,
          scriptBuffer: quickJsScriptBufferPlan,
          consoleAbi: quickJsConsoleAbiPlan,
          parityProbe: quickJsParityProbePlan,
          releaseFallback: quickJsReleaseFallbackPlan,
          bytecodeManifest: quickJsBytecodeManifestPlan,
          moduleResolver: quickJsModuleResolverPlan,
          exceptionAbi: quickJsExceptionAbiPlan,
          hostTrapPolicy: quickJsHostTrapPolicyPlan,
          executionLifecycle: quickJsExecutionLifecyclePlan,
          scriptBufferPolicy: quickJsScriptBufferPolicyPlan,
          contextLimitPolicy: quickJsContextLimitPolicyPlan,
          hostCallDispatch: quickJsHostCallDispatchPlan,
          parityContract: quickJsParityContractPlan,
          releaseFailClosed: quickJsReleaseFailClosedPlan,
          moduleGraph: quickJsModuleGraphPlan,
          moduleExecution: quickJsModuleExecutionPlan,
          moduleCache: quickJsModuleCachePlan,
          resolverAudit: quickJsResolverAuditPlan,
          interopFallback: quickJsInteropFallbackPlan,
          executionJournal: quickJsExecutionJournalPlan,
          checkpointPolicy: quickJsCheckpointPolicyPlan,
          replayCursor: quickJsReplayCursorPlan,
          resumeState: quickJsResumeStatePlan,
          determinismAudit: quickJsDeterminismAuditPlan,
          snapshotPolicy: quickJsSnapshotPolicyPlan,
          snapshotRecords: quickJsSnapshotRecordsPlan,
          replayValidation: quickJsReplayValidationPlan,
          determinismLedger: quickJsDeterminismLedgerPlan,
          auditSeal: quickJsAuditSealPlan,
          executionCommit: quickJsExecutionCommitPlan,
          rollbackPolicy: quickJsRollbackPolicyPlan,
          hostCallReceipts: quickJsHostCallReceiptsPlan,
          releaseAcceptance: quickJsReleaseAcceptancePlan,
          commitAudit: quickJsCommitAuditPlan,
        });
        if (!engineModule || typeof engineModule.executeChunk !== 'function') throw new Error('QuickJS engine module did not return executeChunk');
        return engineModule;
      }).catch((error) => {
        engineModuleError = error;
        return null;
      });
    }
    return engineModulePromise;
  }

  async function executeViaEngineModule(chunk, route, bridge, context, bindings) {
    const module = await loadEngineModule();
    if (!module) return null;
    if (module && typeof module.createContext === 'function') {
      const ctxRecord = asyncQueue.enqueue(quickJsContextLifecyclePlan && quickJsContextLifecyclePlan.moduleCreateHostCall ? quickJsContextLifecyclePlan.moduleCreateHostCall : 'quickjs.moduleContextCreate', { context: context.key, route: chunk.route, source: chunk.source, order: chunk.order });
      const created = module.createContext(context);
      asyncQueue.settle(ctxRecord.id, 'fulfilled', { context: context.key, moduleContext: !!created });
    }
    const moduleRequest = asyncQueue.enqueue('quickjs.moduleExecute', {
      route: chunk.route,
      source: chunk.source,
      order: chunk.order,
      bytes: chunk.bytecodeSize || (chunk.code ? chunk.code.length : 0),
      moduleUrl: String(quickJsEngineModuleUrl || ''),
    });
    try {
      const result = await module.executeChunk({ chunk, route, context, bindings, wasmUrl: quickJsWasmUrl, wasmRuntime: quickJsWasmRuntimePlan, sourceTransfer: quickJsSourceTransferPlan, consoleBridge: quickJsConsoleBridgePlan, executionRecords: quickJsExecutionRecordsPlan, resultBridge: quickJsResultBridgePlan, fallbackPolicy: quickJsFallbackPolicyPlan });
      recordQuickJsExecution('quickjs.executionRecord', { route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, engine: result && result.quickJsWasm ? 'quickjs-wasm-backend-candidate' : 'host-js-fallback', fallbackUsed: !!(result && result.fallback), wasmAccepted: !!(result && result.quickJsWasm), hostBridgeTelemetry: result && result.hostBridgeTelemetry ? result.hostBridgeTelemetry : null, hostBridgeParity: result && result.hostBridgeParity ? result.hostBridgeParity : null });
      recordResultBridge({ route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, ok: !!(result && result.executed), wasmReport: result && result.wasmReport ? result.wasmReport : null, hostBridgeTelemetry: result && result.hostBridgeTelemetry ? result.hostBridgeTelemetry : null });
      asyncQueue.settle(moduleRequest.id, 'fulfilled', { executed: !!(result && result.executed), engineModule: true });
      return Object.freeze({
        ...chunk,
        ...(result || {}),
        executed: !!(result && result.executed),
        engineMode: quickJsEngineModulePlan.executionMode || 'route-scoped-module',
        engineModule: true,
        context: context.key,
      });
    } catch (error) {
      asyncQueue.settle(moduleRequest.id, 'rejected', { message: error && error.message ? error.message : String(error), engineModule: true });
      throw error;
    }
  }

  async function executeChunk(chunk, route, bridge) {
    const context = createContext(route && route.route ? route.route : chunk.route, chunk.source, chunk.order);
    const executionGeneration = context.generation >>> 0;
    const request = asyncQueue.enqueue(quickJsEnginePlan && quickJsEnginePlan.executeHostCall ? quickJsEnginePlan.executeHostCall : 'quickjs.executeChunk', { route: chunk.route, source: chunk.source, order: chunk.order, bytes: chunk.bytecodeSize || (chunk.code ? chunk.code.length : 0), bytecode: !!chunk.bytecode, mode, engineModule: !!quickJsEngineModuleUrl });
    try {
      if (!enabled) throw new Error('QuickJS engine bootstrap is disabled');
      if ((!chunk.code || !String(chunk.code).trim()) && !(chunk.bytecodeBytes && chunk.bytecodeBytes.length)) {
        asyncQueue.settle(request.id, 'fulfilled', { executed: false, empty: true, context: context.key });
        return Object.freeze({ ...chunk, executed: false, engineMode: mode, context: context.key });
      }
      if ((chunk.flags & SCRIPT_FLAG.REMOTE_URL) !== 0) {
        const executed = await injectRemoteScript(chunk);
        asyncQueue.settle(request.id, 'fulfilled', { executed: true, remote: true, context: context.key });
        return Object.freeze({ ...executed, executed: true, engineMode: mode, context: context.key, remote: true });
      }
      const bindings = makeBindings(bridge, chunk);
      const moduleExecuted = await executeViaEngineModule(chunk, route, bridge, context, bindings);
      if (executionGeneration !== (asyncQueue.snapshot().generation >>> 0)) throw new Error('stale route generation rejected for QuickJS execution');
      if (moduleExecuted) {
        asyncQueue.settle(request.id, 'fulfilled', { executed: true, context: context.key, engineModule: true });
        return moduleExecuted;
      }
      if ((chunk.flags & SCRIPT_FLAG.MODULE) !== 0 && typeof URL !== 'undefined' && typeof Blob !== 'undefined' && document.createElement) {
        const blob = new Blob([chunk.code + '\n//# sourceURL=venom://' + safeSourceUrl(chunk.source)], { type: 'text/javascript' });
        const url = URL.createObjectURL(blob);
        try {
          const executed = await injectRemoteScript({ ...chunk, source: url, flags: (chunk.flags | SCRIPT_FLAG.REMOTE_URL) });
          asyncQueue.settle(request.id, 'fulfilled', { executed: true, module: true, context: context.key });
          return Object.freeze({ ...executed, executed: true, engineMode: mode, context: context.key, module: true });
        } finally {
          URL.revokeObjectURL(url);
        }
      }
      if (!fallbackAllowed('engine-module-unavailable-or-compatible-fallback')) throw new Error('QuickJS fallback policy denied host execution');
      if (asyncQueue && quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.enabled) { const decision = asyncQueue.enqueue(quickJsFallbackPolicyPlan.decisionHostCall || 'script.fallbackDecision', { route: chunk.route, source: chunk.source, order: chunk.order, allowed: true }); asyncQueue.settle(decision.id, 'fulfilled', { allowed: true }); }
__VENOM_RUNTIME_ENGINE_FALLBACK_BLOCK__
    } catch (error) {
      if (request && request.id && executionGeneration === (asyncQueue.snapshot().generation >>> 0)) asyncQueue.settle(request.id, 'rejected', { message: error && error.message ? error.message : String(error), context: context.key });
      throw error;
    }
  }

  function abiTable() {
    if (engineModule && typeof engineModule.abiTable === 'function') return engineModule.abiTable();
    return Object.freeze({ abi: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.abi : 0, packageVersion: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.packageVersion : 0, entryCount: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.entryCount : 0, tableHash: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.tableHash : '', table: quickJsRuntimeAbiPlan ? quickJsRuntimeAbiPlan.table : '' });
  }

  function hostImportTable() {
    if (engineModule && typeof engineModule.hostImportTable === 'function') return engineModule.hostImportTable();
    return Object.freeze({ importCount: quickJsHostImportsPlan ? quickJsHostImportsPlan.importCount : 0, tableHash: quickJsHostImportsPlan ? quickJsHostImportsPlan.tableHash : '', table: quickJsHostImportsPlan ? quickJsHostImportsPlan.table : '', design: quickJsHostImportsPlan ? quickJsHostImportsPlan.design : '' });
  }

  function parityProbe() {
    if (engineModule && typeof engineModule.parityProbe === 'function') return engineModule.parityProbe();
    return Object.freeze({ expected: quickJsParityProbePlan ? quickJsParityProbePlan.expected : 'quickjs:5', native: quickJsParityProbePlan ? quickJsParityProbePlan.native : '', wasm: quickJsParityProbePlan ? quickJsParityProbePlan.wasm : '', matched: false, loaded: false });
  }

  function bytecodeManifest() {
    if (engineModule && typeof engineModule.bytecodeManifest === 'function') return engineModule.bytecodeManifest();
    return Object.freeze({ version: quickJsBytecodeManifestPlan ? quickJsBytecodeManifestPlan.version : 0, format: quickJsBytecodeManifestPlan ? quickJsBytecodeManifestPlan.format : '', chunkCount: quickJsBytecodeManifestPlan ? quickJsBytecodeManifestPlan.chunkCount : 0, records: quickJsBytecodeManifestPlan && quickJsBytecodeManifestPlan.records ? quickJsBytecodeManifestPlan.records : Object.freeze([]), loaded: false });
  }

  function moduleResolver() {
    if (engineModule && typeof engineModule.moduleResolver === 'function') return engineModule.moduleResolver();
    return Object.freeze({ version: quickJsModuleResolverPlan ? quickJsModuleResolverPlan.version : 0, abi: quickJsModuleResolverPlan ? quickJsModuleResolverPlan.abi : 0, mode: quickJsModuleResolverPlan ? quickJsModuleResolverPlan.mode : '', entries: quickJsModuleResolverPlan && quickJsModuleResolverPlan.entries ? quickJsModuleResolverPlan.entries : Object.freeze([]), loaded: false });
  }

  function exceptionAbi() {
    if (engineModule && typeof engineModule.exceptionAbi === 'function') return engineModule.exceptionAbi();
    return Object.freeze({ version: quickJsExceptionAbiPlan ? quickJsExceptionAbiPlan.version : 0, abi: quickJsExceptionAbiPlan ? quickJsExceptionAbiPlan.abi : 0, schema: quickJsExceptionAbiPlan ? quickJsExceptionAbiPlan.schema : '', loaded: false });
  }

  function hostTrapPolicy() {
    if (engineModule && typeof engineModule.hostTrapPolicy === 'function') return engineModule.hostTrapPolicy();
    return Object.freeze({ version: quickJsHostTrapPolicyPlan ? quickJsHostTrapPolicyPlan.version : 0, policy: quickJsHostTrapPolicyPlan ? quickJsHostTrapPolicyPlan.policy : '', unknownImport: quickJsHostTrapPolicyPlan ? quickJsHostTrapPolicyPlan.unknownImport : '', loaded: false });
  }

  function executionLifecycle() {
    if (engineModule && typeof engineModule.executionLifecycle === 'function') return engineModule.executionLifecycle();
    return Object.freeze({ version: quickJsExecutionLifecyclePlan ? quickJsExecutionLifecyclePlan.version : 0, states: quickJsExecutionLifecyclePlan ? quickJsExecutionLifecyclePlan.states : '', strictRelease: !!(quickJsExecutionLifecyclePlan && quickJsExecutionLifecyclePlan.strictRelease), loaded: false });
  }

  function scriptBufferPolicy() {
    if (engineModule && typeof engineModule.scriptBufferPolicy === 'function') return engineModule.scriptBufferPolicy();
    return Object.freeze({ version: quickJsScriptBufferPolicyPlan ? quickJsScriptBufferPolicyPlan.version : 0, maxScriptBytes: quickJsScriptBufferPolicyPlan ? quickJsScriptBufferPolicyPlan.maxScriptBytes : 786432, validateHashBeforeExecute: !(quickJsScriptBufferPolicyPlan && quickJsScriptBufferPolicyPlan.validateHashBeforeExecute === false), loaded: false });
  }

  function contextLimitPolicy() {
    if (engineModule && typeof engineModule.contextLimitPolicy === 'function') return engineModule.contextLimitPolicy();
    return Object.freeze({ version: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.version : 0, maxHeapBytes: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.maxHeapBytes : 0, maxStackBytes: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.maxStackBytes : 0, maxScriptBytes: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.maxScriptBytes : 0, maxHostCalls: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.maxHostCalls : 0, maxConsoleEvents: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.maxConsoleEvents : 0, maxModuleRecords: quickJsContextLimitPolicyPlan ? quickJsContextLimitPolicyPlan.maxModuleRecords : 0, loaded: false });
  }

  function hostCallDispatch() {
    if (engineModule && typeof engineModule.hostCallDispatch === 'function') return engineModule.hostCallDispatch();
    return Object.freeze({ version: quickJsHostCallDispatchPlan ? quickJsHostCallDispatchPlan.version : 0, entryCount: quickJsHostCallDispatchPlan ? quickJsHostCallDispatchPlan.entryCount : 0, unknownHostCall: quickJsHostCallDispatchPlan ? quickJsHostCallDispatchPlan.unknownHostCall : '', calls: quickJsHostCallDispatchPlan && quickJsHostCallDispatchPlan.calls ? quickJsHostCallDispatchPlan.calls : Object.freeze([]), loaded: false });
  }

  function parityContract() {
    if (engineModule && typeof engineModule.parityContract === 'function') return engineModule.parityContract();
    return Object.freeze({ version: quickJsParityContractPlan ? quickJsParityContractPlan.version : 0, compare: quickJsParityContractPlan ? quickJsParityContractPlan.compare : '', releaseOnMismatch: quickJsParityContractPlan ? quickJsParityContractPlan.releaseOnMismatch : '', loaded: false });
  }

  function releaseFailClosed() {
    if (engineModule && typeof engineModule.releaseFailClosed === 'function') return engineModule.releaseFailClosed();
    return Object.freeze({ version: quickJsReleaseFailClosedPlan ? quickJsReleaseFailClosedPlan.version : 0, releasePolicy: quickJsReleaseFailClosedPlan ? quickJsReleaseFailClosedPlan.releasePolicy : '', failOn: quickJsReleaseFailClosedPlan ? quickJsReleaseFailClosedPlan.failOn : '', loaded: false });
  }

  function moduleGraph() {
    if (engineModule && typeof engineModule.moduleGraph === 'function') return engineModule.moduleGraph();
    return Object.freeze({ version: quickJsModuleGraphPlan ? quickJsModuleGraphPlan.version : 0, moduleCount: quickJsModuleGraphPlan ? quickJsModuleGraphPlan.moduleCount : 0, modules: quickJsModuleGraphPlan && quickJsModuleGraphPlan.modules ? quickJsModuleGraphPlan.modules : Object.freeze([]), cacheSize: 0, executions: 0, resolverAudits: 0, loaded: false });
  }

  function moduleCacheSnapshot() {
    if (engineModule && typeof engineModule.moduleCacheSnapshot === 'function') return engineModule.moduleCacheSnapshot();
    return Object.freeze({ version: quickJsModuleCachePlan ? quickJsModuleCachePlan.version : 0, mode: quickJsModuleCachePlan ? quickJsModuleCachePlan.mode : '', size: 0, entries: Object.freeze([]), loaded: false });
  }

  function resolverAudit() {
    if (engineModule && typeof engineModule.resolverAudit === 'function') return engineModule.resolverAudit();
    return Object.freeze({ version: quickJsResolverAuditPlan ? quickJsResolverAuditPlan.version : 0, mode: quickJsResolverAuditPlan ? quickJsResolverAuditPlan.mode : '', count: 0, records: Object.freeze([]), loaded: false });
  }

  function interopFallback() {
    if (engineModule && typeof engineModule.interopFallback === 'function') return engineModule.interopFallback();
    return Object.freeze({ version: quickJsInteropFallbackPlan ? quickJsInteropFallbackPlan.version : 0, kind: quickJsInteropFallbackPlan ? quickJsInteropFallbackPlan.fallbackKind : 'host-esm-transform-prototype', releaseBehavior: quickJsInteropFallbackPlan ? quickJsInteropFallbackPlan.releaseBehavior : 'fail-closed-if-required-contract-missing', loaded: false });
  }

  function executionJournal() {
    if (engineModule && typeof engineModule.executionJournal === 'function') return engineModule.executionJournal();
    return Object.freeze({ version: quickJsExecutionJournalPlan ? quickJsExecutionJournalPlan.version : 0, maxRecords: quickJsExecutionJournalPlan ? quickJsExecutionJournalPlan.maxRecords : 0, records: Object.freeze(executionRecords.slice()), loaded: false });
  }

  function checkpointPolicy() {
    if (engineModule && typeof engineModule.checkpointPolicy === 'function') return engineModule.checkpointPolicy();
    return Object.freeze({ version: quickJsCheckpointPolicyPlan ? quickJsCheckpointPolicyPlan.version : 0, maxCheckpoints: quickJsCheckpointPolicyPlan ? quickJsCheckpointPolicyPlan.maxCheckpoints : 0, capture: quickJsCheckpointPolicyPlan ? quickJsCheckpointPolicyPlan.capture : '', restorePolicy: quickJsCheckpointPolicyPlan ? quickJsCheckpointPolicyPlan.restorePolicy : '', loaded: false });
  }

  function replayCursor() {
    if (engineModule && typeof engineModule.replayCursor === 'function') return engineModule.replayCursor();
    return Object.freeze({ version: quickJsReplayCursorPlan ? quickJsReplayCursorPlan.version : 0, sequence: executionRecords.length, cursorFields: quickJsReplayCursorPlan ? quickJsReplayCursorPlan.cursorFields : '', loaded: false });
  }

  function resumeState() {
    if (engineModule && typeof engineModule.resumeState === 'function') return engineModule.resumeState();
    return Object.freeze({ version: quickJsResumeStatePlan ? quickJsResumeStatePlan.version : 0, contextCount: contexts.size, executionCount: executionRecords.length, consoleEvents: consoleEvents.length, loaded: false });
  }

  function determinismAudit() {
    if (engineModule && typeof engineModule.determinismAudit === 'function') return engineModule.determinismAudit();
    return Object.freeze({ version: quickJsDeterminismAuditPlan ? quickJsDeterminismAuditPlan.version : 0, executionCount: executionRecords.length, hostImportCount: quickJsHostImportsPlan ? quickJsHostImportsPlan.importCount : 0, releaseRequires: quickJsDeterminismAuditPlan ? quickJsDeterminismAuditPlan.releaseRequires : '', loaded: false });
  }

  function snapshotPolicy() {
    if (engineModule && typeof engineModule.snapshotPolicy === 'function') return engineModule.snapshotPolicy();
    return Object.freeze({ version: quickJsSnapshotPolicyPlan ? quickJsSnapshotPolicyPlan.version : 0, capture: quickJsSnapshotPolicyPlan ? quickJsSnapshotPolicyPlan.capture : '', validate: quickJsSnapshotPolicyPlan ? quickJsSnapshotPolicyPlan.validate : '', loaded: false });
  }

  function snapshotRecord() {
    if (engineModule && typeof engineModule.snapshotRecord === 'function') return engineModule.snapshotRecord();
    return Object.freeze({ version: quickJsSnapshotRecordsPlan ? quickJsSnapshotRecordsPlan.version : 0, count: executionRecords.length, loaded: false });
  }

  function replayValidation() {
    if (engineModule && typeof engineModule.replayValidation === 'function') return engineModule.replayValidation();
    return Object.freeze({ version: quickJsReplayValidationPlan ? quickJsReplayValidationPlan.version : 0, checks: quickJsReplayValidationPlan ? quickJsReplayValidationPlan.checks : '', loaded: false });
  }

  function determinismLedger() {
    if (engineModule && typeof engineModule.determinismLedger === 'function') return engineModule.determinismLedger();
    return Object.freeze({ version: quickJsDeterminismLedgerPlan ? quickJsDeterminismLedgerPlan.version : 0, chainHash: quickJsDeterminismLedgerPlan ? quickJsDeterminismLedgerPlan.chainHash : '', entries: executionRecords.length, loaded: false });
  }

  function auditSeal() {
    if (engineModule && typeof engineModule.auditSeal === 'function') return engineModule.auditSeal();
    return Object.freeze({ version: quickJsAuditSealPlan ? quickJsAuditSealPlan.version : 0, releaseRequires: quickJsAuditSealPlan ? quickJsAuditSealPlan.releaseRequires : '', loaded: false });
  }

  function executionCommit() {
    if (engineModule && typeof engineModule.executionCommit === 'function') return engineModule.executionCommit();
    return Object.freeze({ version: quickJsExecutionCommitPlan ? quickJsExecutionCommitPlan.version : 0, commitHostCall: quickJsExecutionCommitPlan ? quickJsExecutionCommitPlan.commitHostCall : 'execution.commit', committed: executionRecords.length, loaded: false });
  }

  function rollbackPolicy() {
    if (engineModule && typeof engineModule.rollbackPolicy === 'function') return engineModule.rollbackPolicy();
    return Object.freeze({ version: quickJsRollbackPolicyPlan ? quickJsRollbackPolicyPlan.version : 0, rollbackHostCall: quickJsRollbackPolicyPlan ? quickJsRollbackPolicyPlan.rollbackHostCall : 'execution.rollback', loaded: false });
  }

  function hostCallReceipts() {
    if (engineModule && typeof engineModule.hostCallReceipts === 'function') return engineModule.hostCallReceipts();
    return Object.freeze({ version: quickJsHostCallReceiptsPlan ? quickJsHostCallReceiptsPlan.version : 0, receiptHostCall: quickJsHostCallReceiptsPlan ? quickJsHostCallReceiptsPlan.receiptHostCall : 'host.receipt', count: executionRecords.length, loaded: false });
  }

  function releaseAcceptance() {
    if (engineModule && typeof engineModule.releaseAcceptance === 'function') return engineModule.releaseAcceptance();
    return Object.freeze({ version: quickJsReleaseAcceptancePlan ? quickJsReleaseAcceptancePlan.version : 0, checks: quickJsReleaseAcceptancePlan ? quickJsReleaseAcceptancePlan.checks : '', accepted: true, loaded: false });
  }

  function commitAudit() {
    if (engineModule && typeof engineModule.commitAudit === 'function') return engineModule.commitAudit();
    return Object.freeze({ version: quickJsCommitAuditPlan ? quickJsCommitAuditPlan.version : 0, auditFields: quickJsCommitAuditPlan ? quickJsCommitAuditPlan.auditFields : '', releaseRequires: quickJsCommitAuditPlan ? quickJsCommitAuditPlan.releaseRequires : '', loaded: false });
  }

  function capabilityPolicy() {
    if (engineModule && typeof engineModule.capabilityPolicy === 'function') return engineModule.capabilityPolicy();
    return Object.freeze({ version: quickJsCapabilityPolicyPlan ? quickJsCapabilityPolicyPlan.version : 0, mode: quickJsCapabilityPolicyPlan ? quickJsCapabilityPolicyPlan.mode : '', capabilities: quickJsCapabilityPolicyPlan && quickJsCapabilityPolicyPlan.capabilities ? quickJsCapabilityPolicyPlan.capabilities : '', releaseRequires: quickJsCapabilityPolicyPlan ? quickJsCapabilityPolicyPlan.releaseRequires : '', loaded: false });
  }

  function hostIoPolicy() {
    if (engineModule && typeof engineModule.hostIoPolicy === 'function') return engineModule.hostIoPolicy();
    return Object.freeze({ version: quickJsHostIoPolicyPlan ? quickJsHostIoPolicyPlan.version : 0, network: quickJsHostIoPolicyPlan ? quickJsHostIoPolicyPlan.network : '', filesystem: quickJsHostIoPolicyPlan ? quickJsHostIoPolicyPlan.filesystem : '', releaseBehavior: quickJsHostIoPolicyPlan ? quickJsHostIoPolicyPlan.releaseBehavior : '', loaded: false });
  }

  function permissionSeal() {
    if (engineModule && typeof engineModule.permissionSeal === 'function') return engineModule.permissionSeal();
    return Object.freeze({ version: quickJsPermissionSealPlan ? quickJsPermissionSealPlan.version : 0, algorithm: quickJsPermissionSealPlan ? quickJsPermissionSealPlan.algorithm : '', releaseRequires: quickJsPermissionSealPlan ? quickJsPermissionSealPlan.releaseRequires : '', loaded: false });
  }

  function policyReceipts() {
    if (engineModule && typeof engineModule.policyReceipts === 'function') return engineModule.policyReceipts();
    return Object.freeze({ version: quickJsPolicyReceiptsPlan ? quickJsPolicyReceiptsPlan.version : 0, hostCall: quickJsPolicyReceiptsPlan ? quickJsPolicyReceiptsPlan.hostCall : 'host.receipt', count: executionRecords.length, loaded: false });
  }

  function releaseGate() {
    if (engineModule && typeof engineModule.releaseGate === 'function') return engineModule.releaseGate();
    return Object.freeze({ version: quickJsReleaseGatePlan ? quickJsReleaseGatePlan.version : 0, gate: quickJsReleaseGatePlan ? quickJsReleaseGatePlan.gate : '', failOn: quickJsReleaseGatePlan ? quickJsReleaseGatePlan.failOn : '', loaded: false });
  }

  function hostIoDecision() {
    if (engineModule && typeof engineModule.hostIoDecision === 'function') return engineModule.hostIoDecision();
    return Object.freeze({ version: quickJsHostIoDecisionPlan ? quickJsHostIoDecisionPlan.version : 0, fields: quickJsHostIoDecisionPlan ? quickJsHostIoDecisionPlan.fields : '', count: executionRecords.length, loaded: false });
  }

  function hostIoDenyTrace() {
    if (engineModule && typeof engineModule.hostIoDenyTrace === 'function') return engineModule.hostIoDenyTrace();
    return Object.freeze({ version: quickJsHostIoDenyTracePlan ? quickJsHostIoDenyTracePlan.version : 0, fields: quickJsHostIoDenyTracePlan ? quickJsHostIoDenyTracePlan.fields : '', count: 0, loaded: false });
  }

  function capabilityLedger() {
    if (engineModule && typeof engineModule.capabilityLedger === 'function') return engineModule.capabilityLedger();
    return Object.freeze({ version: quickJsCapabilityLedgerPlan ? quickJsCapabilityLedgerPlan.version : 0, fields: quickJsCapabilityLedgerPlan ? quickJsCapabilityLedgerPlan.fields : '', entries: executionRecords.length, loaded: false });
  }

  function policySealAudit() {
    if (engineModule && typeof engineModule.policySealAudit === 'function') return engineModule.policySealAudit();
    return Object.freeze({ version: quickJsPolicySealAuditPlan ? quickJsPolicySealAuditPlan.version : 0, fields: quickJsPolicySealAuditPlan ? quickJsPolicySealAuditPlan.fields : '', loaded: false });
  }

  function runtimeDenylist() {
    if (engineModule && typeof engineModule.runtimeDenylist === 'function') return engineModule.runtimeDenylist();
    return Object.freeze({ version: quickJsRuntimeDenylistPlan ? quickJsRuntimeDenylistPlan.version : 0, denylist: quickJsRuntimeDenylistPlan ? quickJsRuntimeDenylistPlan.denylist : '', loaded: false });
  }

  return Object.freeze({
    enabled,
    mode,
    contexts,
    createContext,
    destroyContext,
    destroyRoute,
    contextSnapshot,
    executeChunk,
    bridgeMode: quickJsBridgePlan ? quickJsBridgePlan.mode : 'none',
    moduleEnabled: !!(quickJsEngineModulePlan && quickJsEngineModulePlan.enabled),
    moduleUrl: quickJsEngineModuleUrl || '',
    adapterDiagnostics: quickJsAdapterDiagnosticsPlan ? quickJsAdapterDiagnosticsPlan.records : '',
    executionSnapshot() { return Object.freeze({ count: executionRecords.length, records: Object.freeze(executionRecords.slice()) }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; if (asyncQueue && quickJsResultBridgePlan && quickJsResultBridgePlan.enabled) { const request = asyncQueue.enqueue(quickJsResultBridgePlan.consoleFlushHostCall || 'quickjs.consoleFlush', { count }); asyncQueue.settle(request.id, 'fulfilled', { flushed: count }); } return count; },
    fallbackPolicy() { return Object.freeze({ enabled: !!(quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.enabled), mode: quickJsFallbackPolicyPlan ? quickJsFallbackPolicyPlan.mode : 'legacy', allowWhen: quickJsFallbackPolicyPlan ? quickJsFallbackPolicyPlan.allowWhen : '', strictRelease: !!(quickJsFallbackPolicyPlan && quickJsFallbackPolicyPlan.strictRelease) }); },
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
    snapshotPolicy,
    snapshotRecord,
    replayValidation,
    determinismLedger,
    auditSeal,
    executionCommit,
    rollbackPolicy,
    hostCallReceipts,
    releaseAcceptance,
    commitAudit,
    capabilityPolicy,
    hostIoPolicy,
    permissionSeal,
    policyReceipts,
    releaseGate,
    hostIoDecision,
    hostIoDenyTrace,
    capabilityLedger,
    policySealAudit,
    runtimeDenylist,
    moduleStatus() { return Object.freeze({ enabled, mode, moduleEnabled: !!(quickJsEngineModulePlan && quickJsEngineModulePlan.enabled), moduleLoaded: !!engineModule, moduleUrl: quickJsEngineModuleUrl || '', wasmUrl: quickJsWasmUrl || '', wasmRuntime: !!quickJsWasmRuntimePlan, moduleError: engineModuleError ? String(engineModuleError.message || engineModuleError) : '', contextCount: contexts.size }); },
    moduleError() { return engineModuleError ? String(engineModuleError.message || engineModuleError) : ''; },
  });
}

