from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'src/runtime/wasm_runtime.c').read_text(encoding='utf-8')
bridge = (root / 'src/generated/runtime/wasm_runtime_js.cpp').read_text(encoding='utf-8')
header = (root / 'src/generated/runtime/wasm_runtime_blob.hpp').read_text(encoding='utf-8')

assert 'VENOM_RUNTIME_FEATURE_STREAMED_PACKAGE_UPLOAD 0x00000001u' in source
assert 'case 12u: return VENOM_RUNTIME_FEATURE_STREAMED_PACKAGE_UPLOAD;' in source
assert 'const runtimeFeatures = e.v12_n(12) >>> 0;' in bridge
assert 'streamed package upload feature is unavailable' in bridge

blob = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', header))
assert blob.startswith(b'\x00asm\x01\x00\x00\x00')

with tempfile.TemporaryDirectory() as td:
    wasm_path = Path(td) / 'runtime.wasm'
    js_path = Path(td) / 'probe.mjs'
    wasm_path.write_bytes(blob)
    js_path.write_text(
        """
import fs from 'node:fs';
const bytes = fs.readFileSync(process.argv[2]);
const { instance } = await WebAssembly.instantiate(bytes, {});
const e = instance.exports;
if (e.venom_runtime_abi() !== 1) throw new Error('ABI mismatch');
if (e.venom_package_version() !== 40) throw new Error('package version mismatch');
if ((e.v12_n(12) >>> 0) !== 1) throw new Error('streamed upload feature missing');
if (e.v12_x(7, 0, 0, 0, 0) !== 14) throw new Error('upload opcode mismatch');
console.log('embedded package runtime feature probe: PASS');
""".strip() + "\n",
        encoding='utf-8',
    )
    subprocess.run(['node', str(js_path), str(wasm_path)], check=True)

print('package runtime source/blob synchronization smoke: PASS')
