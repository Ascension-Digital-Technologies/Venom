import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { protect, detectProject } from "../../packages/venom/src/index.js";

const root = await fs.mkdtemp(path.join(os.tmpdir(), "venom-"));
const fakeBin = path.join(root, process.platform === "win32" ? "venom.cmd" : "venom-fake");
const callFile = path.join(root, "venom-call.json");
try {
  await fs.writeFile(path.join(root, "package.json"), JSON.stringify({
    packageManager: "npm@10.0.0",
    scripts: { build: "node build.mjs" },
    dependencies: { react: "19.0.0", "react-dom": "19.0.0", vite: "7.0.0" }
  }, null, 2));
  await fs.writeFile(path.join(root, "build.mjs"), `import { promises as fs } from 'node:fs'; await fs.mkdir('dist',{recursive:true}); await fs.writeFile('dist/index.html','<!doctype html><div id="root"></div>');`);
  if (process.platform === "win32") {
    await fs.writeFile(fakeBin, `@echo off\r\nnode "%~dp0\\venom-fake.mjs" %*\r\n`);
    await fs.writeFile(path.join(root, "venom-fake.mjs"), `import { promises as fs } from 'node:fs'; await fs.writeFile(${JSON.stringify(callFile)}, JSON.stringify(process.argv.slice(2)));`);
  } else {
    await fs.writeFile(fakeBin, `#!/usr/bin/env node\nimport { promises as fs } from 'node:fs'; await fs.writeFile(${JSON.stringify(callFile)}, JSON.stringify(process.argv.slice(2)));\n`, { mode: 0o755 });
  }

  const detected = await detectProject(root);
  assert.equal(detected.framework, "react");
  assert.equal(detected.packageManager, "npm");
  const result = await protect({ root, venom: fakeBin, outDir: "protected", quiet: true });
  assert.equal(result.input, path.join(root, "dist"));
  const call = JSON.parse(await fs.readFile(callFile, "utf8"));
  assert.deepEqual(call.slice(0, 2), ["build", path.join(root, "dist")]);
  assert.equal(call[call.indexOf("--out") + 1], path.join(root, "protected"));
  assert.ok(await fs.stat(path.join(root, "dist", "index.html")));
  console.log("venom integration smoke: PASS");
} finally {
  await fs.rm(root, { recursive: true, force: true });
}
