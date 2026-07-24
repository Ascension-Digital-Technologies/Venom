import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { dev } from "../../packages/venom/src/index.js";

const root = await fs.mkdtemp(path.join(os.tmpdir(), "venom-dev-"));
const fakeBin = path.join(root, process.platform === "win32" ? "venom.cmd" : "venom-fake");
const callFile = path.join(root, "venom-calls.jsonl");
try {
  await fs.writeFile(path.join(root, "package.json"), JSON.stringify({
    packageManager: "npm@10.0.0",
    scripts: { build: "node build.mjs" },
    dependencies: { react: "19.0.0", "react-dom": "19.0.0", vite: "7.0.0" }
  }, null, 2));
  await fs.writeFile(path.join(root, "source.txt"), "first\n");
  await fs.writeFile(path.join(root, "build.mjs"), `import { promises as fs } from 'node:fs'; await fs.mkdir('dist',{recursive:true}); const source=await fs.readFile('source.txt','utf8'); await fs.writeFile('dist/index.html','<!doctype html><p>'+source.trim()+'</p>');`);
  const worker = `import { promises as fs } from 'node:fs'; await fs.appendFile(${JSON.stringify(callFile)}, JSON.stringify(process.argv.slice(2))+'\\n');`;
  if (process.platform === "win32") {
    await fs.writeFile(fakeBin, `@echo off\r\nnode "%~dp0\\venom-fake.mjs" %*\r\n`);
    await fs.writeFile(path.join(root, "venom-fake.mjs"), worker);
  } else {
    await fs.writeFile(fakeBin, `#!/usr/bin/env node\n${worker}\n`, { mode: 0o755 });
  }

  const result = await dev({ root, venom: fakeBin, outDir: "protected", quiet: true, once: true });
  assert.equal(result.buildCount, 1);
  assert.equal(result.lastError, null);
  assert.match(await fs.readFile(path.join(root, "dist", "index.html"), "utf8"), /first/);
  const calls = (await fs.readFile(callFile, "utf8")).trim().split(/\r?\n/).map(JSON.parse);
  assert.equal(calls.length, 1);
  assert.deepEqual(calls[0].slice(0, 2), ["build", path.join(root, "dist")]);
  console.log("venom dev smoke: PASS");
} finally {
  await fs.rm(root, { recursive: true, force: true });
}
