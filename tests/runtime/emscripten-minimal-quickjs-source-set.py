#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
files=[root/'tools/linux-scripts/build-quickjs-wasm.sh',root/'tools/windows-scripts/build-quickjs-wasm.ps1']
for p in files:
    text=p.read_text(encoding='utf-8')
    command='\n'.join(line for line in text.splitlines() if not line.lstrip().startswith('#'))
    if 'quickjs-libc.c' in command:
        raise SystemExit(f'{p}: quickjs-libc.c is still passed to emcc')
for p in [root/'tools/build_emscripten.py',root/'tools/setup_emscripten.py']:
    text=p.read_text(encoding='utf-8')
    if "default='4.0.10'" not in text:
        raise SystemExit(f'{p}: verified Emscripten default is not pinned')
print('emscripten minimal QuickJS source-set smoke: PASS')
