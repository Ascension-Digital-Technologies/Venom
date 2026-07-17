import assert from "node:assert/strict";

const calls = [];
globalThis.venom = {
  apiVersion: 1,
  ready: async function () { return this; },
  call: async (name, input, options) => { calls.push({ name, input, options }); if (name === "explode") { const error = new Error("failure"); error.code = "BYTECODE_REJECTED"; throw error; } return { name, input }; },
  batch: async items => Promise.all(items.map(item => globalThis.venom.call(item.name, item.input, item.options))),
  preload: async names => ({ ready: names, loaded: false }),
  status: () => ({ state: "ready", ready: true, disposed: false, exports: ["score", "explode"] }),
  dispose: reason => ({ state: "disposed", reason }),
  info: () => ({ runtimeApiVersion: 1, transport: "test", exports: ["score", "explode"] }),
  exports: {}
};

const sdk = await import(new URL("../../packages/runtime/src/index.js", import.meta.url));
const api = await sdk.initializeVenom();
assert.equal(api, globalThis.venom);
assert.deepEqual(await sdk.callProtected("score", { value: 21 }), { name: "score", input: { value: 21 } });
assert.equal(calls.length, 1);
assert.deepEqual(await sdk.batchProtected([{ name: "score", input: { value: 1 } }, { name: "score", input: { value: 2 } }]), [
  { name: "score", input: { value: 1 } }, { name: "score", input: { value: 2 } }
]);
assert.deepEqual(await sdk.preloadProtected(["score"]), { ready: ["score"], loaded: false });
assert.equal(sdk.getRuntimeStatus().state, "ready");
const client = sdk.createProtectedClient(["score"]);
assert.deepEqual(await client.score({ value: 7 }), { name: "score", input: { value: 7 } });
await assert.rejects(() => sdk.callProtected("explode", {}), error => error instanceof sdk.VenomRuntimeError && error.code === "BYTECODE_REJECTED" && error.phase === "call");
const disposed = await sdk.disposeRuntime("test complete");
assert.equal(disposed.state, "disposed");
await assert.rejects(() => sdk.callProtected("score", {}), error => error.code === "RUNTIME_DISPOSED");
assert.equal(sdk.isVenomRuntimeError(new sdk.VenomRuntimeError("BRIDGE_ERROR", "x")), true);
console.log("runtime SDK smoke: PASS");
