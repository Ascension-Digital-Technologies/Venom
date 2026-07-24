#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
header = root / 'src/generated/runtime/turbojs_runtime_wasm_blob.hpp'
text = header.read_text(encoding='utf-8')

sha_match = re.search(r'kTurboJsRuntimeWasmBlobSha256\s*=\s*"([0-9a-f]{64})"', text)
prov_match = re.search(r'kTurboJsRuntimeWasmBlobProvenance\s*=\s*R"[^\(]*\((.*?)\)[A-Za-z0-9_]*"', text, re.S)
assert sha_match and prov_match, 'embedded TurboJS/WASM metadata is missing'

provenance: dict[str, str] = {}
for line in prov_match.group(1).splitlines():
    if '=' in line:
        key, value = line.split('=', 1)
        provenance[key.strip()] = value.strip()

required_bridge = {
    'venom_tjs_bridge_abi',
    'venom_tjs_bridge_input_alloc',
    'venom_tjs_bridge_input_capacity',
    'venom_tjs_bridge_invoke',
    'venom_tjs_bridge_result_ptr',
    'venom_tjs_bridge_result_size',
    'venom_tjs_bridge_release',
}
exports = set(filter(None, provenance.get('exports', '').split(',')))
missing = sorted(required_bridge - exports)
assert not missing, f'embedded TurboJS/WASM is missing protected bridge exports: {missing}'
assert provenance.get('required_export_count') == '23'
assert provenance.get('release_export_count') == '23'
assert provenance.get('required_exports_satisfied') == 'true'
assert provenance.get('runtime_abi') == '12'
assert provenance.get('bytecode_format') == 'VTJSBC03'
assert provenance.get('module_bundle_contract') == 'VTJSMB04'
assert provenance.get('wasm_sha256') == sha_match.group(1)

# Reconstruct the constexpr byte array and verify that the declared digest is real.
array_match = re.search(
    r'kTurboJsRuntimeWasmBlob\[\]\s*=\s*\{(.*?)\};', text, re.S
)
assert array_match, 'embedded TurboJS/WASM byte array is missing'
blob = bytes(int(value, 16) for value in re.findall(r'0x([0-9a-fA-F]{2})', array_match.group(1)))
assert blob.startswith(b'\0asm'), 'embedded artifact is not WebAssembly'
assert hashlib.sha256(blob).hexdigest() == sha_match.group(1), 'embedded artifact digest mismatch'
assert str(len(blob)) == provenance.get('wasm_bytes'), 'embedded artifact size mismatch'

print('embedded TurboJS protected bridge artifact coherence: PASS')
