#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
abi = (root/'tools/quickjs_release_abi.py').read_text()
engine = (root/'src/compiler/quickjs_engine_module.cpp').read_text()
runtime = (root/'src/compiler/runtime_js.cpp').read_text()
build = (root/'src/compiler/build.cpp').read_text()
sh = (root/'scripts/build-quickjs-wasm.sh').read_text()
assert 'venom_qjs_compact_result_ptr' in abi
assert abi.count("'venom_qjs_") <= 23
start = engine.index("const compactPtr = typeof e.venom_qjs_compact_result_ptr")
protected = engine[start:engine.index('    const resultPtr = e.venom_qjs_result_ptr()', start)]
assert 'JSON.parse' not in protected
assert 'new DataView' in protected and '32' in protected
assert 'VRPOL002' in runtime and 'VRPOL002' in build
assert 'VENOM_QJS_OPT_PROFILE' in sh and 'MAXIMUM_MEMORY' in sh
assert 'wasm-opt' in sh and ('-O4' in sh or 'WASM_OPT_LEVEL' in sh)
assert '--detect-features' in sh
assert '--enable-bulk-memory-opt' in sh
assert '--enable-nontrapping-float-to-int' in sh
print('phase2 runtime performance smoke passed')
