#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
windows = (root/'tools/windows-scripts/build-quickjs-wasm.ps1').read_text(encoding='utf-8')
linux = (root/'tools/linux-scripts/build-quickjs-wasm.sh').read_text(encoding='utf-8')
for name, text in [('windows', windows), ('linux', linux)]:
    if '--strip-all' in text:
        raise SystemExit(f'{name}: unsupported Binaryen --strip-all is still hard-coded')
    for token in ('--strip-debug', '--strip-dwarf', '--strip-producers', '--help'):
        if token not in text:
            raise SystemExit(f'{name}: missing adaptive wasm-opt token {token}')
if 'emcc.py' not in windows or 'EMSDK_PYTHON' not in windows:
    raise SystemExit('windows: emcc batch-wrapper bypass is missing')
print('wasm-opt compatibility smoke: PASS')
