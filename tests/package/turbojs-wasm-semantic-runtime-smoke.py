#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: turbojs-wasm-semantic-runtime-smoke.py <venom> <site> <out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True, text=True, capture_output=True)
check = subprocess.run([str(venom), 'verify', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
for marker in [
    'turbojs_runtime_package_version: 83',
    'turbojs_interpreter_dispatch: yes',
    'turbojs_semantic_runtime: yes',
    'turbojs_semantic_runtime_exports: yes',
    'release_status: PASS',
]:
    if marker not in check.stdout:
        raise SystemExit(f'missing semantic runtime marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'venom_tjs_interpreter_semantic_record_ptr',
    b'venom_tjs_global_slot_record_ptr',
    b'turbojs-wasm-abi12-upstream-global-host-api-shims',
    b'console.log',
    b'basic site script loaded',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked semantic marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing turbojs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [
    b'venom_tjs_interpreter_semantic_record_ptr',
    b'venom_tjs_interpreter_semantic_record_size',
    b'venom_tjs_interpreter_property_write_count',
    b'venom_tjs_interpreter_builtin_probe_count',
    b'venom_tjs_interpreter_console_call_count',
    b'venom_tjs_global_slot_record_ptr',
    b'venom_tjs_global_slot_record_size',
]:
    if export not in wasm:
        raise SystemExit(f'turbojs-runtime wasm missing semantic export: {export!r}')

engine_files = list((out / 'assets' / 'runtime').glob('engine*.js'))
if not engine_files:
    raise SystemExit('missing turbojs-engine module')
engine = engine_files[0].read_text(encoding='utf-8')
for marker in ['interpreterSemanticRecord', 'interpreterPropertyWriteCount', 'venom_tjs_interpreter_semantic_record_ptr']:
    if marker not in engine:
        raise SystemExit(f'turbojs engine module missing semantic marker: {marker}')

print('TurboJS WASM semantic runtime smoke passed')
