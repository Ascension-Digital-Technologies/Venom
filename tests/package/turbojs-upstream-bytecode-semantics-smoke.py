#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: turbojs-upstream-runtime-bridge-smoke.py <venom> <site> <out>')
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
    'turbojs_upstream_parity: yes',
    'turbojs_upstream_core: turbojs-upstream-global-host-api-shims-v7',
    'turbojs_upstream_exports: yes',
    'turbojs_upstream_runtime_exports: yes',
    'turbojs_upstream_bytecode_semantics: yes',
    'turbojs_upstream_bytecode_semantics_exports: yes',
    'turbojs_upstream_intrinsic_semantics: yes',
    'turbojs_upstream_intrinsic_semantics_exports: yes',
    'turbojs_semantic_runtime: yes',
    'turbojs_bytecode_boundary: wasm-owned',
    'release_status: PASS',
]
for marker in required:
    if marker not in check.stdout:
        raise SystemExit(f'missing upstream runtime bridge marker: {marker}\n{check.stdout}')

pkg = next((out / 'assets').glob('app*.vbc'))
raw = pkg.read_bytes()
for marker in [
    b'turbojs-upstream-global-host-api-shims-v7',
    b'turbojs-wasm-abi12-upstream-global-host-api-shims',
    b'VENOM_TURBOJS_NATIVE_PARITY_V3',
    b'upstream_turbojs_bridge=enabled',
    b'console.log',
    b'basic site script loaded',
    b'VTJSBC01',
    b'route-bytecode.vmbc',
]:
    if marker in raw:
        raise SystemExit(f'raw protected package leaked upstream runtime marker: {marker!r}')

wasm_files = list((out / 'assets' / 'runtime').glob('runtime*.wasm'))
if not wasm_files:
    raise SystemExit('missing turbojs-runtime wasm asset')
wasm = wasm_files[0].read_bytes()
for export in [
    b'venom_tjs_upstream_turbojs_ready',
    b'venom_tjs_upstream_parity_record_ptr',
    b'venom_tjs_upstream_parity_record_size',
    b'venom_tjs_upstream_feature_count',
    b'venom_tjs_upstream_builtin_count',
    b'venom_tjs_upstream_object_model_score',
    b'venom_tjs_upstream_exception_model_score',
    b'venom_tjs_upstream_module_model_score',
    b'venom_tjs_upstream_object_record_ptr',
    b'venom_tjs_upstream_exception_record_ptr',
    b'venom_tjs_upstream_module_record_ptr',
    b'venom_tjs_upstream_runtime_bridge_score',
    b'venom_tjs_upstream_bytecode_semantics_record_ptr',
    b'venom_tjs_upstream_bytecode_semantics_score',
    b'venom_tjs_upstream_lexical_scope_record_ptr',
    b'venom_tjs_upstream_closure_record_ptr',
    b'venom_tjs_upstream_iterator_record_ptr',
]:
    if export not in wasm:
        raise SystemExit(f'turbojs-runtime wasm missing upstream export: {export!r}')

engine_files = list((out / 'assets' / 'runtime').glob('engine*.js'))
if not engine_files:
    raise SystemExit('missing turbojs-engine module')
engine = engine_files[0].read_text(encoding='utf-8')
for marker in ['upstreamTurboJsReady', 'upstreamParityRecord', 'upstreamObjectModelScore', 'upstreamBytecodeSemanticsScore', 'upstreamBytecodeSemanticsRecord', 'turbojs-wasm-abi12-upstream-global-host-api-shims']:
    if marker not in engine:
        raise SystemExit(f'turbojs engine module missing upstream marker: {marker}')

print('TurboJS upstream bytecode semantics smoke passed')
