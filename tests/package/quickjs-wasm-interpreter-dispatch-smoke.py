#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: quickjs-wasm-interpreter-dispatch-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'release-check', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'quickjs_wasm_runtime_mode: quickjs-wasm-abi12-upstream-global-host-api-shims',
    'quickjs_runtime_abi: 12',
    'quickjs_runtime_package_version: 83',
    'quickjs_abi_contract: quickjs-wasm-abi12-runtime',
    'quickjs_abi12_runtime: yes',
    'quickjs_interpreter_dispatch: yes',
    'quickjs_interpreter_exports: yes',
    'quickjs_interpreter_contract_export: yes',
    'quickjs_host_js_fallback_allowed: no',
    'release_status: PASS',
]
for marker in required:
    if marker not in check.stdout:
        raise SystemExit(f'missing interpreter-dispatch marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'VENOM_QJS_INTERPRETER_CONTRACT_V1',
    b'quickjs-wasm-abi12-interpreter-v1',
    b'quickjs-wasm-abi12-upstream-global-host-api-shims',
    b'source_eval_in_runtime=false',
    b'console.log',
    b'basic site script loaded',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked interpreter marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing quickjs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [
    b'venom_qjs_interpreter_ready',
    b'venom_qjs_interpreter_contract_ptr',
    b'venom_qjs_interpreter_dispatch_count',
    b'venom_qjs_interpreter_opcode_count',
    b'venom_qjs_global_slot_count',
    b'venom_qjs_last_global_write_hash',
]:
    if export not in wasm:
        raise SystemExit(f'quickjs-runtime wasm missing interpreter export: {export!r}')

engine_files = list((out / 'assets' / 'runtime').glob('engine*.js'))
if not engine_files:
    raise SystemExit('missing quickjs-engine module')
engine = engine_files[0].read_text(encoding='utf-8')
for marker in ['quickjs-wasm-abi12-upstream-global-host-api-shims', 'QuickJS WASM interpreter unavailable; source-decode fallback denied']:
    if marker not in engine:
        raise SystemExit(f'quickjs engine module missing interpreter marker: {marker}')

print('QuickJS WASM interpreter dispatch smoke passed')
