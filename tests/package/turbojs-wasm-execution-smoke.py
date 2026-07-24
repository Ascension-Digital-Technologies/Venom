#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

if len(sys.argv) != 5:
    print('usage: turbojs-wasm-execution-smoke.py <venom> <site> <browser-out> <native-out>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
browser_out = pathlib.Path(sys.argv[3])
native_out = pathlib.Path(sys.argv[4])

for path in (browser_out, native_out):
    if path.exists():
        shutil.rmtree(path)

browser_build = subprocess.run([str(venom), 'build', str(site), '--out', str(browser_out), '--profile', 'prod'], check=True, text=True, capture_output=True)
if 'turbojs_backend=wasm-real' not in browser_build.stdout + browser_build.stderr:
    raise SystemExit('browser-protect did not default to wasm-real backend')

browser_check = subprocess.run([str(venom), 'verify', str(browser_out), '--target', 'browser'], check=True, text=True, capture_output=True)
required = [
    'turbojs_wasm_execution: yes',
    'turbojs_execution_backend: turbojs-wasm-real',
    'turbojs_host_js_fallback_allowed: no',
    'turbojs_release_fail_closed: yes',
    'turbojs_wasm_chunks: 1',
    'turbojs_runtime_abi: 12',
    'turbojs_abi12_runtime: yes',
    'turbojs_bytecode_validate_export: yes',
    'release_status: PASS',
]
for marker in required:
    if marker not in browser_check.stdout:
        raise SystemExit(f'missing browser TurboJS/WASM execution marker: {marker}\n{browser_check.stdout}')

browser_pkg = next((browser_out / 'assets').glob('app*.vbc'))
raw = browser_pkg.read_bytes()
for marker in [b'VENOM_TJS_WASM_EXECUTION_V1', b'VENOM_TJS_WASM_EXECUTION_V2', b'turbojs-wasm-execution.vqwe', b'turbojs-wasm-real', b'turbojs-wasm-abi12-interpreter-v1']:
    if marker in raw:
        raise SystemExit(f'raw browser package leaked TurboJS/WASM execution marker: {marker!r}')

# Native protect uses the same fail-closed script execution contract, then layers libsodium section crypto.
key_file = native_out.with_suffix('.key')
subprocess.run([str(venom), 'keygen', '--out', str(key_file), '--force'], check=True)
native_build = subprocess.run([str(venom), 'build', str(site), '--out', str(native_out), '--profile', 'prod', '--key-file', str(key_file), '--require-audited-crypto'], text=True, capture_output=True)
if native_build.returncode != 0:
    if 'libsodium provider is not available' in native_build.stdout + native_build.stderr:
        print('libsodium unavailable; native TurboJS/WASM execution path skipped')
        raise SystemExit(0)
    print(native_build.stdout)
    print(native_build.stderr)
    raise SystemExit(native_build.returncode)

native_check = subprocess.run([str(venom), 'verify', str(native_out), '--target', 'native', '--key-file', str(key_file), '--require-audited-crypto'], check=True, text=True, capture_output=True)
for marker in required:
    if marker not in native_check.stdout:
        raise SystemExit(f'missing native TurboJS/WASM execution marker: {marker}\n{native_check.stdout}')
if 'provider_libsodium_sections: 0' in native_check.stdout:
    raise SystemExit('native-protect TurboJS/WASM execution package did not use libsodium sections')

print('TurboJS/WASM execution smoke passed')
