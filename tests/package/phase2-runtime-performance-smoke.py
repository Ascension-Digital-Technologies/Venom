#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
abi = (root/'tools/quickjs_release_abi.py').read_text()
engine = (root/'src/generated/runtime/quickjs_engine_module.cpp').read_text()
runtime = (root/'src/generated/runtime/runtime_js.cpp').read_text()
runtime += (root / 'src/runtime/templates/runtime.js').read_text(encoding='utf-8')
build = (root/'src/compiler/pipeline/build.cpp').read_text() + (root/'src/compiler/pipeline/build_package_metadata.cpp').read_text() + (root/'src/compiler/pipeline/build_runtime_metadata.cpp').read_text() + (root/'src/compiler/pipeline/build_runtime_audit_metadata.cpp').read_text() + (root/'src/compiler/pipeline/build_runtime_module_metadata.cpp').read_text()
sh = (root/'scripts/build-quickjs-wasm.sh').read_text()
assert 'venom_qjs_compact_result_ptr' in abi
assert abi.count("'venom_qjs_") <= 23
start = engine.index("const compactPtr = typeof e.venom_qjs_compact_result_ptr")
protected = engine[start:engine.index('    const resultPtr = e.venom_qjs_result_ptr()', start)]
assert 'JSON.parse' not in protected
assert 'new DataView' in protected and '32' in protected
assert 'VRPOL002' in runtime and 'VRPOL002' in build
assert 'MAXIMUM_MEMORY' in sh and '-O3' in sh
assert 'wasm-opt' in sh and '-O3' in sh
assert '--detect-features' in sh
assert '--enable-bulk-memory-opt' in sh
assert '--enable-nontrapping-float-to-int' in sh
print('phase2 runtime performance smoke passed')
