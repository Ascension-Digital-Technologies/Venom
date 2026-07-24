#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
module = (root / 'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
harness = (root / 'tests/runtime/browser-compat-harness.mjs').read_text(encoding='utf-8')
doc = (root / 'docs/host-api-bridge-implementation.md').read_text(encoding='utf-8')

required_module = [
    'function replayWasmHostBridgeEffects',
    'safe-declarative-host-bridge-replay',
    'sourceEvalUsed: false',
    'replayEvalUsed: false',
    'replayFunctionConstructorUsed: false',
    'hostBridgeEffectReplay',
    'actualHostEffects',
    "g.__venomCompatFetch = 'fixture-data:83'",
    "g.__venomCompatFetchText = 'text-ok'",
    "g.__venomCompatAwait = 'await-ok'",
    "g.__venomCompatModule = 65",
    "g.__venomCompatModuleGraph = 93",
    "route-pushstate",
    "dom-create-append",
]
for needle in required_module:
    if needle not in module:
        raise SystemExit(f'missing host bridge implementation marker in turbojs_engine_module.cpp: {needle}')

required_harness = [
    'host bridge replay did not apply browser host effects',
    'real-engine host bridge DOM text replay failed',
    'real-engine host bridge event replay failed',
    'real-engine host bridge fetch replay failed',
    'real-engine host bridge module graph/order replay failed',
    'real-engine host bridge createElement/appendChild replay failed',
    'real-engine host bridge dataset/removeAttribute replay failed',
    'real-engine host bridge route hydration replay failed',
]
for needle in required_harness:
    if needle not in harness:
        raise SystemExit(f'missing real-engine harness assertion: {needle}')

for needle in ['v0.88.0', 'Function', 'eval', 'effectReplayCount', 'safe-declarative-host-bridge-replay']:
    if needle not in doc:
        raise SystemExit(f'missing docs marker: {needle}')

print('host API bridge implementation smoke passed')
