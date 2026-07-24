import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { resolveVenomBinary } from "../../packages/venom/src/index.js";

const root = await fs.mkdtemp(path.join(os.tmpdir(), "venom-bin-"));
const previous = process.env.VENOM_BIN;
try {
  const binDir = path.join(root, "node_modules", ".bin");
  await fs.mkdir(binDir, { recursive: true });
  const local = path.join(binDir, process.platform === "win32" ? "venom.cmd" : "venom");
  await fs.writeFile(local, process.platform === "win32" ? "@echo off\r\nexit /b 0\r\n" : "#!/bin/sh\nexit 0\n", { mode: 0o755 });

  delete process.env.VENOM_BIN;
  const project = await resolveVenomBinary({ root });
  assert.equal(project.source, "project");
  assert.equal(project.command, local);

  const explicitDir = path.join(root, "toolchain");
  await fs.mkdir(explicitDir);
  const explicit = path.join(explicitDir, process.platform === "win32" ? "venom.exe" : "venom");
  await fs.writeFile(explicit, process.platform === "win32" ? "" : "#!/bin/sh\nexit 0\n", { mode: 0o755 });
  const configured = await resolveVenomBinary({ root, config: { venom: path.relative(root, explicit) } });
  assert.equal(configured.source, "config");
  assert.equal(configured.command, explicit);

  process.env.VENOM_BIN = explicit;
  const environment = await resolveVenomBinary({ root });
  assert.equal(environment.source, "environment");
  assert.equal(environment.command, explicit);

  await assert.rejects(
    () => resolveVenomBinary({ root, venom: path.join(root, "missing-venom") }),
    /not found or is not executable/
  );
  console.log("venom binary resolution smoke: PASS");
} finally {
  if (previous === undefined) delete process.env.VENOM_BIN;
  else process.env.VENOM_BIN = previous;
  await fs.rm(root, { recursive: true, force: true });
}
