#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: turbojs-wasm-abi12-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'verify', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'turbojs_wasm_execution: yes',
    'turbojs_execution_backend: turbojs-wasm-real',
    'turbojs_bytecode_boundary: wasm-owned',
    'turbojs_source_transfer: opaque-vtjsbc03-native-object',
    'turbojs_wasm_runtime_mode: turbojs-wasm-abi12-upstream-global-host-api-shims',
    'turbojs_runtime_abi: 12',
    'turbojs_runtime_package_version: 83',
    'turbojs_abi_contract: turbojs-wasm-abi12-runtime',
    'turbojs_abi12_runtime: yes',
    'turbojs_status_exports: yes',
    'turbojs_limit_exports: yes',
    'turbojs_bytecode_validate_export: yes',
    'turbojs_backend_contract_export: yes',
    'turbojs_interpreter_dispatch: yes',
    'turbojs_interpreter_exports: yes',
    'turbojs_interpreter_contract_export: yes',
    'release_status: PASS',
]
for marker in required:
    if marker not in check.stdout:
        raise SystemExit(f'missing ABI12 marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'VENOM_TJS_WASM_BACKEND_CONTRACT_V2',
    b'turbojs-wasm-abi12-runtime',
    b'turbojs-wasm-abi12-interpreter-v1',
    b'venom_tjs_status_code',
    b'venom_tjs_bytecode_validate',
    b'VENOM_TJS_RUNTIME_ABI_V12',
    b'VENOM_TJS_HOST_IMPORT_TABLE_V10',
    b'VENOM_TJS_PARITY_PROBE_V7',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked ABI12 marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing turbojs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [
    b'venom_tjs_execute_bytecode',
    b'venom_tjs_bytecode_validate',
    b'venom_tjs_status_code',
    b'venom_tjs_status_record_ptr',
    b'venom_tjs_runtime_limits_ptr',
    b'venom_tjs_context_report_ptr',
    b'venom_tjs_backend_contract_ptr',
    b'venom_tjs_interpreter_ready',
    b'venom_tjs_interpreter_contract_ptr',
    b'venom_tjs_interpreter_dispatch_count',
    b'venom_tjs_release_status',
]:
    if export not in wasm:
        raise SystemExit(f'turbojs-runtime wasm missing ABI12 export: {export!r}')

print('TurboJS WASM ABI12 smoke passed')
