import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { venom } from "../../packages/vite/src/index.js";

const workspace = await fs.mkdtemp(path.join(os.tmpdir(), "venom-react-vite-"));
const tmp = path.join(workspace, "examples", "react-app");
await fs.mkdir(tmp, { recursive: true });
await fs.writeFile(path.join(tmp, "package.json"), JSON.stringify({ dependencies: { react: "^19", "react-dom": "^19" } }));
await fs.mkdir(path.join(tmp, "src"));
await fs.writeFile(path.join(tmp, "src", "main.tsx"), "export const App = () => <main>React</main>;");
const dist = path.join(tmp, "dist");
await fs.mkdir(path.join(dist, "assets"), { recursive: true });
await fs.writeFile(path.join(dist, "index.html"), '<div id="root"></div><script type="module" src="/assets/app.js"></script>');
await fs.writeFile(path.join(dist, "assets", "app.js"), 'document.querySelector("#root").textContent="React";');
const log = path.join(tmp, "args.json");
const fakeDir = path.join(workspace, "build");
await fs.mkdir(fakeDir, { recursive: true });
const fake = path.join(fakeDir, process.platform === "win32" ? "venom.cmd" : "venom");
if (process.platform === "win32") {
  await fs.writeFile(fake, `@echo off\nnode -e "require('fs').writeFileSync(process.argv[1], JSON.stringify(process.argv.slice(2)))" "${log}" %*\n`);
} else {
  await fs.writeFile(fake, `#!/bin/sh\nnode -e 'require("fs").writeFileSync(process.argv[1], JSON.stringify(process.argv.slice(2)))' '${log}' "$@"\n`, { mode: 0o755 });
}
const plugin = venom({ root: tmp, outDir: "dist-venom" });
const messages = [];
await plugin.configResolved({ root: tmp, command: "build", build: { outDir: "dist" }, logger: { info: value => messages.push(value), warn() {}, error() {} } });
await plugin.buildStart();
assert.equal(plugin.api.getStatus().build, 0, "production React must not protect uncompiled TSX during buildStart");
await plugin.writeBundle();
const args = JSON.parse(await fs.readFile(log, "utf8"));
assert.equal(path.resolve(args[1]), dist, "Venom must ingest Vite's compiled output directory");
assert.equal(plugin.api.getStatus().framework, "react");
assert.equal(path.resolve(plugin.api.getStatus().input), dist);
assert.ok(messages.some(value => value.includes("React detected")));
assert.equal(path.resolve(plugin.api.options.venom), path.resolve(fake), "Vite plugin must discover a repository-local Venom compiler");
console.log("React Vite output integration: PASS");
