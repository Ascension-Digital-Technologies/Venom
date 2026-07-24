#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
worker = ((root/'src/generated/runtime/worker_runtime_js.cpp').read_text() + (root/'src/generated/runtime/worker_runtime_template.hpp').read_text())
runtime = (root/'src/generated/runtime/javascript/browser_runtime.js').read_text()
required_worker = ['BRIDGE_INTEGRITY_SEAL', 'bridgeIntegritySeal()', 'assertBridgeIntegrity()', 'bridge integrity seal mismatch']
required_runtime = ['activeRuntimeIntegritySeal', 'computeRuntimeIntegritySeal', 'assertRuntimeIntegrity', 'runtime integrity seal mismatch', 'activeRuntimeOpcodeMap']
missing = [x for x in required_worker if x not in worker] + [x for x in required_runtime if x not in runtime]
if missing:
    raise SystemExit('runtime integrity seal smoke missing: ' + ', '.join(missing))
if runtime.count('assertRuntimeIntegrity(activeRuntimeOpcodeMap)') < 1:
    raise SystemExit('runtime execution path does not re-check the active integrity seal')
print('runtime integrity seal smoke: PASS')
