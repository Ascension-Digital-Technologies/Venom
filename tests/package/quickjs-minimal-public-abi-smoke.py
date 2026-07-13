#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
text = (root / 'src/runtime/quickjs_runtime_scaffold.c').read_text(encoding='utf-8')
public = sorted(set(re.findall(r'VENOM_QJS_PUBLIC\("([A-Za-z0-9_]+)"\)', text)))
assert 55 <= len(public) <= 75, f'expected compact ABI (55..75), got {len(public)}'
for forbidden in (
    'venom_qjs_abi_table_ptr',
    'venom_qjs_checkpoint_create',
    'venom_qjs_snapshot_capture',
    'venom_qjs_replay_advance',
    'venom_qjs_upstream_object_record_ptr',
    'venom_qjs_policy_receipts_ptr',
):
    assert forbidden not in public, f'diagnostic export leaked: {forbidden}'
for required in (
    'venom_qjs_engine_abi',
    'venom_qjs_context_alloc',
    'venom_qjs_script_buffer_alloc',
    'venom_qjs_execute_bytecode',
    'venom_qjs_result_ptr',
    'venom_qjs_exception_ptr',
    'venom_qjs_upstream_quickjs_ready',
):
    assert required in public, f'missing execution export: {required}'
print(f'[venom] minimal public QuickJS ABI: PASS ({len(public)} exports)')

# Release builds must suppress export_name on the full diagnostic surface and
# rely on the generated 23-function EXPORTED_FUNCTIONS list instead.
assert 'VENOM_QJS_MINIMAL_EXPORTS' in text
ps1=(root/'scripts/build-quickjs-wasm.ps1').read_text(encoding='utf-8')
sh=(root/'scripts/build-quickjs-wasm.sh').read_text(encoding='utf-8')
validator=(root/'tools/wasm_exports.py').read_text(encoding='utf-8')
assert '-DVENOM_QJS_MINIMAL_EXPORTS=1' in ps1
assert '-DVENOM_QJS_MINIMAL_EXPORTS=1' in sh
for support in ('__indirect_function_table', '_emscripten_stack_restore', 'emscripten_stack_get_current'):
    assert support in validator, f'missing standalone WASM support export allowance: {support}'
print('[venom] release QuickJS ABI cutover policy: PASS')

analyzer=(root/'tools/analyze_dist.py').read_text(encoding='utf-8')
for required in ('quickjs_release_abi.py', '--require-json', '--exact-exports', '--release-runtime-values'):
    assert required in analyzer, f'dist analyzer is not aligned with the minimal release ABI: {required}'
assert "--require-from', str(repo / 'src' / 'runtime' / 'quickjs_runtime_scaffold.c')" not in analyzer
print('[venom] dist analyzer minimal release ABI policy: PASS')
