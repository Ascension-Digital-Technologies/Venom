import { useCallback, useEffect, useMemo, useRef, useState, useSyncExternalStore } from "react";
import {
  callProtected,
  getRuntimeStatus,
  initializeVenom,
  isVenomRuntimeError,
  preloadProtected
} from "@venom/runtime";

const RUNTIME_EVENTS = ["venom:ready", "venom:error", "venom:boot-ready", "venom:boot-error"];

function subscribeRuntime(listener) {
  if (typeof globalThis.addEventListener !== "function") return () => {};
  for (const event of RUNTIME_EVENTS) globalThis.addEventListener(event, listener);
  return () => {
    for (const event of RUNTIME_EVENTS) globalThis.removeEventListener(event, listener);
  };
}

function runtimeSnapshot() {
  return getRuntimeStatus();
}

const SERVER_SNAPSHOT = Object.freeze({
  state: "uninitialized",
  apiVersion: 1,
  ready: false,
  disposed: false,
  exports: Object.freeze([]),
  transport: null,
  lastError: null
});

export function useVenomRuntime(options = {}) {
  const status = useSyncExternalStore(subscribeRuntime, runtimeSnapshot, () => SERVER_SNAPSHOT);
  const [initializationError, setInitializationError] = useState(null);
  const autoInitialize = options.autoInitialize !== false;
  const timeout = options.timeout;

  useEffect(() => {
    if (!autoInitialize || status.ready || status.disposed) return undefined;
    const controller = new AbortController();
    initializeVenom({ timeout, signal: controller.signal }).catch(error => {
      if (error?.code !== "RUNTIME_ABORTED") setInitializationError(error);
    });
    return () => controller.abort();
  }, [autoInitialize, status.ready, status.disposed, timeout]);

  return useMemo(() => Object.freeze({
    ...status,
    error: initializationError || status.lastError || null,
    initialize: initializeVenom
  }), [status, initializationError]);
}

export function useProtectedCall(name, defaults = {}) {
  const operation = String(name || "");
  if (!operation) throw new TypeError("useProtectedCall requires a protected export name");

  const mounted = useRef(true);
  const activeController = useRef(null);
  const invocation = useRef(0);
  const [state, setState] = useState(() => ({
    pending: false,
    data: defaults.initialData,
    error: null,
    invocation: 0
  }));

  useEffect(() => {
    mounted.current = true;
    return () => {
      mounted.current = false;
      activeController.current?.abort();
    };
  }, []);

  const reset = useCallback(() => {
    activeController.current?.abort();
    invocation.current += 1;
    setState({ pending: false, data: defaults.initialData, error: null, invocation: invocation.current });
  }, [defaults.initialData]);

  const execute = useCallback(async (input, callOptions = {}) => {
    activeController.current?.abort();
    const controller = new AbortController();
    activeController.current = controller;
    const current = ++invocation.current;
    const externalSignal = callOptions.signal;
    const onAbort = () => controller.abort();
    externalSignal?.addEventListener?.("abort", onAbort, { once: true });
    setState(previous => ({ ...previous, pending: true, error: null, invocation: current }));
    try {
      const data = await callProtected(operation, input, {
        ...defaults,
        ...callOptions,
        signal: controller.signal
      });
      if (mounted.current && current === invocation.current) {
        setState({ pending: false, data, error: null, invocation: current });
      }
      defaults.onSuccess?.(data, input);
      return data;
    } catch (error) {
      if (mounted.current && current === invocation.current && error?.code !== "RUNTIME_ABORTED") {
        setState(previous => ({ ...previous, pending: false, error, invocation: current }));
      }
      defaults.onError?.(error, input);
      throw error;
    } finally {
      externalSignal?.removeEventListener?.("abort", onAbort);
      if (activeController.current === controller) activeController.current = null;
    }
  }, [operation, defaults]);

  const cancel = useCallback(() => activeController.current?.abort(), []);

  return useMemo(() => Object.freeze({
    execute,
    cancel,
    reset,
    pending: state.pending,
    data: state.data,
    error: state.error,
    invocation: state.invocation
  }), [execute, cancel, reset, state]);
}

export function useProtectedPreload(names, options = {}) {
  const requested = useMemo(() => Array.from(new Set((Array.isArray(names) ? names : [names]).filter(Boolean).map(String))).sort(), [names]);
  const key = requested.join("\u0000");
  const [state, setState] = useState({ pending: requested.length > 0, ready: [], error: null });

  useEffect(() => {
    if (!requested.length) {
      setState({ pending: false, ready: [], error: null });
      return undefined;
    }
    const controller = new AbortController();
    setState(previous => ({ ...previous, pending: true, error: null }));
    preloadProtected(requested, { ...options, signal: controller.signal })
      .then(() => setState({ pending: false, ready: requested, error: null }))
      .catch(error => {
        if (error?.code !== "RUNTIME_ABORTED") setState({ pending: false, ready: [], error });
      });
    return () => controller.abort();
  }, [key, options.timeout]);

  return Object.freeze(state);
}

export function createProtectedHooks(names) {
  const allowed = new Set(Array.from(names || [], String));
  return Object.freeze({
    useCall(name, defaults) {
      const selected = String(name || "");
      if (allowed.size && !allowed.has(selected)) throw new TypeError(`Unknown protected export: ${selected}`);
      return useProtectedCall(selected, defaults);
    },
    usePreload(options) {
      return useProtectedPreload(Array.from(allowed), options);
    },
    exports: Object.freeze(Array.from(allowed).sort())
  });
}

export { isVenomRuntimeError };
