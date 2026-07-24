function createTurboJsEngine(asyncQueue, turboJsEnginePlan, scriptEnginePolicyPlan, turboJsBridgePlan, turboJsEngineModulePlan = null, turboJsEngineModuleUrl = null, turboJsContextLifecyclePlan = null, hostCapabilitiesPlan = null, turboJsAdapterDiagnosticsPlan = null, turboJsWasmRuntimePlan = null, turboJsWasmUrl = null, turboJsSourceTransferPlan = null, turboJsConsoleBridgePlan = null, turboJsExecutionRecordsPlan = null, turboJsResultBridgePlan = null, turboJsFallbackPolicyPlan = null, turboJsRuntimeAbiPlan = null, turboJsHostImportsPlan = null, turboJsHeapLimitsPlan = null, turboJsScriptBufferPlan = null, turboJsConsoleAbiPlan = null, turboJsParityProbePlan = null, turboJsReleaseFallbackPlan = null, turboJsBytecodeManifestPlan = null, turboJsModuleResolverPlan = null, turboJsExceptionAbiPlan = null, turboJsHostTrapPolicyPlan = null, turboJsExecutionLifecyclePlan = null, turboJsScriptBufferPolicyPlan = null, turboJsContextLimitPolicyPlan = null, turboJsHostCallDispatchPlan = null, turboJsParityContractPlan = null, turboJsReleaseFailClosedPlan = null, turboJsModuleGraphPlan = null, turboJsModuleExecutionPlan = null, turboJsModuleCachePlan = null, turboJsResolverAuditPlan = null, turboJsInteropFallbackPlan = null, turboJsExecutionJournalPlan = null, turboJsCheckpointPolicyPlan = null, turboJsReplayCursorPlan = null, turboJsResumeStatePlan = null, turboJsDeterminismAuditPlan = null, turboJsSnapshotPolicyPlan = null, turboJsSnapshotRecordsPlan = null, turboJsReplayValidationPlan = null, turboJsDeterminismLedgerPlan = null, turboJsAuditSealPlan = null, turboJsExecutionCommitPlan = null, turboJsRollbackPolicyPlan = null, turboJsHostCallReceiptsPlan = null, turboJsReleaseAcceptancePlan = null, turboJsCommitAuditPlan = null, turboJsCapabilityPolicyPlan = null, turboJsHostIoPolicyPlan = null, turboJsPermissionSealPlan = null, turboJsPolicyReceiptsPlan = null, turboJsReleaseGatePlan = null, turboJsHostIoDecisionPlan = null, turboJsHostIoDenyTracePlan = null, turboJsCapabilityLedgerPlan = null, turboJsPolicySealAuditPlan = null, turboJsRuntimeDenylistPlan = null) {
  const contexts = new Map();
  const enabled = !!(turboJsEnginePlan && turboJsEnginePlan.enabled);
  const mode = enabled ? (turboJsEnginePlan.mode || 'bootstrap') : 'disabled';
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

  function recordTurboJsExecution(kind, payload = {}) {
    const record = Object.freeze({ id: executionRecords.length + 1, kind: String(kind || 'turbojs.executionRecord'), at: Date.now ? Date.now() : 0, ...payload });
    executionRecords.push(record);
    if (asyncQueue && turboJsExecutionRecordsPlan && turboJsExecutionRecordsPlan.enabled) {
      const request = asyncQueue.enqueue(turboJsExecutionRecordsPlan.recordHostCall || 'turbojs.executionRecord', record);
      asyncQueue.settle(request.id, 'fulfilled', { recorded: record.id, kind: record.kind });
    }
    return record;
  }

  function recordResultBridge(payload = {}) {
    const record = Object.freeze({ id: executionRecords.length + 1, kind: 'turbojs.resultBridge', at: Date.now ? Date.now() : 0, ...payload });
    executionRecords.push(record);
    if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) {
      const request = asyncQueue.enqueue(turboJsResultBridgePlan.resultHostCall || 'turbojs.resultBridge', record);
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
        if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) {
          const request = asyncQueue.enqueue(turboJsResultBridgePlan.consoleEventHostCall || 'turbojs.consoleEvent', event);
          asyncQueue.settle(request.id, 'fulfilled', { consoleEvent: event.id });
        }
        if (baseConsole && typeof baseConsole[level] === 'function') baseConsole[level](...args);
      };
    }
    return Object.freeze(out);
  }

  function fallbackAllowed(reason) {
    const mode = turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled ? turboJsFallbackPolicyPlan.mode : 'legacy';
    const denyList = String((turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.denyWhen) || '') + '|' + String((turboJsReleaseFallbackPlan && turboJsReleaseFallbackPlan.denyWhen) || '');
    const strict = !!((turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.strictRelease) || (turboJsReleaseFallbackPlan && turboJsReleaseFallbackPlan.enabled && turboJsReleaseFallbackPlan.denyHostFallback));
    const wasmOrModuleFallback = reason === 'engine-module-unavailable-or-compatible-fallback' || reason === 'wasm-interpreter-unavailable' || reason === 'release-strict-no-fallback';
    const allowed = !(strict && wasmOrModuleFallback) && !denyList.split('|').filter(Boolean).includes(reason);
    if (asyncQueue && turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled) {
      const request = asyncQueue.enqueue(turboJsFallbackPolicyPlan.policyCheckHostCall || 'turbojs.fallbackPolicyCheck', { reason, mode, strict, allowed });
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
      const request = asyncQueue.enqueue(turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.createHostCall ? turboJsContextLifecyclePlan.createHostCall : (turboJsEnginePlan && turboJsEnginePlan.contextCreateHostCall ? turboJsEnginePlan.contextCreateHostCall : 'turbojs.contextCreate'), { route: String(route || '/'), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(request.id, 'fulfilled', { context: key, mode, engineModule: !!turboJsEngineModuleUrl });
      ctx = Object.freeze({ key, generation: asyncQueue.snapshot().generation >>> 0, route: String(route || '/'), source: String(source || ''), order: order >>> 0, mode, createdAt: Date.now ? Date.now() : 0, reuseCount: 0 });
      contexts.set(key, ctx);
    } else if (turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.enabled) {
      const request = asyncQueue.enqueue(turboJsContextLifecyclePlan.reuseHostCall || 'turbojs.contextReuse', { context: key, route: String(route || '/'), source: String(source || ''), order: order >>> 0 });
      asyncQueue.settle(request.id, 'fulfilled', { context: key, reused: true });
    }
    return ctx;
  }

  function destroyContext(route, source, order) {
    const key = contextKey(route, source, order);
    const existed = contexts.delete(key);
    const request = asyncQueue.enqueue(turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.destroyHostCall ? turboJsContextLifecyclePlan.destroyHostCall : 'turbojs.contextDestroy', { context: key, existed });
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
    return Object.freeze({ count: contexts.size, keys: Object.freeze(Array.from(contexts.keys())), moduleLoaded: !!engineModule, moduleError: engineModuleError ? String(engineModuleError.message || engineModuleError) : '', wasmUrl: turboJsWasmUrl || '' });
  }

  function recordBrowserApiShimCall(kind, chunk, detail) {
    const payload = Object.freeze({
      kind: String(kind || 'unknown'),
      route: chunk && chunk.route ? String(chunk.route) : '',
      source: chunk && chunk.source ? String(chunk.source) : '',
      order: chunk && chunk.order ? chunk.order >>> 0 : 0,
      ...(detail || {}),
    });
    if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) {
      const request = asyncQueue.enqueue('turbojs.browserApiShim', payload);
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
    if (permits('console')) bindings.console = turboJsConsoleBridgePlan && turboJsConsoleBridgePlan.enabled ? makeConsoleBridgeConsole(globalThis.console, chunk) : globalThis.console;
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
        const record = bridge.enqueueHostCall(hostCapabilitiesPlan.injectHostCall || 'turbojs.hostCapabilityInject', { route: chunk.route, source: chunk.source, order: chunk.order, capabilities: capabilityNames });
        if (bridge && typeof bridge.settleHostCall === 'function') bridge.settleHostCall(record.id, 'fulfilled', { injected: capabilityNames.length });
      }
      bindings.__venomCapabilities = Object.freeze(capabilityNames.slice());
    }
    return Object.freeze(bindings);
  }

  async function loadEngineModule() {
    if (!turboJsEngineModulePlan || !turboJsEngineModulePlan.enabled || !turboJsEngineModuleUrl) return null;
    if (engineModule) return engineModule;
    if (engineModuleError) return null;
    if (!engineModulePromise) {
      engineModulePromise = import(turboJsEngineModuleUrl).then((mod) => {
        const factoryName = turboJsEngineModulePlan.exportName || 'createVenomTurboJsEngineModule';
        const factory = mod && mod[factoryName];
        if (typeof factory !== 'function') throw new Error('TurboJS engine module missing export: ' + factoryName);
        engineModule = factory({
          version: turboJsEngineModulePlan.version,
          mode: turboJsEngineModulePlan.executionMode,
          fallback: turboJsEngineModulePlan.fallback,
          policy: scriptEnginePolicyPlan,
          bridge: turboJsBridgePlan,
          contextLifecycle: turboJsContextLifecyclePlan,
          hostCapabilities: hostCapabilitiesPlan,
          diagnostics: turboJsAdapterDiagnosticsPlan,
          wasmRuntime: turboJsWasmRuntimePlan,
          wasmUrl: turboJsWasmUrl,
          diversification: activeReleaseDiversification,
          abiFingerprint: activeTurboJsAbiFingerprint,
          sourceTransfer: turboJsSourceTransferPlan,
          consoleBridge: turboJsConsoleBridgePlan,
          executionRecords: turboJsExecutionRecordsPlan,
          resultBridge: turboJsResultBridgePlan,
          fallbackPolicy: turboJsFallbackPolicyPlan,
          runtimeAbi: turboJsRuntimeAbiPlan,
          hostImports: turboJsHostImportsPlan,
          heapLimits: turboJsHeapLimitsPlan,
          scriptBuffer: turboJsScriptBufferPlan,
          consoleAbi: turboJsConsoleAbiPlan,
          parityProbe: turboJsParityProbePlan,
          releaseFallback: turboJsReleaseFallbackPlan,
          bytecodeManifest: turboJsBytecodeManifestPlan,
          moduleResolver: turboJsModuleResolverPlan,
          exceptionAbi: turboJsExceptionAbiPlan,
          hostTrapPolicy: turboJsHostTrapPolicyPlan,
          executionLifecycle: turboJsExecutionLifecyclePlan,
          scriptBufferPolicy: turboJsScriptBufferPolicyPlan,
          contextLimitPolicy: turboJsContextLimitPolicyPlan,
          hostCallDispatch: turboJsHostCallDispatchPlan,
          parityContract: turboJsParityContractPlan,
          releaseFailClosed: turboJsReleaseFailClosedPlan,
          moduleGraph: turboJsModuleGraphPlan,
          moduleExecution: turboJsModuleExecutionPlan,
          moduleCache: turboJsModuleCachePlan,
          resolverAudit: turboJsResolverAuditPlan,
          interopFallback: turboJsInteropFallbackPlan,
          executionJournal: turboJsExecutionJournalPlan,
          checkpointPolicy: turboJsCheckpointPolicyPlan,
          replayCursor: turboJsReplayCursorPlan,
          resumeState: turboJsResumeStatePlan,
          determinismAudit: turboJsDeterminismAuditPlan,
          snapshotPolicy: turboJsSnapshotPolicyPlan,
          snapshotRecords: turboJsSnapshotRecordsPlan,
          replayValidation: turboJsReplayValidationPlan,
          determinismLedger: turboJsDeterminismLedgerPlan,
          auditSeal: turboJsAuditSealPlan,
          executionCommit: turboJsExecutionCommitPlan,
          rollbackPolicy: turboJsRollbackPolicyPlan,
          hostCallReceipts: turboJsHostCallReceiptsPlan,
          releaseAcceptance: turboJsReleaseAcceptancePlan,
          commitAudit: turboJsCommitAuditPlan,
        });
        if (!engineModule || typeof engineModule.executeChunk !== 'function') throw new Error('TurboJS engine module did not return executeChunk');
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
      const ctxRecord = asyncQueue.enqueue(turboJsContextLifecyclePlan && turboJsContextLifecyclePlan.moduleCreateHostCall ? turboJsContextLifecyclePlan.moduleCreateHostCall : 'turbojs.moduleContextCreate', { context: context.key, route: chunk.route, source: chunk.source, order: chunk.order });
      const created = module.createContext(context);
      asyncQueue.settle(ctxRecord.id, 'fulfilled', { context: context.key, moduleContext: !!created });
    }
    const moduleRequest = asyncQueue.enqueue('turbojs.moduleExecute', {
      route: chunk.route,
      source: chunk.source,
      order: chunk.order,
      bytes: chunk.bytecodeSize || (chunk.code ? chunk.code.length : 0),
      moduleUrl: String(turboJsEngineModuleUrl || ''),
    });
    try {
      const result = await module.executeChunk({ chunk, route, context, bindings, wasmUrl: turboJsWasmUrl, wasmRuntime: turboJsWasmRuntimePlan, sourceTransfer: turboJsSourceTransferPlan, consoleBridge: turboJsConsoleBridgePlan, executionRecords: turboJsExecutionRecordsPlan, resultBridge: turboJsResultBridgePlan, fallbackPolicy: turboJsFallbackPolicyPlan });
      recordTurboJsExecution('turbojs.executionRecord', { route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, engine: result && result.turboJsWasm ? 'turbojs-wasm-backend-candidate' : 'host-js-fallback', fallbackUsed: !!(result && result.fallback), wasmAccepted: !!(result && result.turboJsWasm), hostBridgeTelemetry: result && result.hostBridgeTelemetry ? result.hostBridgeTelemetry : null, hostBridgeParity: result && result.hostBridgeParity ? result.hostBridgeParity : null });
      recordResultBridge({ route: chunk.route, source: chunk.source, order: chunk.order, context: context.key, ok: !!(result && result.executed), wasmReport: result && result.wasmReport ? result.wasmReport : null, hostBridgeTelemetry: result && result.hostBridgeTelemetry ? result.hostBridgeTelemetry : null });
      asyncQueue.settle(moduleRequest.id, 'fulfilled', { executed: !!(result && result.executed), engineModule: true });
      return Object.freeze({
        ...chunk,
        ...(result || {}),
        executed: !!(result && result.executed),
        engineMode: turboJsEngineModulePlan.executionMode || 'route-scoped-module',
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
    const request = asyncQueue.enqueue(turboJsEnginePlan && turboJsEnginePlan.executeHostCall ? turboJsEnginePlan.executeHostCall : 'turbojs.executeChunk', { route: chunk.route, source: chunk.source, order: chunk.order, bytes: chunk.bytecodeSize || (chunk.code ? chunk.code.length : 0), bytecode: !!chunk.bytecode, mode, engineModule: !!turboJsEngineModuleUrl });
    try {
      if (!enabled) throw new Error('TurboJS engine bootstrap is disabled');
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
      if (executionGeneration !== (asyncQueue.snapshot().generation >>> 0)) throw new Error('stale route generation rejected for TurboJS execution');
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
      if (!fallbackAllowed('engine-module-unavailable-or-compatible-fallback')) throw new Error('TurboJS fallback policy denied host execution');
      if (asyncQueue && turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled) { const decision = asyncQueue.enqueue(turboJsFallbackPolicyPlan.decisionHostCall || 'script.fallbackDecision', { route: chunk.route, source: chunk.source, order: chunk.order, allowed: true }); asyncQueue.settle(decision.id, 'fulfilled', { allowed: true }); }
__VENOM_RUNTIME_ENGINE_FALLBACK_BLOCK__
    } catch (error) {
      if (request && request.id && executionGeneration === (asyncQueue.snapshot().generation >>> 0)) asyncQueue.settle(request.id, 'rejected', { message: error && error.message ? error.message : String(error), context: context.key });
      throw error;
    }
  }

  function abiTable() {
    if (engineModule && typeof engineModule.abiTable === 'function') return engineModule.abiTable();
    return Object.freeze({ abi: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.abi : 0, packageVersion: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.packageVersion : 0, entryCount: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.entryCount : 0, tableHash: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.tableHash : '', table: turboJsRuntimeAbiPlan ? turboJsRuntimeAbiPlan.table : '' });
  }

  function hostImportTable() {
    if (engineModule && typeof engineModule.hostImportTable === 'function') return engineModule.hostImportTable();
    return Object.freeze({ importCount: turboJsHostImportsPlan ? turboJsHostImportsPlan.importCount : 0, tableHash: turboJsHostImportsPlan ? turboJsHostImportsPlan.tableHash : '', table: turboJsHostImportsPlan ? turboJsHostImportsPlan.table : '', design: turboJsHostImportsPlan ? turboJsHostImportsPlan.design : '' });
  }

  function parityProbe() {
    if (engineModule && typeof engineModule.parityProbe === 'function') return engineModule.parityProbe();
    return Object.freeze({ expected: turboJsParityProbePlan ? turboJsParityProbePlan.expected : 'turbojs:5', native: turboJsParityProbePlan ? turboJsParityProbePlan.native : '', wasm: turboJsParityProbePlan ? turboJsParityProbePlan.wasm : '', matched: false, loaded: false });
  }

  function bytecodeManifest() {
    if (engineModule && typeof engineModule.bytecodeManifest === 'function') return engineModule.bytecodeManifest();
    return Object.freeze({ version: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.version : 0, format: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.format : '', chunkCount: turboJsBytecodeManifestPlan ? turboJsBytecodeManifestPlan.chunkCount : 0, records: turboJsBytecodeManifestPlan && turboJsBytecodeManifestPlan.records ? turboJsBytecodeManifestPlan.records : Object.freeze([]), loaded: false });
  }

  function moduleResolver() {
    if (engineModule && typeof engineModule.moduleResolver === 'function') return engineModule.moduleResolver();
    return Object.freeze({ version: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.version : 0, abi: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.abi : 0, mode: turboJsModuleResolverPlan ? turboJsModuleResolverPlan.mode : '', entries: turboJsModuleResolverPlan && turboJsModuleResolverPlan.entries ? turboJsModuleResolverPlan.entries : Object.freeze([]), loaded: false });
  }

  function exceptionAbi() {
    if (engineModule && typeof engineModule.exceptionAbi === 'function') return engineModule.exceptionAbi();
    return Object.freeze({ version: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.version : 0, abi: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.abi : 0, schema: turboJsExceptionAbiPlan ? turboJsExceptionAbiPlan.schema : '', loaded: false });
  }

  function hostTrapPolicy() {
    if (engineModule && typeof engineModule.hostTrapPolicy === 'function') return engineModule.hostTrapPolicy();
    return Object.freeze({ version: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.version : 0, policy: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.policy : '', unknownImport: turboJsHostTrapPolicyPlan ? turboJsHostTrapPolicyPlan.unknownImport : '', loaded: false });
  }

  function executionLifecycle() {
    if (engineModule && typeof engineModule.executionLifecycle === 'function') return engineModule.executionLifecycle();
    return Object.freeze({ version: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.version : 0, states: turboJsExecutionLifecyclePlan ? turboJsExecutionLifecyclePlan.states : '', strictRelease: !!(turboJsExecutionLifecyclePlan && turboJsExecutionLifecyclePlan.strictRelease), loaded: false });
  }

  function scriptBufferPolicy() {
    if (engineModule && typeof engineModule.scriptBufferPolicy === 'function') return engineModule.scriptBufferPolicy();
    return Object.freeze({ version: turboJsScriptBufferPolicyPlan ? turboJsScriptBufferPolicyPlan.version : 0, maxScriptBytes: turboJsScriptBufferPolicyPlan ? turboJsScriptBufferPolicyPlan.maxScriptBytes : 786432, validateHashBeforeExecute: !(turboJsScriptBufferPolicyPlan && turboJsScriptBufferPolicyPlan.validateHashBeforeExecute === false), loaded: false });
  }

  function contextLimitPolicy() {
    if (engineModule && typeof engineModule.contextLimitPolicy === 'function') return engineModule.contextLimitPolicy();
    return Object.freeze({ version: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.version : 0, maxHeapBytes: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxHeapBytes : 0, maxStackBytes: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxStackBytes : 0, maxScriptBytes: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxScriptBytes : 0, maxHostCalls: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxHostCalls : 0, maxConsoleEvents: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxConsoleEvents : 0, maxModuleRecords: turboJsContextLimitPolicyPlan ? turboJsContextLimitPolicyPlan.maxModuleRecords : 0, loaded: false });
  }

  function hostCallDispatch() {
    if (engineModule && typeof engineModule.hostCallDispatch === 'function') return engineModule.hostCallDispatch();
    return Object.freeze({ version: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.version : 0, entryCount: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.entryCount : 0, unknownHostCall: turboJsHostCallDispatchPlan ? turboJsHostCallDispatchPlan.unknownHostCall : '', calls: turboJsHostCallDispatchPlan && turboJsHostCallDispatchPlan.calls ? turboJsHostCallDispatchPlan.calls : Object.freeze([]), loaded: false });
  }

  function parityContract() {
    if (engineModule && typeof engineModule.parityContract === 'function') return engineModule.parityContract();
    return Object.freeze({ version: turboJsParityContractPlan ? turboJsParityContractPlan.version : 0, compare: turboJsParityContractPlan ? turboJsParityContractPlan.compare : '', releaseOnMismatch: turboJsParityContractPlan ? turboJsParityContractPlan.releaseOnMismatch : '', loaded: false });
  }

  function releaseFailClosed() {
    if (engineModule && typeof engineModule.releaseFailClosed === 'function') return engineModule.releaseFailClosed();
    return Object.freeze({ version: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.version : 0, releasePolicy: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.releasePolicy : '', failOn: turboJsReleaseFailClosedPlan ? turboJsReleaseFailClosedPlan.failOn : '', loaded: false });
  }

  function moduleGraph() {
    if (engineModule && typeof engineModule.moduleGraph === 'function') return engineModule.moduleGraph();
    return Object.freeze({ version: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.version : 0, moduleCount: turboJsModuleGraphPlan ? turboJsModuleGraphPlan.moduleCount : 0, modules: turboJsModuleGraphPlan && turboJsModuleGraphPlan.modules ? turboJsModuleGraphPlan.modules : Object.freeze([]), cacheSize: 0, executions: 0, resolverAudits: 0, loaded: false });
  }

  function moduleCacheSnapshot() {
    if (engineModule && typeof engineModule.moduleCacheSnapshot === 'function') return engineModule.moduleCacheSnapshot();
    return Object.freeze({ version: turboJsModuleCachePlan ? turboJsModuleCachePlan.version : 0, mode: turboJsModuleCachePlan ? turboJsModuleCachePlan.mode : '', size: 0, entries: Object.freeze([]), loaded: false });
  }

  function resolverAudit() {
    if (engineModule && typeof engineModule.resolverAudit === 'function') return engineModule.resolverAudit();
    return Object.freeze({ version: turboJsResolverAuditPlan ? turboJsResolverAuditPlan.version : 0, mode: turboJsResolverAuditPlan ? turboJsResolverAuditPlan.mode : '', count: 0, records: Object.freeze([]), loaded: false });
  }

  function interopFallback() {
    if (engineModule && typeof engineModule.interopFallback === 'function') return engineModule.interopFallback();
    return Object.freeze({ version: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.version : 0, kind: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.fallbackKind : 'host-esm-transform-prototype', releaseBehavior: turboJsInteropFallbackPlan ? turboJsInteropFallbackPlan.releaseBehavior : 'fail-closed-if-required-contract-missing', loaded: false });
  }

  function executionJournal() {
    if (engineModule && typeof engineModule.executionJournal === 'function') return engineModule.executionJournal();
    return Object.freeze({ version: turboJsExecutionJournalPlan ? turboJsExecutionJournalPlan.version : 0, maxRecords: turboJsExecutionJournalPlan ? turboJsExecutionJournalPlan.maxRecords : 0, records: Object.freeze(executionRecords.slice()), loaded: false });
  }

  function checkpointPolicy() {
    if (engineModule && typeof engineModule.checkpointPolicy === 'function') return engineModule.checkpointPolicy();
    return Object.freeze({ version: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.version : 0, maxCheckpoints: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.maxCheckpoints : 0, capture: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.capture : '', restorePolicy: turboJsCheckpointPolicyPlan ? turboJsCheckpointPolicyPlan.restorePolicy : '', loaded: false });
  }

  function replayCursor() {
    if (engineModule && typeof engineModule.replayCursor === 'function') return engineModule.replayCursor();
    return Object.freeze({ version: turboJsReplayCursorPlan ? turboJsReplayCursorPlan.version : 0, sequence: executionRecords.length, cursorFields: turboJsReplayCursorPlan ? turboJsReplayCursorPlan.cursorFields : '', loaded: false });
  }

  function resumeState() {
    if (engineModule && typeof engineModule.resumeState === 'function') return engineModule.resumeState();
    return Object.freeze({ version: turboJsResumeStatePlan ? turboJsResumeStatePlan.version : 0, contextCount: contexts.size, executionCount: executionRecords.length, consoleEvents: consoleEvents.length, loaded: false });
  }

  function determinismAudit() {
    if (engineModule && typeof engineModule.determinismAudit === 'function') return engineModule.determinismAudit();
    return Object.freeze({ version: turboJsDeterminismAuditPlan ? turboJsDeterminismAuditPlan.version : 0, executionCount: executionRecords.length, hostImportCount: turboJsHostImportsPlan ? turboJsHostImportsPlan.importCount : 0, releaseRequires: turboJsDeterminismAuditPlan ? turboJsDeterminismAuditPlan.releaseRequires : '', loaded: false });
  }

  function snapshotPolicy() {
    if (engineModule && typeof engineModule.snapshotPolicy === 'function') return engineModule.snapshotPolicy();
    return Object.freeze({ version: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.version : 0, capture: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.capture : '', validate: turboJsSnapshotPolicyPlan ? turboJsSnapshotPolicyPlan.validate : '', loaded: false });
  }

  function snapshotRecord() {
    if (engineModule && typeof engineModule.snapshotRecord === 'function') return engineModule.snapshotRecord();
    return Object.freeze({ version: turboJsSnapshotRecordsPlan ? turboJsSnapshotRecordsPlan.version : 0, count: executionRecords.length, loaded: false });
  }

  function replayValidation() {
    if (engineModule && typeof engineModule.replayValidation === 'function') return engineModule.replayValidation();
    return Object.freeze({ version: turboJsReplayValidationPlan ? turboJsReplayValidationPlan.version : 0, checks: turboJsReplayValidationPlan ? turboJsReplayValidationPlan.checks : '', loaded: false });
  }

  function determinismLedger() {
    if (engineModule && typeof engineModule.determinismLedger === 'function') return engineModule.determinismLedger();
    return Object.freeze({ version: turboJsDeterminismLedgerPlan ? turboJsDeterminismLedgerPlan.version : 0, chainHash: turboJsDeterminismLedgerPlan ? turboJsDeterminismLedgerPlan.chainHash : '', entries: executionRecords.length, loaded: false });
  }

  function auditSeal() {
    if (engineModule && typeof engineModule.auditSeal === 'function') return engineModule.auditSeal();
    return Object.freeze({ version: turboJsAuditSealPlan ? turboJsAuditSealPlan.version : 0, releaseRequires: turboJsAuditSealPlan ? turboJsAuditSealPlan.releaseRequires : '', loaded: false });
  }

  function executionCommit() {
    if (engineModule && typeof engineModule.executionCommit === 'function') return engineModule.executionCommit();
    return Object.freeze({ version: turboJsExecutionCommitPlan ? turboJsExecutionCommitPlan.version : 0, commitHostCall: turboJsExecutionCommitPlan ? turboJsExecutionCommitPlan.commitHostCall : 'execution.commit', committed: executionRecords.length, loaded: false });
  }

  function rollbackPolicy() {
    if (engineModule && typeof engineModule.rollbackPolicy === 'function') return engineModule.rollbackPolicy();
    return Object.freeze({ version: turboJsRollbackPolicyPlan ? turboJsRollbackPolicyPlan.version : 0, rollbackHostCall: turboJsRollbackPolicyPlan ? turboJsRollbackPolicyPlan.rollbackHostCall : 'execution.rollback', loaded: false });
  }

  function hostCallReceipts() {
    if (engineModule && typeof engineModule.hostCallReceipts === 'function') return engineModule.hostCallReceipts();
    return Object.freeze({ version: turboJsHostCallReceiptsPlan ? turboJsHostCallReceiptsPlan.version : 0, receiptHostCall: turboJsHostCallReceiptsPlan ? turboJsHostCallReceiptsPlan.receiptHostCall : 'host.receipt', count: executionRecords.length, loaded: false });
  }

  function releaseAcceptance() {
    if (engineModule && typeof engineModule.releaseAcceptance === 'function') return engineModule.releaseAcceptance();
    return Object.freeze({ version: turboJsReleaseAcceptancePlan ? turboJsReleaseAcceptancePlan.version : 0, checks: turboJsReleaseAcceptancePlan ? turboJsReleaseAcceptancePlan.checks : '', accepted: true, loaded: false });
  }

  function commitAudit() {
    if (engineModule && typeof engineModule.commitAudit === 'function') return engineModule.commitAudit();
    return Object.freeze({ version: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.version : 0, auditFields: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.auditFields : '', releaseRequires: turboJsCommitAuditPlan ? turboJsCommitAuditPlan.releaseRequires : '', loaded: false });
  }

  function capabilityPolicy() {
    if (engineModule && typeof engineModule.capabilityPolicy === 'function') return engineModule.capabilityPolicy();
    return Object.freeze({ version: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.version : 0, mode: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.mode : '', capabilities: turboJsCapabilityPolicyPlan && turboJsCapabilityPolicyPlan.capabilities ? turboJsCapabilityPolicyPlan.capabilities : '', releaseRequires: turboJsCapabilityPolicyPlan ? turboJsCapabilityPolicyPlan.releaseRequires : '', loaded: false });
  }

  function hostIoPolicy() {
    if (engineModule && typeof engineModule.hostIoPolicy === 'function') return engineModule.hostIoPolicy();
    return Object.freeze({ version: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.version : 0, network: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.network : '', filesystem: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.filesystem : '', releaseBehavior: turboJsHostIoPolicyPlan ? turboJsHostIoPolicyPlan.releaseBehavior : '', loaded: false });
  }

  function permissionSeal() {
    if (engineModule && typeof engineModule.permissionSeal === 'function') return engineModule.permissionSeal();
    return Object.freeze({ version: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.version : 0, algorithm: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.algorithm : '', releaseRequires: turboJsPermissionSealPlan ? turboJsPermissionSealPlan.releaseRequires : '', loaded: false });
  }

  function policyReceipts() {
    if (engineModule && typeof engineModule.policyReceipts === 'function') return engineModule.policyReceipts();
    return Object.freeze({ version: turboJsPolicyReceiptsPlan ? turboJsPolicyReceiptsPlan.version : 0, hostCall: turboJsPolicyReceiptsPlan ? turboJsPolicyReceiptsPlan.hostCall : 'host.receipt', count: executionRecords.length, loaded: false });
  }

  function releaseGate() {
    if (engineModule && typeof engineModule.releaseGate === 'function') return engineModule.releaseGate();
    return Object.freeze({ version: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.version : 0, gate: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.gate : '', failOn: turboJsReleaseGatePlan ? turboJsReleaseGatePlan.failOn : '', loaded: false });
  }

  function hostIoDecision() {
    if (engineModule && typeof engineModule.hostIoDecision === 'function') return engineModule.hostIoDecision();
    return Object.freeze({ version: turboJsHostIoDecisionPlan ? turboJsHostIoDecisionPlan.version : 0, fields: turboJsHostIoDecisionPlan ? turboJsHostIoDecisionPlan.fields : '', count: executionRecords.length, loaded: false });
  }

  function hostIoDenyTrace() {
    if (engineModule && typeof engineModule.hostIoDenyTrace === 'function') return engineModule.hostIoDenyTrace();
    return Object.freeze({ version: turboJsHostIoDenyTracePlan ? turboJsHostIoDenyTracePlan.version : 0, fields: turboJsHostIoDenyTracePlan ? turboJsHostIoDenyTracePlan.fields : '', count: 0, loaded: false });
  }

  function capabilityLedger() {
    if (engineModule && typeof engineModule.capabilityLedger === 'function') return engineModule.capabilityLedger();
    return Object.freeze({ version: turboJsCapabilityLedgerPlan ? turboJsCapabilityLedgerPlan.version : 0, fields: turboJsCapabilityLedgerPlan ? turboJsCapabilityLedgerPlan.fields : '', entries: executionRecords.length, loaded: false });
  }

  function policySealAudit() {
    if (engineModule && typeof engineModule.policySealAudit === 'function') return engineModule.policySealAudit();
    return Object.freeze({ version: turboJsPolicySealAuditPlan ? turboJsPolicySealAuditPlan.version : 0, fields: turboJsPolicySealAuditPlan ? turboJsPolicySealAuditPlan.fields : '', loaded: false });
  }

  function runtimeDenylist() {
    if (engineModule && typeof engineModule.runtimeDenylist === 'function') return engineModule.runtimeDenylist();
    return Object.freeze({ version: turboJsRuntimeDenylistPlan ? turboJsRuntimeDenylistPlan.version : 0, denylist: turboJsRuntimeDenylistPlan ? turboJsRuntimeDenylistPlan.denylist : '', loaded: false });
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
    bridgeMode: turboJsBridgePlan ? turboJsBridgePlan.mode : 'none',
    moduleEnabled: !!(turboJsEngineModulePlan && turboJsEngineModulePlan.enabled),
    moduleUrl: turboJsEngineModuleUrl || '',
    adapterDiagnostics: turboJsAdapterDiagnosticsPlan ? turboJsAdapterDiagnosticsPlan.records : '',
    executionSnapshot() { return Object.freeze({ count: executionRecords.length, records: Object.freeze(executionRecords.slice()) }); },
    consoleEvents() { return Object.freeze(consoleEvents.slice()); },
    clearConsoleEvents() { const count = consoleEvents.length; consoleEvents.length = 0; if (asyncQueue && turboJsResultBridgePlan && turboJsResultBridgePlan.enabled) { const request = asyncQueue.enqueue(turboJsResultBridgePlan.consoleFlushHostCall || 'turbojs.consoleFlush', { count }); asyncQueue.settle(request.id, 'fulfilled', { flushed: count }); } return count; },
    fallbackPolicy() { return Object.freeze({ enabled: !!(turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.enabled), mode: turboJsFallbackPolicyPlan ? turboJsFallbackPolicyPlan.mode : 'legacy', allowWhen: turboJsFallbackPolicyPlan ? turboJsFallbackPolicyPlan.allowWhen : '', strictRelease: !!(turboJsFallbackPolicyPlan && turboJsFallbackPolicyPlan.strictRelease) }); },
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
    moduleStatus() { return Object.freeze({ enabled, mode, moduleEnabled: !!(turboJsEngineModulePlan && turboJsEngineModulePlan.enabled), moduleLoaded: !!engineModule, moduleUrl: turboJsEngineModuleUrl || '', wasmUrl: turboJsWasmUrl || '', wasmRuntime: !!turboJsWasmRuntimePlan, moduleError: engineModuleError ? String(engineModuleError.message || engineModuleError) : '', contextCount: contexts.size }); },
    moduleError() { return engineModuleError ? String(engineModuleError.message || engineModuleError) : ''; },
  });
}

