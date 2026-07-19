#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
loader = (root / 'src/pipeline/js.cpp').read_text(encoding='utf-8')
worker = (root / 'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
checks = {
    'browser pending-call limit': 'MAX_PUBLIC_PENDING_CALLS=32' in loader and 'pendingBridge.size>=MAX_PUBLIC_PENDING_CALLS' in loader,
    'browser JSON depth limit': 'MAX_PUBLIC_JSON_DEPTH=24' in loader and "exceeds the JSON depth limit" in loader,
    'browser JSON node limit': 'MAX_PUBLIC_JSON_NODES=16384' in loader and "exceeds the JSON node limit" in loader,
    'browser dangerous-key rejection': "key==='__proto__'||key==='prototype'||key==='constructor'" in loader,
    'worker JSON depth limit': 'MAX_BRIDGE_JSON_DEPTH = 24' in worker,
    'worker JSON node limit': 'MAX_BRIDGE_JSON_NODES = 16384' in worker,
    'worker dangerous-key rejection': "key === '__proto__' || key === 'prototype' || key === 'constructor'" in worker,
    'worker result-size gate': "resultSize > MAX_BRIDGE_MESSAGE_BYTES" in worker and "result-too-large" in worker,
}
failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items(): print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed: raise SystemExit('protected bridge limit checks failed: ' + ', '.join(failed))
print('protected bridge limits smoke: PASS')
