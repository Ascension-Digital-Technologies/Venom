#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: turbojs-wasm-interpreter-dispatch-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'verify', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'turbojs_wasm_runtime_mode: turbojs-wasm-abi12-upstream-global-host-api-shims',
    'turbojs_runtime_abi: 12',
    'turbojs_runtime_package_version: 83',
    'turbojs_abi_contract: turbojs-wasm-abi12-runtime',
    'turbojs_abi12_runtime: yes',
    'turbojs_interpreter_dispatch: yes',
    'turbojs_interpreter_exports: yes',
    'turbojs_interpreter_contract_export: yes',
    'turbojs_host_js_fallback_allowed: no',
    'release_status: PASS',
]
for marker in required:
    if marker not in check.stdout:
        raise SystemExit(f'missing interpreter-dispatch marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'VENOM_TJS_INTERPRETER_CONTRACT_V1',
    b'turbojs-wasm-abi12-interpreter-v1',
    b'turbojs-wasm-abi12-upstream-global-host-api-shims',
    b'source_eval_in_runtime=false',
    b'console.log',
    b'basic site script loaded',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked interpreter marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing turbojs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [
    b'venom_tjs_interpreter_ready',
    b'venom_tjs_interpreter_contract_ptr',
    b'venom_tjs_interpreter_dispatch_count',
    b'venom_tjs_interpreter_opcode_count',
    b'venom_tjs_global_slot_count',
    b'venom_tjs_last_global_write_hash',
]:
    if export not in wasm:
        raise SystemExit(f'turbojs-runtime wasm missing interpreter export: {export!r}')

engine_files = list((out / 'assets' / 'runtime').glob('engine*.js'))
if not engine_files:
    raise SystemExit('missing turbojs-engine module')
engine = engine_files[0].read_text(encoding='utf-8')
for marker in ['turbojs-wasm-abi12-upstream-global-host-api-shims', 'TurboJS WASM interpreter unavailable; source-decode fallback denied']:
    if marker not in engine:
        raise SystemExit(f'turbojs engine module missing interpreter marker: {marker}')

print('TurboJS WASM interpreter dispatch smoke passed')
