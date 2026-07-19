import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
required = [
    'scripts/linux/build-emsdk.sh',
    'scripts/windows/build-emsdk.bat',
    'scripts/windows/build-emsdk.bat',
    'tools/embed_wasm.py',
    'tools/quickjs_wasm_preflight.py',
    'tools/quickjs_wasm_cutover.py',
    'tools/setup_emscripten.py',
    'docs/getting-started/build-from-source.md',
]
for rel in required:
    path = root / rel
    if not path.exists():
        raise SystemExit(f'missing {rel}')

scaffold = (root / 'src/runtime/quickjs_runtime_scaffold.c').read_text(encoding='utf-8')
for marker in (
    'VENOM_ENABLE_UPSTREAM_QJS_WASM',
    '#include "quickjs.h"',
    'JS_NewRuntime',
    'JS_Eval',
    'JS_ExecutePendingJob',
    'venom_qjs_execute_bytecode',
    'g_real_engine_candidate = 1u',
):
    if marker not in scaffold:
        raise SystemExit(f'missing upstream QuickJS WASM marker {marker!r}')

exports = set(re.findall(r'VENOM_QJS_PUBLIC\("([A-Za-z0-9_]+)"\)', scaffold))
for name in (
    'venom_qjs_engine_abi',
    'venom_qjs_engine_version',
    'venom_qjs_context_alloc',
    'venom_qjs_context_free',
    'venom_qjs_script_buffer_alloc',
    'venom_qjs_execute_source',
    'venom_qjs_execute_bytecode',
    'venom_qjs_result_ptr',
    'venom_qjs_exception_ptr',
    'venom_qjs_real_engine_candidate',
):
    if name not in exports:
        raise SystemExit(f'missing ABI12 export {name}')
if len(exports) < 60:
    raise SystemExit(f'expected complete ABI12 export set, found only {len(exports)} exports')

script = (root/'tools'/'linux-scripts'/'build-quickjs-wasm.sh').read_text(encoding='utf-8')
for marker in ('emcc', 'VENOM_ENABLE_UPSTREAM_QJS_WASM=1', 'third_party/quickjs/quickjs.c',
    'tools/quickjs_wasm_preflight.py', 'tools/quickjs_wasm_cutover.py', 'quickjs-runtime.wasm',
    'quickjs_wasm_preflight.py', '--preflight-only'):
    if marker not in script:
        raise SystemExit(f'missing build script marker {marker!r}')

doc = (root / 'docs/getting-started/build-from-source.md').read_text(encoding='utf-8')
if 'verify-runtime --require-real-engine' not in doc or 'Cutover rule' not in doc:
    raise SystemExit('WASM artifact generation doc is missing the release gate/cutover rule')
