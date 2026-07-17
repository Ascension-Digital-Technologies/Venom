function createAsyncHostQueue(fetchPlan, queuePlan, timerPlan = null, quickJsPlan = null) {
  let nextId = 1;
  let nextTimerId = 1;
  const pending = new Map();
  const completed = new Map();
  const timers = new Map();
  const maxPending = queuePlan && queuePlan.maxPending ? queuePlan.maxPending : 1024;
  const maxTimers = timerPlan && timerPlan.maxActive ? timerPlan.maxActive : 256;
  const maxCompleted = queuePlan && queuePlan.maxCompleted ? queuePlan.maxCompleted : 256;
  const maxFetchResponseBytes = queuePlan && queuePlan.maxFetchResponseBytes ? queuePlan.maxFetchResponseBytes : 1048576;
  const maxFetchRequestBytes = queuePlan && queuePlan.maxFetchRequestBytes ? queuePlan.maxFetchRequestBytes : 65536;
  const maxTimerDelayMs = queuePlan && queuePlan.maxTimerDelayMs ? queuePlan.maxTimerDelayMs : 86400000;
  const maxRouteLifetimeMs = queuePlan && queuePlan.maxRouteLifetimeMs ? queuePlan.maxRouteLifetimeMs : 86400000;
  const maxHostCallsPerRoute = queuePlan && queuePlan.maxHostCallsPerRoute ? queuePlan.maxHostCallsPerRoute : 8192;
  const maxConcurrentFetches = queuePlan && queuePlan.maxConcurrentFetches ? queuePlan.maxConcurrentFetches : 16;
  const maxScheduledPerRoute = timerPlan && timerPlan.maxScheduledPerRoute ? timerPlan.maxScheduledPerRoute : 512;
  const abortControllers = new Map();
  let activeFetches = 0;
  let disposed = false;
  let activeGeneration = 0;
  let routeStartedAt = Date.now ? Date.now() : 0;
  let hostCallsThisRoute = 0;
  let fetchesThisRoute = 0;
  let timersScheduledThisRoute = 0;

  function assertRouteLifetime(label = 'route operation') {
    const now = Date.now ? Date.now() : routeStartedAt;
    if (maxRouteLifetimeMs && now - routeStartedAt > maxRouteLifetimeMs) throw new Error('Venom route lifetime limit exceeded for ' + label);
  }

  function snapshot() {
    return Object.freeze({ pending: pending.size, completed: completed.size, timers: timers.size, activeFetches, nextId, nextTimerId, generation: activeGeneration, hostCallsThisRoute, fetchesThisRoute, timersScheduledThisRoute, routeAgeMs: Math.max(0, (Date.now ? Date.now() : routeStartedAt) - routeStartedAt), limits: Object.freeze({ maxPending, maxCompleted, maxTimers, maxHostCallsPerRoute, maxConcurrentFetches, maxScheduledPerRoute, maxRouteLifetimeMs }) });
  }

  function enqueue(kind, payload = {}) {
    if (disposed) throw new Error('Venom async host queue is disposed');
    assertRouteLifetime(kind || 'host.call');
    if (hostCallsThisRoute >= maxHostCallsPerRoute) throw new Error('Venom per-route host-call budget exceeded');
    if (pending.size >= maxPending) throw new Error('Venom async host-call queue capacity exceeded');
    const id = nextId++ >>> 0;
    const record = { id, kind: String(kind || 'host.call'), state: 'pending', payload, generation: activeGeneration, createdAt: Date.now ? Date.now() : 0 };
    pending.set(id, record);
    hostCallsThisRoute++;
    return Object.freeze({ id, kind: record.kind, state: record.state });
  }

  function assertGeneration(generation, label = 'async operation') {
    if ((generation >>> 0) !== (activeGeneration >>> 0)) throw new Error('stale route generation rejected for ' + label);
  }

  function settle(id, state, value) {
    const record = pending.get(id >>> 0);
    if (!record) throw new Error('unknown Venom async host-call id: ' + id);
    assertGeneration(record.generation, record.kind);
    pending.delete(id >>> 0);
    const completedRecord = Object.freeze({ ...record, state, value, completedAt: Date.now ? Date.now() : 0 });
    completed.set(id >>> 0, completedRecord);
    while (completed.size > maxCompleted) completed.delete(completed.keys().next().value);
    return completedRecord;
  }

  async function requestFetch(input, init = {}) {
    if (!fetchPlan || !fetchPlan.enabled) throw new Error('Venom fetch bridge is disabled');
    if (typeof globalThis.fetch !== 'function') throw new Error('global fetch is not available');
    assertRouteLifetime('fetch request');
    if (activeFetches >= maxConcurrentFetches) throw new Error('Venom concurrent fetch limit exceeded');
    const requestBytes = new TextEncoder().encode(String(input) + JSON.stringify(init || {})).byteLength;
    if (requestBytes > maxFetchRequestBytes) throw new Error('Venom fetch request size limit exceeded');
    const controller = typeof AbortController === 'function' ? new AbortController() : null;
    const requestGeneration = activeGeneration;
    const request = enqueue(fetchPlan.requestHostCall || 'fetch.request', { input: String(input), init });
    fetchesThisRoute++;
    activeFetches++;
    if (controller) abortControllers.set(request.id, controller);
    try {
      const authorizedUrl = authorizeHostUrl(input, { sameOriginOnly: true });
      const response = await globalThis.fetch(authorizedUrl.href, controller ? { ...init, signal: controller.signal, credentials: 'same-origin', redirect: 'error' } : { ...init, credentials: 'same-origin', redirect: 'error' });
      assertGeneration(requestGeneration, 'fetch response');
      const headers = {};
      if (response && response.headers && typeof response.headers.forEach === 'function') {
        response.headers.forEach((value, key) => { headers[key] = value; });
      }
      const bridgeHeaders = Object.freeze({
        ...headers,
        get: (name) => { const key = String(name || '').toLowerCase(); return Object.prototype.hasOwnProperty.call(headers, key) ? headers[key] : (Object.prototype.hasOwnProperty.call(headers, String(name || '')) ? headers[String(name || '')] : null); },
        forEach: (cb) => { if (typeof cb === 'function') { for (const [key, value] of Object.entries(headers)) cb(value, key); } },
      });
      const bridgeResponse = {
        ok: !!(response && response.ok),
        status: response && typeof response.status === 'number' ? response.status : 0,
        statusText: response && response.statusText ? String(response.statusText) : '',
        url: response && response.url ? String(response.url) : authorizedUrl.href,
        headers: bridgeHeaders,
        text: async () => { assertGeneration(requestGeneration, 'fetch text'); const value = response && typeof response.text === 'function' ? await response.text() : ''; assertGeneration(requestGeneration, 'fetch text'); if (new TextEncoder().encode(value).byteLength > maxFetchResponseBytes) throw new Error('Venom fetch response size limit exceeded'); return value; },
        json: async () => { assertGeneration(requestGeneration, 'fetch json'); const value = response && typeof response.json === 'function' ? await response.json() : JSON.parse(await bridgeResponse.text()); assertGeneration(requestGeneration, 'fetch json'); return value; },
        arrayBuffer: async () => { assertGeneration(requestGeneration, 'fetch arrayBuffer'); const value = response && typeof response.arrayBuffer === 'function' ? await response.arrayBuffer() : new ArrayBuffer(0); assertGeneration(requestGeneration, 'fetch arrayBuffer'); if (value.byteLength > maxFetchResponseBytes) throw new Error('Venom fetch response size limit exceeded'); return value; },
        raw: response,
        venomRequestId: request.id,
      };
      abortControllers.delete(request.id);
      activeFetches = Math.max(0, activeFetches - 1);
      settle(request.id, 'fulfilled', { status: bridgeResponse.status, ok: bridgeResponse.ok });
      return Object.freeze(bridgeResponse);
    } catch (error) {
      abortControllers.delete(request.id);
      activeFetches = Math.max(0, activeFetches - 1);
      if (pending.has(request.id) && (requestGeneration >>> 0) === (activeGeneration >>> 0)) {
        settle(request.id, 'rejected', { message: error && error.message ? error.message : String(error) });
      }
      throw error;
    }
  }

  function scheduleTimer(callback, delay = 0, repeat = false) {
    if (!timerPlan || !timerPlan.enabled) throw new Error('Venom timer bridge is disabled');
    assertRouteLifetime('timer schedule');
    if (timers.size >= maxTimers) throw new Error('Venom timer bridge capacity exceeded');
    if (timersScheduledThisRoute >= maxScheduledPerRoute) throw new Error('Venom per-route timer schedule budget exceeded');
    const id = nextTimerId++ >>> 0;
    timersScheduledThisRoute++;
    const ms = Math.min(maxTimerDelayMs, Math.max(Number(delay) || 0, timerPlan.minimumDelayMs || 0));
    const timerGeneration = activeGeneration;
    const request = enqueue(timerPlan.scheduleHostCall || 'timer.schedule', { timerId: id, delayMs: ms, repeat: !!repeat });
    const runner = () => {
      if ((timerGeneration >>> 0) !== (activeGeneration >>> 0)) {
        if (repeat && typeof globalThis.clearInterval === 'function') globalThis.clearInterval(nativeId);
        if (!repeat && typeof globalThis.clearTimeout === 'function') globalThis.clearTimeout(nativeId);
        timers.delete(id);
        return;
      }
      try {
        if (typeof callback === 'function') callback();
        if (!repeat) {
          timers.delete(id);
          settle(request.id, 'fulfilled', { timerId: id, fired: true });
        }
      } catch (error) {
        if (!repeat && pending.has(request.id)) settle(request.id, 'rejected', { timerId: id, message: error && error.message ? error.message : String(error) });
        throw error;
      }
    };
    const nativeId = repeat ? globalThis.setInterval(runner, ms) : globalThis.setTimeout(runner, ms);
    timers.set(id, Object.freeze({ id, nativeId, repeat: !!repeat, requestId: request.id, delayMs: ms, generation: timerGeneration }));
    return Object.freeze({ id, requestId: request.id, state: 'scheduled', repeat: !!repeat, delayMs: ms });
  }

  function cancelTimer(id) {
    const record = timers.get(id >>> 0);
    if (!record) return Object.freeze({ id: id >>> 0, cancelled: false });
    timers.delete(id >>> 0);
    if (record.repeat && typeof globalThis.clearInterval === 'function') globalThis.clearInterval(record.nativeId);
    if (!record.repeat && typeof globalThis.clearTimeout === 'function') globalThis.clearTimeout(record.nativeId);
    if (pending.has(record.requestId)) settle(record.requestId, 'fulfilled', { timerId: record.id, cancelled: true });
    const cancelRecord = enqueue(timerPlan && timerPlan.cancelHostCall ? timerPlan.cancelHostCall : 'timer.cancel', { timerId: record.id });
    settle(cancelRecord.id, 'fulfilled', { timerId: record.id, cancelled: true });
    return Object.freeze({ id: record.id, cancelled: true });
  }

  function clearRouteResources(reason = 'route-disposed', finalDispose = false) {
    if (disposed) return;
    if (finalDispose) disposed = true;
    for (const record of timers.values()) {
      if (record.repeat && typeof globalThis.clearInterval === 'function') globalThis.clearInterval(record.nativeId);
      if (!record.repeat && typeof globalThis.clearTimeout === 'function') globalThis.clearTimeout(record.nativeId);
    }
    timers.clear();
    for (const controller of abortControllers.values()) { try { controller.abort(reason); } catch (_) {} }
    abortControllers.clear();
    activeFetches = 0;
    for (const [id] of pending) {
      const record = pending.get(id); pending.delete(id); completed.set(id, Object.freeze({ ...record, state: 'rejected', value: { message: reason } }));
    }
    while (completed.size > maxCompleted) completed.delete(completed.keys().next().value);
  }

  function setRouteGeneration(generation, reason = 'route-generation-change') {
    const next = generation >>> 0;
    if (next === (activeGeneration >>> 0)) return activeGeneration;
    clearRouteResources(reason, false);
    activeGeneration = next;
    routeStartedAt = Date.now ? Date.now() : 0;
    hostCallsThisRoute = 0;
    fetchesThisRoute = 0;
    timersScheduledThisRoute = 0;
    return activeGeneration;
  }
  function resetRoute(reason = 'route-reset') { clearRouteResources(reason, false); activeGeneration = (activeGeneration + 1) >>> 0; routeStartedAt = Date.now ? Date.now() : 0; hostCallsThisRoute = 0; fetchesThisRoute = 0; timersScheduledThisRoute = 0; return activeGeneration; }
  function dispose(reason = 'queue-disposed') { clearRouteResources(reason, true); }

  function callQuickJs(entry, payload = {}) {
    const request = enqueue(quickJsPlan && quickJsPlan.callHostCall ? quickJsPlan.callHostCall : 'quickjs.call', { entry: String(entry || ''), payload });
    const mode = quickJsPlan && quickJsPlan.mode ? quickJsPlan.mode : 'planned-boundary';
    return Object.freeze({
      id: request.id,
      state: request.state,
      mode,
      scriptIsolation: quickJsPlan && quickJsPlan.scriptIsolation ? quickJsPlan.scriptIsolation : 'route-scoped',
      bytecodeInput: quickJsPlan && quickJsPlan.bytecodeInput ? quickJsPlan.bytecodeInput : 'planned',
      chunkMetadata: quickJsPlan && quickJsPlan.chunkMetadata ? quickJsPlan.chunkMetadata : '',
      engineMetadata: quickJsPlan && quickJsPlan.engineMetadata ? quickJsPlan.engineMetadata : '',
      fallbackPolicy: quickJsPlan && quickJsPlan.fallbackPolicy ? quickJsPlan.fallbackPolicy : 'host-js-isolated-wrapper',
    });
  }

  return Object.freeze({ enqueue, settle, snapshot, requestFetch, scheduleTimer, cancelTimer, callQuickJs, setRouteGeneration, resetRoute, dispose });
}

function createEventQueue(eventPlan) {
  const records = [];
  const maxRecords = eventPlan && eventPlan.maxRecords ? eventPlan.maxRecords : 256;
  const maxDispatchesPerRoute = eventPlan && eventPlan.maxDispatchesPerRoute ? eventPlan.maxDispatchesPerRoute : 1024;
  let dispatches = 0;
  function push(record = {}) {
    if (!eventPlan || !eventPlan.enabled) return Object.freeze({ queued: false, size: records.length });
    if (dispatches >= maxDispatchesPerRoute) throw new Error('Venom per-route event dispatch budget exceeded');
    if (records.length >= maxRecords) records.shift();
    dispatches++;
    const item = Object.freeze({
      id: records.length + 1,
      eventName: record && record.eventName ? String(record.eventName) : '',
      attrName: record && record.attrName ? String(record.attrName) : '',
      route: globalThis.location && globalThis.location.pathname ? String(globalThis.location.pathname) : '/',
      targetTag: record && record.element && record.element.tagName ? String(record.element.tagName).toLowerCase() : '',
      createdAt: Date.now ? Date.now() : 0,
    });
    records.push(item);
    return Object.freeze({ queued: true, size: records.length, record: item });
  }
  function snapshot() {
    return Object.freeze({ size: records.length, maxRecords, dispatches, maxDispatchesPerRoute, last: records.length ? records[records.length - 1] : null });
  }
  function clear() { const count = records.length; records.length = 0; dispatches = 0; return count; }
  return Object.freeze({ push, snapshot, clear });
}

function isInlineEventAttribute(name) {
  return /^on[a-z][a-z0-9_:-]*$/i.test(String(name || ''));
}

function eventNameFromAttribute(name) {
  return String(name || '').slice(2).toLowerCase();
}

function bindInlineEventAttribute(target, attrName, source, hostBridgePlan = null) {
  if (!target || !isInlineEventAttribute(attrName)) return false;
  const eventName = eventNameFromAttribute(attrName);
  if (!eventName) return false;
  if (typeof target.setAttribute === 'function') {
    target.setAttribute('data-venom-event-' + eventName, 'inline');
  }
  if (typeof target.addEventListener === 'function') {
    const bindBridge = globalThis.__venomRuntime || null;
    const boundGeneration = bindBridge && typeof bindBridge.currentRouteGeneration === 'function' ? bindBridge.currentRouteGeneration() : 0;
    target.addEventListener(eventName, function venomInlineEventHandler(event) {
      const bridge = globalThis.__venomRuntime || null;
      if (!bridge || typeof bridge.isRouteGenerationActive !== 'function' || !bridge.isRouteGenerationActive(boundGeneration)) return undefined;
      if (typeof bridge.dispatchEventBinding === 'function') {
        bridge.dispatchEventBinding({ event, element: target, eventName, source, attrName, routeGeneration: boundGeneration });
      }
      // Protected runtime never evaluates inline event source in the host runtime.
      // The source is treated as metadata and must be dispatched through QuickJS.
      return undefined;
    });
  }
  return true;
}


