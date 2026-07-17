from pathlib import Path
root = Path(__file__).resolve().parents[2]
c = (root / 'src/runtime/wasm_runtime.c').read_text(encoding='utf-8')
js = (root / 'src/generated/runtime/wasm_runtime_js.cpp').read_text(encoding='utf-8')
required_c = [
    'g_package_upload_expected', 'g_package_upload_received',
    'venom_wasm_begin_package_upload', 'venom_wasm_accept_package_chunk',
    'venom_wasm_finish_package_upload', 'venom_wasm_release_package',
    'case 7u:', 'case 8u:', 'case 9u:', 'case 10u:',
    'venom_wasm_secure_zero(g_package, g_package_loaded_size)'
]
required_js = [
    'const chunkSize = 64 * 1024', 'e.v12_x(7, packageSize',
    'e.v12_x(8, offset >>> 0', 'e.v12_x(9, 0, 0, 0, 0)',
    'bytes.fill(0)', 'residentWasm: true', 'bytes: null',
    "WASM runtime requires a resident package"
]
for token in required_c:
    assert token in c, token
for token in required_js:
    assert token in js, token
assert 'wasmMemoryView(instance).set(pkg.bytes' not in js
print('streamed WASM decoding smoke: PASS')
