import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { venom } from "../../packages/vite/src/index.js";

const tmp = await fs.mkdtemp(path.join(os.tmpdir(), "venom-vite-"));
const site = path.join(tmp, "site");
const log = path.join(tmp, "calls.log");
await fs.mkdir(site, { recursive: true });
await fs.writeFile(path.join(site, "index.html"), "<script type=module src=/src.js></script>");
await fs.writeFile(path.join(site, "src.js"), "export const answer = 42;");
const fake = path.join(tmp, process.platform === "win32" ? "venom.cmd" : "venom");
if (process.platform === "win32") {
  await fs.writeFile(fake, `@echo off\necho call>>"${log}"\nexit /b 0\n`);
} else {
  await fs.writeFile(fake, `#!/bin/sh\necho call >> "${log}"\nexit 0\n`, { mode: 0o755 });
}

const plugin = venom({ root: tmp, site: "site", outDir: "protected", profile: "prod", venom: fake, debounceMs: 10 });
const messages = [];
plugin.configResolved({ logger: { info: message => messages.push(message), warn: message => messages.push(message), error: message => messages.push(message) } });
assert.equal(plugin.resolveId("virtual:venom-status"), "\0virtual:venom-status");
assert.match(plugin.load("\0virtual:venom-status"), /state/);
await plugin.buildStart();
assert.equal(plugin.api.getStatus().state, "ready");
assert.equal(plugin.api.getStatus().build, 1);
await plugin.api.build("unchanged");
assert.equal(plugin.api.getStatus().build, 1, "unchanged content must not rebuild");
await fs.writeFile(path.join(site, "src.js"), "export const answer = 43;");
await plugin.api.build("changed");
assert.equal(plugin.api.getStatus().build, 2);
const calls = (await fs.readFile(log, "utf8")).trim().split(/\r?\n/);
assert.equal(calls.length, 2);

let middleware;
plugin.configureServer({ middlewares: { use(route, fn) { assert.equal(route, "/__venom/status"); middleware = fn; } } });
const headers = {};
let body = "";
const res = { statusCode: 0, setHeader(k, v) { headers[k] = v; }, end(value) { body = value; } };
middleware({}, res);
assert.equal(res.statusCode, 200);
assert.equal(headers["Cache-Control"], "no-store");
assert.equal(JSON.parse(body).state, "ready");
console.log("vite plugin smoke passed");
