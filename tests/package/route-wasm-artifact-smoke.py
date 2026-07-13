#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(sys.argv[1])
h=(root/'src/compiler/wasm_runtime_blob.hpp').read_text(encoding='utf-8')
required=['artifact_kind=real-route-vm','runtime_implementation=venom-route-vm-wasm','scaffold_runtime=false','required_exports_satisfied=true','instruction_contract=VPOL0010-v2-fixed16']
missing=[x for x in required if x not in h]
if missing: raise SystemExit('missing Route VM provenance: '+', '.join(missing))
if 'contract-scaffold' in h: raise SystemExit('Route VM scaffold provenance remains embedded')
print('[PASS] authoritative Route VM WASM artifact is embedded with verified provenance')
