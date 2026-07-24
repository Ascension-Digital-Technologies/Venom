#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
engine = (root / 'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
app = (root / 'examples/javascript-playground/browser/app.js').read_text(encoding='utf-8')

required_engine = [
    "chunk.bridgeCandidate",
    "venom_tjs_bridge_input_alloc",
    "venom_tjs_bridge_invoke",
    "venom_tjs_bridge_result_ptr",
    "venom_tjs_bridge_result_size",
    "venom_tjs_bridge_release",
]
for token in required_engine:
    assert token in engine, token

# The minimal ABI path must not unconditionally invoke removed result exports.
assert "else if (typeof e.venom_tjs_result_ptr === 'function'" in engine
assert engine.index('if (chunk.bridgeCandidate)') < engine.index("else if (typeof e.venom_tjs_result_ptr === 'function'")

required_app = [
    '__venomProtectedBridge',
    'bridgeCandidate: candidate',
    'bridgeArgs: [input]',
    'consoleCapture',
]
for token in required_app:
    assert token in app, token

print('javascript playground minimal result ABI: PASS')
