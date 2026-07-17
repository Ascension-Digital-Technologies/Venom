#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
runtime = (root/'src/runtime/quickjs_runtime_scaffold.c').read_text(encoding='utf-8')
worker = (root/'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
release = (root/'tools/quickjs_release_abi.py').read_text(encoding='utf-8')
for name in (
    'venom_qjs_bridge_abi','venom_qjs_bridge_input_alloc','venom_qjs_bridge_input_capacity',
    'venom_qjs_bridge_invoke','venom_qjs_bridge_result_ptr','venom_qjs_bridge_result_size',
    'venom_qjs_bridge_release'):
    assert f'VENOM_QJS_PUBLIC("{name}")' in runtime, name
    assert name in worker, name
    assert name in release, name
for marker in (
    '__venomProtectedBridge','JS_ParseJSON','JS_Call','JS_JSONStringify',
    'candidate-not-registered','json-value-v1',
    'sessionResultOp','sessionErrorOp','encodeBridgeFrame','decodeBridgeFrame',
    'e.venom_qjs_bridge_release(quickJsContext'):
    assert marker in runtime or marker in worker, marker
assert 'executor-not-linked' not in worker
assert "'executor-not-ready'" in worker
assert 'if (!quickJsInstance || !quickJsContext)' in worker
print('[venom] QuickJS protected function bridge ABI: PASS')
