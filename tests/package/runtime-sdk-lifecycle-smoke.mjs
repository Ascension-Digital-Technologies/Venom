import assert from "node:assert/strict";

const bus = new EventTarget();
globalThis.addEventListener = bus.addEventListener.bind(bus);
globalThis.removeEventListener = bus.removeEventListener.bind(bus);
globalThis.dispatchEvent = bus.dispatchEvent.bind(bus);

delete globalThis.venom;
const sdk = await import(new URL("../../packages/runtime/src/index.js?lifecycle=1", import.meta.url));
const p1 = sdk.initializeVenom({ timeout: 1000 });
const p2 = sdk.initializeVenom({ timeout: 1000 });
const api = {
  apiVersion: 1,
  info: () => ({ runtimeApiVersion: 1, exports: ["score"] }),
  status: () => ({ state: "ready" }),
  call: async () => 42,
  ready: async function () { return this; },
  exports: {}
};
setTimeout(() => {
  globalThis.venom = api;
  globalThis.dispatchEvent(new CustomEvent("venom:ready", { detail: api }));
}, 20);
const [a, b] = await Promise.all([p1, p2]);
assert.equal(a, api);
assert.equal(b, api);
assert.equal(sdk.getRuntimeStatus().ready, true);

await sdk.disposeRuntime("lifecycle test");
assert.equal(sdk.getRuntimeStatus().state, "disposed");

const sdkAbort = await import(new URL("../../packages/runtime/src/index.js?lifecycle=2", import.meta.url));
delete globalThis.venom;
const controller = new AbortController();
controller.abort();
await assert.rejects(() => sdkAbort.initializeVenom({ signal: controller.signal }), error => error.code === "RUNTIME_ABORTED");

globalThis.venom = { ...api, apiVersion: 2, info: () => ({ runtimeApiVersion: 2, exports: [] }) };
const sdkMismatch = await import(new URL("../../packages/runtime/src/index.js?lifecycle=3", import.meta.url));
await assert.rejects(() => sdkMismatch.initializeVenom(), error => error.code === "ABI_MISMATCH" && error.recoverable === false);
console.log("runtime SDK lifecycle smoke: PASS");
