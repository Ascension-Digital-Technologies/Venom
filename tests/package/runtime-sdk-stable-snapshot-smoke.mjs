import assert from "node:assert/strict";
import { pathToFileURL } from "node:url";
import path from "node:path";

const root = path.resolve(import.meta.dirname, "../..");
const sdk = await import(pathToFileURL(path.join(root, "packages/runtime/src/index.js")));

const first = sdk.getRuntimeStatus();
const second = sdk.getRuntimeStatus();
assert.strictEqual(second, first, "unchanged runtime status must preserve object identity for useSyncExternalStore");
assert.strictEqual(second.exports, first.exports, "unchanged export lists must preserve object identity");

console.log("runtime-sdk-stable-snapshot-smoke: PASS");
