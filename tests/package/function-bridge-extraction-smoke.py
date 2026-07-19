from pathlib import Path
root = Path(__file__).resolve().parents[2]
js = (root/'src/pipeline/js.cpp').read_text(encoding='utf-8') + (root/'src/pipeline/js_planning.cpp').read_text(encoding='utf-8')
js += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
js += (root / 'src/pipeline/js_rewriting.cpp').read_text(encoding='utf-8')
worker = (root/'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
header = (root/'src/pipeline/include/venom/pipeline/js.hpp').read_text(encoding='utf-8')
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
