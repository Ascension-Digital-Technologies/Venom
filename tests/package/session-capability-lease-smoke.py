#!/usr/bin/env python3
from pathlib import Path
import sys
root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
worker = (root/'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
loader = (root/'src/compiler/pipeline/js.cpp').read_text(encoding='utf-8')
planning = (root/'src/compiler/pipeline/js_planning.cpp').read_text(encoding='utf-8')
required_worker = ['deriveLeaseCapability', 'rotateSessionOpcode', 'refreshSessionProtocol', 'sessionInvokeOp', 'sessionCancelOp', 'sessionResultOp', 'sessionErrorOp', 'counter <= bridgeCounter']
required_loader = ['leaseCapabilityForSlot', 'rotateSessionOpcode', 'sessionInvokeOp', 'sessionCancelOp', 'sessionResultOp', 'sessionErrorOp', "binary-capability-v3-leased"]
for token in required_worker:
    assert token in worker, f'missing worker lease token: {token}'
for token in required_loader:
    assert token in loader, f'missing loader lease token: {token}'
assert 'binary-capability-v3-leased' in planning
assert 'sendFrame(sessionInvokeOp,counter,capability' in loader
assert 'deriveLeaseCapability(BRIDGE_CAPABILITIES[i]' in worker
assert 'BRIDGE_CAPABILITIES.indexOf(frame.capability' not in worker
print('session capability lease smoke: PASS')
