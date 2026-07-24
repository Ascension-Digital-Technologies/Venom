#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
text = (root / 'src/runtime/turbojs_runtime_scaffold.c').read_text(encoding='utf-8')
public = sorted(set(re.findall(r'VENOM_TJS_PUBLIC\("([A-Za-z0-9_]+)"\)', text)))
assert 55 <= len(public) <= 75, f'expected compact ABI (55..75), got {len(public)}'
for forbidden in (
    'venom_tjs_abi_table_ptr',
    'venom_tjs_checkpoint_create',
    'venom_tjs_snapshot_capture',
    'venom_tjs_replay_advance',
    'venom_tjs_upstream_object_record_ptr',
    'venom_tjs_policy_receipts_ptr',
):
    assert forbidden not in public, f'diagnostic export leaked: {forbidden}'
for required in (
    'venom_tjs_engine_abi',
    'venom_tjs_context_alloc',
    'venom_tjs_script_buffer_alloc',
    'venom_tjs_execute_bytecode',
    'venom_tjs_result_ptr',
    'venom_tjs_exception_ptr',
    'venom_tjs_upstream_turbojs_ready',
):
    assert required in public, f'missing execution export: {required}'
print(f'[venom] minimal public TurboJS ABI: PASS ({len(public)} exports)')

# Release builds must suppress export_name on the full diagnostic surface and
# rely on the generated 23-function EXPORTED_FUNCTIONS list instead.
assert 'VENOM_TJS_MINIMAL_EXPORTS' in text
ps1=(root/'tools'/'windows-scripts'/'build-turbojs-wasm.ps1').read_text(encoding='utf-8')
sh=(root/'tools'/'linux-scripts'/'build-turbojs-wasm.sh').read_text(encoding='utf-8')
validator=(root/'tools/wasm_exports.py').read_text(encoding='utf-8')
assert '-DVENOM_TJS_MINIMAL_EXPORTS=1' in ps1
assert '-DVENOM_TJS_MINIMAL_EXPORTS=1' in sh
for support in ('__indirect_function_table', '_emscripten_stack_restore', 'emscripten_stack_get_current'):
    assert support in validator, f'missing standalone WASM support export allowance: {support}'
print('[venom] release TurboJS ABI cutover policy: PASS')

analyzer=(root/'tools/analyze_dist.py').read_text(encoding='utf-8')
for required in ('turbojs_release_abi.py', '--require-json', '--exact-exports', '--release-runtime-values'):
    assert required in analyzer, f'dist analyzer is not aligned with the minimal release ABI: {required}'
assert "--require-from', str(repo / 'src' / 'runtime' / 'turbojs_runtime_scaffold.c')" not in analyzer
print('[venom] dist analyzer minimal release ABI policy: PASS')
