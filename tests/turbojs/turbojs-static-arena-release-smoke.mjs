#!/usr/bin/env node
import fs from 'node:fs';

const runtimePath = new URL('../../runtime/turbojs-runtime.wasm', import.meta.url);
const bytes = fs.readFileSync(runtimePath);
const text = bytes.toString('latin1');
if (!text.includes('VENOM_TJS_RUNTIME_ABI_V13') || !text.includes('runtime_abi=13')) {
  throw new Error('canonical TurboJS runtime is not ABI 13');
}

const module = new WebAssembly.Module(bytes);
const imports = {
  env: { emscripten_notify_memory_growth() {} },
  wasi_snapshot_preview1: {
    fd_close() { return 0; },
    clock_time_get() { return 0; },
    environ_sizes_get() { return 0; },
    environ_get() { return 0; },
    fd_write() { return 0; },
  },
};
const instance = new WebAssembly.Instance(module, imports);
const release = instance.exports.venom_tjs_script_buffer_free;
if (typeof release !== 'function') throw new Error('script-buffer compatibility export is missing');
for (let index = 0; index < 10000; ++index) {
  const context = (0xdeadbeef + index) >>> 0;
  const pointer = (247024 + index) >>> 0;
  if (release(context, pointer) !== 1) throw new Error('compatibility release is state-dependent');
}
console.log('TurboJS static-arena release smoke: 10,000 adversarial calls passed');
