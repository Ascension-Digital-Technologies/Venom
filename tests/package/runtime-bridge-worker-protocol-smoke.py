from pathlib import Path
root = Path(__file__).resolve().parents[2]
worker = (root / 'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
header = (root / 'src/pipeline/include/venom/pipeline/js.hpp').read_text(encoding='utf-8')
build = (root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
js = (root / 'src/pipeline/js.cpp').read_text(encoding='utf-8')
js += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
for marker in [
    'BRIDGE_CANDIDATES', 'BRIDGE_CAPABILITIES', 'BRIDGE_MAGIC', 'decodeBridgeFrame',
    'binary-json-v2', 'MAX_BRIDGE_CONCURRENCY', 'unknown-capability', 'candidate-not-registered'
]:
    assert marker in worker, marker
assert 'bridge_candidate_ids' in header
assert 'make_worker_runtime_js(js_bridge.bridge_candidate_ids, js_bridge.bridge_registry_bytecode,' in build
assert 'bridge.bridge_candidate_ids.push_back' in js
print('runtime bridge worker protocol smoke: PASS')
