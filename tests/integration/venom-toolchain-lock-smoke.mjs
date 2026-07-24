import assert from "node:assert/strict";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import process from "node:process";
import { writeToolchainLock, verifyToolchainLock } from "../../packages/venom/src/index.js";

const root = await fs.mkdtemp(path.join(os.tmpdir(), "venom-lock-"));
try {
  const binary = path.join(root, process.platform === "win32" ? "venom.cmd" : "venom");
  const first = process.platform === "win32"
    ? "@echo off\r\necho venom 3.0.0\r\n"
    : "#!/bin/sh\necho venom 3.0.0\n";
  await fs.writeFile(binary, first, { mode: 0o755 });
  const created = await writeToolchainLock(root, binary);
  assert.equal(created.value.version, "3.0.0");
  assert.equal((await verifyToolchainLock(root, binary)).ok, true);

  const changed = process.platform === "win32"
    ? "@echo off\r\necho venom 2.0.1\r\n"
    : "#!/bin/sh\necho venom 2.0.1\n";
  await fs.writeFile(binary, changed, { mode: 0o755 });
  const mismatch = await verifyToolchainLock(root, binary);
  assert.equal(mismatch.present, true);
  assert.equal(mismatch.ok, false);

  const refreshed = await writeToolchainLock(root, binary);
  assert.equal(refreshed.value.version, "2.0.1");
  assert.equal((await verifyToolchainLock(root, binary)).ok, true);
  console.log("venom toolchain lock smoke: PASS");
} finally {
  await fs.rm(root, { recursive: true, force: true });
}
