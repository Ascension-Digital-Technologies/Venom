import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { diagnose, initProject, detectProject } from "../../packages/venom/src/index.js";

const root = await fs.mkdtemp(path.join(os.tmpdir(), "venom-init-"));
try {
  const localBinDir = path.join(root, "node_modules", ".bin");
  await fs.mkdir(localBinDir, { recursive: true });
  const localVenom = path.join(localBinDir, process.platform === "win32" ? "venom.cmd" : "venom");
  await fs.writeFile(localVenom, process.platform === "win32" ? "@echo off\r\nexit /b 0\r\n" : "#!/bin/sh\nexit 0\n", { mode: 0o755 });
  await fs.writeFile(path.join(root, "package.json"), JSON.stringify({
    scripts: { build: "vite build" },
    dependencies: { react: "19.0.0", "react-dom": "19.0.0", vite: "7.0.0" }
  }, null, 2));
  const initialized = await initProject({ root });
  assert.equal(initialized.framework, "react");
  assert.equal(initialized.packageUpdated, true);
  const config = JSON.parse(await fs.readFile(path.join(root, "venom.config.json"), "utf8"));
  assert.equal(config.input, "dist");
  assert.equal(config.output, "dist-venom");
  assert.equal(config.build, true);
  const pkg = JSON.parse(await fs.readFile(path.join(root, "package.json"), "utf8"));
  assert.equal(pkg.scripts["build:protected"], "venom");
  const detected = await detectProject(root);
  assert.equal(detected.configFile, path.join(root, "venom.config.json"));
  assert.equal(detected.config.input, "dist");
  const report = await diagnose({ root });
  assert.equal(report.ok, true);
  assert.equal(report.binary?.source, "project");
  assert.equal(report.binary?.command, localVenom);
  assert.ok(report.findings.some(item => item.code === "configured-input-missing"));
  await assert.rejects(() => initProject({ root }), /Configuration already exists/);
  console.log("venom init smoke: PASS");
} finally {
  await fs.rm(root, { recursive: true, force: true });
}
