from pathlib import Path
root = Path(__file__).resolve().parents[2]
js = (root/'src/compiler/js.cpp').read_text(encoding='utf-8')
worker = (root/'src/compiler/worker_runtime_js.cpp').read_text(encoding='utf-8')
build = (root/'src/compiler/build.cpp').read_text(encoding='utf-8')
header = (root/'src/compiler/js.hpp').read_text(encoding='utf-8')
for marker in [
    'apply_bridge_rewrites', 'all_external_calls_are_awaited',
    'resolve_liftable_function_dependencies', '__venomInvokeProtected',
    'bridge-rewrite-plan.json', 'awaited-calls-plus-pure-dependency-lifting',
]:
    assert marker in js or marker in build, marker
for marker in ['BRIDGE_REGISTRY_BYTECODE', 'fnv1a32', 'venom_qjs_execute_bytecode', 'protected bridge registry execution failed']:
    assert marker in worker, marker
assert 'bridge_registry_bytecode' in header
assert 'make_worker_runtime_js(js_bridge.bridge_candidate_ids, js_bridge.bridge_registry_bytecode' in build
print('function bridge extraction smoke: PASS')
