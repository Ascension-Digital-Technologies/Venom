#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
contract=json.loads((root/'contracts/quickjs-wasm-abi.json').read_text())
assert contract['schema']=='venom.quickjs-wasm-abi.v1'
assert contract['runtimeAbi']==12 and contract['bridgeAbi']==1
assert contract['bytecodeFormat']=='VQJSBC03'
required=contract['requiredExports']
assert len(required)==len(set(required)) and 'memory' in required
subprocess.run([sys.executable, str(root/'tools/generate_quickjs_wasm_abi.py'), '--repo-root', str(root)], check=True)
header=(root/'src/generated/contracts/quickjs_wasm_abi.hpp').read_text()
js=(root/'src/generated/runtime/quickjs_wasm_abi.js').read_text()
for name in required:
    assert name in header, name
    assert name in js, name
proc=subprocess.run([sys.executable, str(root/'tools/inspect_embedded_quickjs_wasm.py'), '--repo-root', str(root), '--format', 'json'], text=True, capture_output=True, check=True)
report=json.loads(proc.stdout)
assert report['passed'] and not report['missing']
print('authoritative QuickJS/WASM ABI contract: PASS')
