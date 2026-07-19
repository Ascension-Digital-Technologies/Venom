#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, sys
root=Path(sys.argv[1]).resolve()
subprocess.run([sys.executable, str(root/'tools/route_wasm_provenance.py'), '--header', str(root/'include/venom/generated/runtime/wasm_runtime_blob.hpp'), '--out', str(root/'src/generated/runtime/metadata/wasm_runtime_provenance.json'), '--check'], check=True)
m=json.loads((root/'src/generated/runtime/metadata/wasm_runtime_provenance.json').read_text(encoding='utf-8'))
assert m['artifact_kind']=='venom-package-route-runtime-wasm'
assert m['scaffold_runtime'] is False
assert m['required_exports_satisfied'] is True
assert m['dom_command_contract']=='VDOP0020' and m['dom_command_contract_embedded'] is True
assert m['route_vm_contract']=='VPOL0010-v2-fixed16'
assert m['route_vm_contract_location']=='protected package plan section'
print('[PASS] authoritative package/route WASM provenance matches the embedded artifact')
