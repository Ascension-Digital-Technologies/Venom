#!/usr/bin/env python3
from pathlib import Path
import json
root = Path(__file__).resolve().parents[2]
abi_tool = (root/'tools/turbojs_release_abi.py').read_text()
abi_contract = json.loads((root/'contracts/turbojs-wasm-abi.json').read_text(encoding='utf-8'))
engine = (root/'src/generated/runtime/turbojs_engine_module.cpp').read_text()
runtime = (root/'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/generated/runtime/javascript/browser_runtime.js').read_text(encoding='utf-8')
build = (root/'src/pipeline/build.cpp').read_text() + (root/'src/pipeline/build_package_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_audit_metadata.cpp').read_text() + (root/'src/pipeline/build_runtime_module_metadata.cpp').read_text()
sh = (root/'tools'/'linux-scripts'/'build-turbojs-wasm.sh').read_text()
assert 'venom_tjs_compact_result_ptr' in abi_contract['requiredExports']
assert len([name for name in abi_contract['requiredExports'] if name != 'memory']) <= 23
assert "contract['requiredExports']" in abi_tool and "if n != 'memory'" in abi_tool
start = engine.index("    if (protectedRelease) {")
protected = engine[start:engine.index('    let report = null;', start)]
assert 'JSON.parse' not in protected
assert 'new DataView' in protected and '32' in protected
assert 'VRPOL002' in runtime and 'VRPOL002' in build
assert 'MAXIMUM_MEMORY' in sh and '-O3' in sh
assert 'wasm-opt' in sh and '-O3' in sh
assert '--detect-features' in sh
assert '--enable-bulk-memory-opt' in sh
assert '--enable-nontrapping-float-to-int' in sh
print('phase2 runtime performance smoke passed')
