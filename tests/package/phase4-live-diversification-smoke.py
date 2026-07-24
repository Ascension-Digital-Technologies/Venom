#!/usr/bin/env python3
from pathlib import Path
import json
import re
root = Path(__file__).resolve().parents[2]

def project_version(text):
    match = re.search(r'project\(venom\s+VERSION\s+(\d+)\.(\d+)\.(\d+)', text, re.S)
    return tuple(map(int, match.groups())) if match else (0, 0, 0)
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
runtime = (root / 'src/generated/runtime/runtime_js.cpp').read_text(encoding='utf-8')
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
engine = (root / 'src/generated/runtime/turbojs_engine_module.cpp').read_text(encoding='utf-8')
scaffold = (root / 'src/runtime/turbojs_runtime_scaffold.c').read_text(encoding='utf-8')
abi = json.loads((root / 'contracts/turbojs-wasm-abi.json').read_text(encoding='utf-8'))
checks = {
    'v1.0.17+': project_version(cmake) >= (1, 0, 17),
    'release contract required': "missing release diversification contract" in runtime,
    'host permutation validation': 'invalid host-call diversification permutation' in runtime,
    'result permutation validation': 'invalid result diversification permutation' in runtime,
    'engine passes contract': 'diversification: activeReleaseDiversification' in runtime,
    'wasm install required': 'TurboJS release runtime lacks live diversification support' in engine,
    'wasm contract installer': 'venom_tjs_diversification_install' in scaffold,
    'wire host-call translation': 'host_call_logical_from_wire' in scaffold,
    'permuted compact result': 'g_compact_result[g_result_field_wire[i]]' in scaffold,
    'permuted result decoder': 'resultLogicalToWire' in engine,
    'release export allowlist': 'venom_tjs_diversification_install' in abi.get('requiredExports', []),
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('phase4 smoke failed: ' + ', '.join(failed))
print('phase4 live diversification smoke: PASS')
