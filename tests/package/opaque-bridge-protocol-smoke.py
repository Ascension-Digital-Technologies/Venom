#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
worker = (root/'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
loader = (root/'src/compiler/pipeline/js.cpp').read_text(encoding='utf-8')
loader += (root / 'src/compiler/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
build = (root / 'src/compiler/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/compiler/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
for token in ['BRIDGE_INVOKE_OP', 'BRIDGE_CANCEL_OP', 'BRIDGE_RESULT_OP', 'BRIDGE_ERROR_OP']:
    if token not in worker:
        raise SystemExit(f'missing worker opcode: {token}')
for token in ['__venomInvokeOp', '__venomCancelOp', '__venomResultOp', '__venomErrorOp']:
    if token not in loader:
        raise SystemExit(f'missing loader opcode: {token}')
for label in ['"invoke"', '"cancel"', '"result"', '"error"']:
    if f'bridge_protocol_opcode(remote_vendor_options.bridge_id_salt, {label})' not in build:
        raise SystemExit(f'missing build-derived opcode: {label}')
if "postMessage([WORKER_PROTOCOL, BRIDGE_RESULT_OP" not in worker:
    raise SystemExit('worker result is not an array envelope')
if "postMessage([1,__venomInvokeOp" not in loader:
    raise SystemExit('loader invoke is not an array envelope')

for token in ['bridgeSession', 'bridgeCounter', 'candidateSlot']:
    if token not in worker and token not in loader:
        raise SystemExit(f'missing session-bound bridge token: {token}')
if 'String(candidate||'')' in loader or 'worker.postMessage([1,__venomInvokeOp,requestId,String(candidate||'')' in loader:
    raise SystemExit('browser bridge still sends candidate strings')
if 'BRIDGE_CANDIDATES[candidateSlot]' not in worker:
    raise SystemExit('worker does not resolve numeric candidate slots')
if 'counter <= bridgeCounter' not in worker:
    raise SystemExit('worker replay counter enforcement missing')
if '__venomInvokeProtectedById' not in loader:
    raise SystemExit('generated protected stubs are not name-routed')

legacy = ["type: 'invoke'", "type:'invoke'", "type: 'cancel'", "type:'cancel'", "type: 'bridge-result'", "type:'bridge-result'", "type: 'bridge-error'", "type:'bridge-error'"]
for token in legacy:
    if token in worker or token in loader:
        raise SystemExit(f'legacy readable bridge transport token remains: {token}')
print('opaque bridge protocol smoke: PASS')
