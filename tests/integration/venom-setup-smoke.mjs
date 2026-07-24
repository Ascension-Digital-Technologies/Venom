import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import process from "node:process";
import { setupCompiler, resolveVenomBinary } from "../../packages/venom/src/index.js";

const root = await fs.mkdtemp(path.join(os.tmpdir(), "venom-setup-"));
try {
  const project = path.join(root, "app");
  const source = path.join(root, "venom-source");
  await fs.mkdir(path.join(source, "src", "cli"), { recursive: true });
  await fs.mkdir(project, { recursive: true });
  await fs.writeFile(path.join(source, "CMakeLists.txt"), "cmake_minimum_required(VERSION 3.20)\n");
  await fs.writeFile(path.join(source, "src", "cli", "main.cpp"), "int main(){return 0;}\n");

  const fake = path.join(root, process.platform === "win32" ? "fake-cmake.cmd" : "fake-cmake");
  if (process.platform === "win32") {
    await fs.writeFile(fake, `@echo off\r\nnode "${path.join(root, "fake-cmake.mjs")}" %*\r\n`);
  } else {
    await fs.writeFile(fake, `#!/bin/sh\nexec node "${path.join(root, "fake-cmake.mjs")}" "$@"\n`, { mode: 0o755 });
  }
  await fs.writeFile(path.join(root, "fake-cmake.mjs"), `
import { promises as fs } from "node:fs";
import path from "node:path";
const args = process.argv.slice(2);
const b = args.indexOf("-B");
if (b >= 0) { await fs.mkdir(args[b + 1], { recursive: true }); process.exit(0); }
const build = args.indexOf("--build");
if (build >= 0) {
  const dir = args[build + 1];
  await fs.mkdir(dir, { recursive: true });
  const output = path.join(dir, process.platform === "win32" ? "venom.exe" : "venom");
  await fs.writeFile(output, process.platform === "win32" ? "" : "#!/bin/sh\\nexit 0\\n", { mode: 0o755 });
  process.exit(0);
}
process.exit(2);
`);

  const result = await setupCompiler({ root: project, source, buildDir: "build/auto", cmake: fake, quiet: true });
  assert.equal(result.sourceRoot, source);
  assert.equal(result.binary, path.join(source, "build", "auto", process.platform === "win32" ? "venom.exe" : "venom"));
  const config = JSON.parse(await fs.readFile(path.join(project, "venom.config.json"), "utf8"));
  assert.equal(path.resolve(project, config.venom), result.binary);
  const resolved = await resolveVenomBinary({ root: project, config });
  assert.equal(resolved.command, result.binary);
  assert.equal(resolved.source, "config");

  await assert.rejects(() => setupCompiler({ root: project, source: path.join(root, "missing"), cmake: fake }), /source checkout was not found/);
  console.log("venom setup smoke: PASS");
} finally {
  await fs.rm(root, { recursive: true, force: true });
}
