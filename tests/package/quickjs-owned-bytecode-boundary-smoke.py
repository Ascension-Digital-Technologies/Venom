#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: quickjs-owned-bytecode-boundary-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'browser-protect'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'release-check', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'quickjs_bytecode_records: 1',
    'quickjs_wasm_execution: yes',
    'quickjs_execution_backend: quickjs-wasm-real',
    'quickjs_host_js_fallback_allowed: no',
    'quickjs_bytecode_boundary: wasm-owned',
    'quickjs_source_transfer: opaque-vqjsbc03-native-object',
    'quickjs_wasm_runtime_mode: quickjs-wasm-abi12-upstream-global-host-api-shims',
    'quickjs_execute_bytecode_export: yes',
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
        raise SystemExit(f'missing owned-bytecode boundary marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'console.log',
    b'basic site script loaded',
    b'VQJSBC01',
    b'source-preserving-byte-buffer-record',
    b'quickjs-source-transfer.vqst',
    b'quickjs-wasm-runtime.vqwr',
    b'VENOM_QJS_WASM_BACKEND_CONTRACT_V2',
    b'quickjs-wasm-abi12-runtime',
    b'venom_qjs_status_code',
    b'venom_qjs_bytecode_validate',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing quickjs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [b'venom_qjs_execute_bytecode', b'venom_qjs_status_code', b'venom_qjs_bytecode_validate', b'venom_qjs_context_report_ptr', b'venom_qjs_runtime_limits_ptr', b'venom_qjs_backend_contract_ptr', b'venom_qjs_interpreter_ready', b'venom_qjs_interpreter_dispatch_count']:
    if export not in wasm:
        raise SystemExit(f'quickjs-runtime wasm does not export {export!r}')

engine_files = list((out / 'assets' / 'runtime').glob('engine*.js'))
if not engine_files:
    raise SystemExit('missing quickjs-engine module')
engine = engine_files[0].read_text(encoding='utf-8')
if 'quickjs-wasm-abi12-upstream-global-host-api-shims' not in engine:
    raise SystemExit('quickjs engine module did not advertise owned-bytecode adapter')
if 'native QuickJS object bytecode cannot be converted back to host source' not in engine:
    raise SystemExit('quickjs engine is missing fail-closed native-bytecode boundary')

print('QuickJS owned bytecode boundary smoke passed')
