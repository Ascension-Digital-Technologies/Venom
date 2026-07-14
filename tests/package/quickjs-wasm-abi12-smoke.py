#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: quickjs-wasm-abi12-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'release-check', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'quickjs_wasm_execution: yes',
    'quickjs_execution_backend: quickjs-wasm-real',
    'quickjs_bytecode_boundary: wasm-owned',
    'quickjs_source_transfer: opaque-vqjsbc03-native-object',
    'quickjs_wasm_runtime_mode: quickjs-wasm-abi12-upstream-global-host-api-shims',
    'quickjs_runtime_abi: 12',
    'quickjs_runtime_package_version: 83',
    'quickjs_abi_contract: quickjs-wasm-abi12-runtime',
    'quickjs_abi12_runtime: yes',
    'quickjs_status_exports: yes',
    'quickjs_limit_exports: yes',
    'quickjs_bytecode_validate_export: yes',
    'quickjs_backend_contract_export: yes',
    'quickjs_interpreter_dispatch: yes',
    'quickjs_interpreter_exports: yes',
    'quickjs_interpreter_contract_export: yes',
    'release_status: PASS',
]
for marker in required:
    if marker not in check.stdout:
        raise SystemExit(f'missing ABI12 marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'VENOM_QJS_WASM_BACKEND_CONTRACT_V2',
    b'quickjs-wasm-abi12-runtime',
    b'quickjs-wasm-abi12-interpreter-v1',
    b'venom_qjs_status_code',
    b'venom_qjs_bytecode_validate',
    b'VENOM_QJS_RUNTIME_ABI_V12',
    b'VENOM_QJS_HOST_IMPORT_TABLE_V10',
    b'VENOM_QJS_PARITY_PROBE_V7',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked ABI12 marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing quickjs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [
    b'venom_qjs_execute_bytecode',
    b'venom_qjs_bytecode_validate',
    b'venom_qjs_status_code',
    b'venom_qjs_status_record_ptr',
    b'venom_qjs_runtime_limits_ptr',
    b'venom_qjs_context_report_ptr',
    b'venom_qjs_backend_contract_ptr',
    b'venom_qjs_interpreter_ready',
    b'venom_qjs_interpreter_contract_ptr',
    b'venom_qjs_interpreter_dispatch_count',
    b'venom_qjs_release_status',
]:
    if export not in wasm:
        raise SystemExit(f'quickjs-runtime wasm missing ABI12 export: {export!r}')

print('QuickJS WASM ABI12 smoke passed')
