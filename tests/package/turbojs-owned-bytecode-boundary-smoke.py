#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: turbojs-owned-bytecode-boundary-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'verify', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'turbojs_bytecode_records: 1',
    'turbojs_wasm_execution: yes',
    'turbojs_execution_backend: turbojs-wasm-real',
    'turbojs_host_js_fallback_allowed: no',
    'turbojs_bytecode_boundary: wasm-owned',
    'turbojs_source_transfer: opaque-vtjsbc03-native-object',
    'turbojs_wasm_runtime_mode: turbojs-wasm-abi12-upstream-global-host-api-shims',
    'turbojs_execute_bytecode_export: yes',
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
        raise SystemExit(f'missing owned-bytecode boundary marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'console.log',
    b'basic site script loaded',
    b'VTJSBC01',
    b'source-preserving-byte-buffer-record',
    b'turbojs-source-transfer.vqst',
    b'turbojs-wasm-runtime.vqwr',
    b'VENOM_TJS_WASM_BACKEND_CONTRACT_V2',
    b'turbojs-wasm-abi12-runtime',
    b'venom_tjs_status_code',
    b'venom_tjs_bytecode_validate',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing turbojs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [b'venom_tjs_execute_bytecode', b'venom_tjs_status_code', b'venom_tjs_bytecode_validate', b'venom_tjs_context_report_ptr', b'venom_tjs_runtime_limits_ptr', b'venom_tjs_backend_contract_ptr', b'venom_tjs_interpreter_ready', b'venom_tjs_interpreter_dispatch_count']:
    if export not in wasm:
        raise SystemExit(f'turbojs-runtime wasm does not export {export!r}')

engine_files = list((out / 'assets' / 'runtime').glob('engine*.js'))
if not engine_files:
    raise SystemExit('missing turbojs-engine module')
engine = engine_files[0].read_text(encoding='utf-8')
if 'turbojs-wasm-abi12-upstream-global-host-api-shims' not in engine:
    raise SystemExit('turbojs engine module did not advertise owned-bytecode adapter')
if 'native TurboJS object bytecode cannot be converted back to host source' not in engine:
    raise SystemExit('turbojs engine is missing fail-closed native-bytecode boundary')

print('TurboJS owned bytecode boundary smoke passed')
