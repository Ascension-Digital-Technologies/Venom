export const VENOM_RUNTIME_API_VERSION = 1;

const STATE = Object.freeze({
  UNINITIALIZED: "uninitialized",
  INITIALIZING: "initializing",
  READY: "ready",
  ERROR: "error",
  DISPOSED: "disposed"
});

let localState = STATE.UNINITIALIZED;
let initialization = null;
let lastError = null;
let disposedReason = "";

export class VenomRuntimeError extends Error {
  constructor(code, message, options = {}) {
    super(String(message || "Venom runtime operation failed"), options.cause ? { cause: options.cause } : undefined);
    this.name = "VenomRuntimeError";
    this.code = String(code || "BRIDGE_ERROR");
    this.phase = String(options.phase || "runtime");
    this.recoverable = options.recoverable !== false;
    this.details = options.details && typeof options.details === "object" ? Object.freeze({ ...options.details }) : undefined;
  }
}

export function isVenomRuntimeError(value) {
  return value instanceof VenomRuntimeError || !!(value && value.name === "VenomRuntimeError" && typeof value.code === "string");
}

function asRuntimeError(error, fallbackCode, phase, recoverable = true) {
  if (isVenomRuntimeError(error)) return error;
  const message = error && error.message ? error.message : String(error || "Unknown Venom runtime failure");
  const originalCode = error && typeof error.code === "string" ? error.code : "";
  let code = originalCode || fallbackCode;
  if (error?.name === "AbortError") code = "RUNTIME_ABORTED";
  else if (originalCode === "TIMEOUT" || error?.name === "VenomTimeoutError") code = "EXECUTION_TIMEOUT";
  else if (/memory|heap|allocation/i.test(message)) code = "MEMORY_LIMIT";
  else if (/integrity|hash|attestation/i.test(message)) code = "PACKAGE_INTEGRITY";
  else if (/bytecode/i.test(message) && /invalid|reject|mismatch/i.test(message)) code = "BYTECODE_REJECTED";
  else if (/network|fetch|http/i.test(message)) code = "NETWORK_FAILURE";
  return new VenomRuntimeError(code, message, { phase, recoverable, cause: error, details: originalCode && originalCode !== code ? { originalCode } : undefined });
}

function publicApi() {
  const api = globalThis.venom;
  return api && typeof api === "object" ? api : null;
}

function abortError(phase = "initialize") {
  return new VenomRuntimeError("RUNTIME_ABORTED", "Venom runtime operation was aborted", { phase, recoverable: true });
}

function disposedError(phase = "runtime") {
  return new VenomRuntimeError("RUNTIME_DISPOSED", disposedReason || "Venom runtime has been disposed", { phase, recoverable: false });
}

function assertCompatible(api) {
  const info = typeof api.info === "function" ? api.info() : {};
  const apiVersion = Number(api.apiVersion ?? info.runtimeApiVersion ?? 1);
  if (apiVersion !== VENOM_RUNTIME_API_VERSION) {
    throw new VenomRuntimeError("ABI_MISMATCH", `Venom runtime API ${apiVersion} is incompatible with SDK API ${VENOM_RUNTIME_API_VERSION}`, {
      phase: "initialize", recoverable: false, details: { expected: VENOM_RUNTIME_API_VERSION, actual: apiVersion }
    });
  }
  return api;
}

export async function initializeVenom(options = {}) {
  if (localState === STATE.DISPOSED) throw disposedError("initialize");
  const existing = publicApi();
  if (existing) {
    localState = STATE.READY;
    return assertCompatible(existing);
  }
  if (initialization) return initialization;
  localState = STATE.INITIALIZING;
  const timeout = Math.max(1, Math.min(Number(options.timeout ?? 15000), 120000));
  const signal = options.signal;
  initialization = new Promise((resolve, reject) => {
    let settled = false;
    let timer;
    const finish = (error, api) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      globalThis.removeEventListener?.("venom:ready", onReady);
      globalThis.removeEventListener?.("venom:error", onError);
      signal?.removeEventListener?.("abort", onAbort);
      if (error) {
        lastError = error;
        localState = STATE.ERROR;
        initialization = null;
        reject(error);
      } else {
        localState = STATE.READY;
        lastError = null;
        resolve(api);
      }
    };
    const onReady = event => {
      try { finish(null, assertCompatible(event?.detail || publicApi())); }
      catch (error) { finish(asRuntimeError(error, "ABI_MISMATCH", "initialize", false)); }
    };
    const onError = event => finish(asRuntimeError(event?.detail || event, "RUNTIME_INITIALIZATION_FAILED", "initialize", false));
    const onAbort = () => finish(abortError("initialize"));
    if (signal?.aborted) return onAbort();
    globalThis.addEventListener?.("venom:ready", onReady, { once: true });
    globalThis.addEventListener?.("venom:error", onError, { once: true });
    signal?.addEventListener?.("abort", onAbort, { once: true });
    timer = setTimeout(() => finish(new VenomRuntimeError("RUNTIME_TIMEOUT", `Venom runtime did not become ready within ${timeout}ms`, { phase: "initialize", recoverable: true })), timeout);
    queueMicrotask(() => {
      const api = publicApi();
      if (api) onReady({ detail: api });
    });
  });
  return initialization;
}

export async function callProtected(name, input, options = {}) {
  if (localState === STATE.DISPOSED) throw disposedError("call");
  const api = await initializeVenom(options);
  if (typeof api.call !== "function") throw new VenomRuntimeError("ABI_MISMATCH", "Venom runtime does not expose call()", { phase: "call", recoverable: false });
  try { return await api.call(String(name), input, options); }
  catch (error) { throw asRuntimeError(error, "BRIDGE_ERROR", "call"); }
}

export async function batchProtected(calls, options = {}) {
  if (localState === STATE.DISPOSED) throw disposedError("batch");
  const api = await initializeVenom(options);
  if (!Array.isArray(calls)) throw new VenomRuntimeError("INVALID_BATCH", "Protected batch must be an array", { phase: "batch" });
  try {
    if (typeof api.batch === "function") return await api.batch(calls, options);
    return await Promise.all(calls.map(item => callProtected(item.name, item.input, item.options || options)));
  } catch (error) { throw asRuntimeError(error, "BRIDGE_ERROR", "batch"); }
}

export async function preloadProtected(names, options = {}) {
  const api = await initializeVenom(options);
  const requested = Array.isArray(names) ? names.map(String) : [String(names)];
  try {
    if (typeof api.preload === "function") return await api.preload(requested, options);
    const available = new Set((typeof api.info === "function" ? api.info().exports : []) || []);
    const missing = requested.filter(name => !available.has(name));
    if (missing.length) throw new VenomRuntimeError("UNKNOWN_EXPORT", `Unknown protected exports: ${missing.join(", ")}`, { phase: "preload", details: { missing } });
    return Object.freeze({ ready: requested.slice(), loaded: false });
  } catch (error) { throw asRuntimeError(error, "BRIDGE_ERROR", "preload"); }
}

export function getRuntimeStatus() {
  const api = publicApi();
  const info = api && typeof api.info === "function" ? api.info() : null;
  const nativeStatus = api && typeof api.status === "function" ? api.status() : null;
  return Object.freeze({
    state: localState === STATE.DISPOSED ? STATE.DISPOSED : (nativeStatus?.state || localState),
    apiVersion: Number(api?.apiVersion ?? info?.runtimeApiVersion ?? VENOM_RUNTIME_API_VERSION),
    ready: !!api && localState === STATE.READY,
    disposed: localState === STATE.DISPOSED,
    exports: Object.freeze([...(info?.exports || [])]),
    transport: info?.transport || null,
    lastError
  });
}

export async function disposeRuntime(reason = "SDK disposal") {
  if (localState === STATE.DISPOSED) return getRuntimeStatus();
  disposedReason = String(reason || "SDK disposal");
  const api = publicApi();
  try {
    if (api && typeof api.dispose === "function") await api.dispose(disposedReason);
    else if (globalThis.__venomRuntime && typeof globalThis.__venomRuntime.teardownRoute === "function") globalThis.__venomRuntime.teardownRoute(disposedReason);
  } catch (error) {
    lastError = asRuntimeError(error, "BRIDGE_ERROR", "dispose", false);
  }
  localState = STATE.DISPOSED;
  initialization = null;
  return getRuntimeStatus();
}

export function createProtectedClient(names) {
  const allow = names ? new Set(Array.from(names, String)) : null;
  return new Proxy(Object.create(null), {
    get(_target, property) {
      if (typeof property !== "string" || (allow && !allow.has(property))) return undefined;
      return (input, options) => callProtected(property, input, options);
    },
    has(_target, property) { return typeof property === "string" && (!allow || allow.has(property)); },
    ownKeys() { return allow ? Array.from(allow).sort() : getRuntimeStatus().exports; },
    getOwnPropertyDescriptor(_target, property) { return typeof property === "string" && (!allow || allow.has(property)) ? { enumerable: true, configurable: true } : undefined; }
  });
}

export const venomRuntime = Object.freeze({
  initialize: initializeVenom,
  call: callProtected,
  batch: batchProtected,
  preload: preloadProtected,
  status: getRuntimeStatus,
  dispose: disposeRuntime,
  createClient: createProtectedClient
});

export default venomRuntime;
