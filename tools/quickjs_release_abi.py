#!/usr/bin/env python3
"""Emit the release ABI from the authoritative QuickJS/WASM contract."""
import json, sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
contract=json.loads((root/'contracts/quickjs-wasm-abi.json').read_text())
RELEASE_ABI=[n for n in contract['requiredExports'] if n != 'memory']
if len(sys.argv) != 2:
    raise SystemExit('usage: quickjs_release_abi.py OUT.json')
with open(sys.argv[1], 'w', encoding='utf-8') as f:
    json.dump(['_' + n for n in RELEASE_ABI], f, indent=2)
print(f'[venom] wrote {len(RELEASE_ABI)} release ABI exports to {sys.argv[1]}')
