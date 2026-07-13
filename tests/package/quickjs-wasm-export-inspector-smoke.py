import json
import re
import subprocess
import sys
from pathlib import Path

root = Path(sys.argv[1])
out = Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
header = (root / 'src/compiler/quickjs_runtime_wasm_blob.hpp').read_text(encoding='utf-8')
body = header.split('inline constexpr std::uint8_t kQuickJsRuntimeWasmBlob[] = {', 1)[1].split('};', 1)[0]
wasm = bytes(int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]{2})', body))
wasm_path = out / 'quickjs-runtime-from-header.wasm'
wasm_path.write_bytes(wasm)
manifest = out / 'quickjs-runtime.manifest.txt'
exports_json = out / 'quickjs-runtime.exports.json'
subprocess.run([
    sys.executable,
    str(root / 'tools/wasm_exports.py'),
    str(wasm_path),
    '--require-from', str(root / 'src/runtime/quickjs_runtime_scaffold.c'),
    '--manifest', str(manifest),
    '--exports-json', str(exports_json),
    '--runtime-abi', '12',
    '--package-version', '83',
    '--fail-missing',
], check=True)
text = manifest.read_text(encoding='utf-8')
for marker in (
    'VENOM_QJS_WASM_ARTIFACT_V3',
    'runtime_abi=12',
    'package_version=83',
    'required_exports_satisfied=true',
    'missing_export_count=0',
    'runtime_implementation=quickjs-wasm-upstream-quickjs',
):
    if marker not in text:
        raise SystemExit(f'missing manifest marker {marker!r}')
info = json.loads(exports_json.read_text(encoding='utf-8'))
for name in ('venom_qjs_engine_abi', 'venom_qjs_execute_bytecode', 'venom_qjs_real_engine_candidate'):
    if name not in info['exports']:
        raise SystemExit(f'missing exported symbol {name}')
if not info['required_exports_satisfied']:
    raise SystemExit('expected generated export set to satisfy ABI requirements')
