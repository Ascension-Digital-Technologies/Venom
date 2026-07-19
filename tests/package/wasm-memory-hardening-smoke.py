#!/usr/bin/env python3
from pathlib import Path
import sys
root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
worker = ((root/'src/generated/runtime/worker_runtime_js.cpp').read_text() + (root/'src/generated/include/venom/generated/runtime/worker_runtime_template.hpp').read_text())
engine = (root/'src/generated/runtime/quickjs_engine_module.cpp').read_text()
required_worker = ['secureMemoryRange','secureMemoryWrite','secureMemoryRead','secureMemoryZero','envelope.fill(0)','resultBytes.fill(0)']
required_engine = ['memoryRange','readWasmBytes','zeroWasmRange','QuickJS script input']
for marker in required_worker:
    assert marker in worker, f'missing worker memory hardening marker: {marker}'
for marker in required_engine:
    assert marker in engine, f'missing engine memory hardening marker: {marker}'
assert 'new Uint8Array(e.memory.buffer, ptr, envelope.byteLength).set(envelope)' not in worker
assert 'memory.set(bytes, ptr)' not in engine
print('WASM memory hardening smoke: PASS')
