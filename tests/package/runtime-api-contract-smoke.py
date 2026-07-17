#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, sys
root = Path(__file__).resolve().parents[2]
contract = json.loads((root/'contracts/runtime-api.json').read_text(encoding='utf-8'))
assert contract['schema'] == 'VENOM_RUNTIME_API_V1'
assert contract['version'] == 1
assert contract['package'] == '@venom-js/runtime'
for method in ('initializeVenom','callProtected','preloadProtected','getRuntimeStatus','disposeRuntime'):
    assert method in contract['methods']
for code in ('RUNTIME_DISPOSED','UNKNOWN_EXPORT','ABI_MISMATCH','BYTECODE_REJECTED','PACKAGE_INTEGRITY'):
    assert code in contract['errorCodes']
sdk = (root/'packages/runtime/src/index.js').read_text(encoding='utf-8')
loader = (root/'src/compiler/pipeline/js.cpp').read_text(encoding='utf-8')
for token in ('VenomRuntimeError','initializeVenom','callProtected','disposeRuntime','createProtectedClient'):
    assert token in sdk
for token in ("apiVersion:1", "runtimeApiVersion:1", "venom:ready", "venom:error", "preloadProtected", "disposeProtectedRuntime"):
    assert token in loader, token
run = subprocess.run(['node', str(root/'tests/package/runtime-sdk-smoke.mjs')], cwd=root, text=True, capture_output=True)
if run.returncode:
    print(run.stdout); print(run.stderr, file=sys.stderr); raise SystemExit(run.returncode)
print(run.stdout.strip())
print('runtime API contract smoke: PASS')
