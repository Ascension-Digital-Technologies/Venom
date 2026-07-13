from pathlib import Path
root = Path(__file__).resolve().parents[2]
worker = (root / 'src/compiler/worker_runtime_js.cpp').read_text(encoding='utf-8')
header = (root / 'src/compiler/js.hpp').read_text(encoding='utf-8')
build = (root / 'src/compiler/build.cpp').read_text(encoding='utf-8')
js = (root / 'src/compiler/js.cpp').read_text(encoding='utf-8')
for marker in [
    'BRIDGE_CANDIDATES', "m.type === 'invoke'", "m.type === 'bridge-capabilities'",
    'json-value-v1', 'MAX_BRIDGE_CONCURRENCY', 'unknown-candidate', 'candidate-not-registered'
]:
    assert marker in worker, marker
assert 'bridge_candidate_ids' in header
assert 'make_worker_runtime_js(js_bridge.bridge_candidate_ids, js_bridge.bridge_registry_bytecode)' in build
assert 'bridge.bridge_candidate_ids.push_back' in js
print('realm bridge worker protocol smoke: PASS')
