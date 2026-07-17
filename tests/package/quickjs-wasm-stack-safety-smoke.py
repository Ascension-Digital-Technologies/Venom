#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(sys.argv[1]).resolve()
ps = (root / 'tools' / 'windows-scripts' / 'build-quickjs-wasm.ps1').read_text(encoding='utf-8')
sh = (root / 'tools' / 'linux-scripts' / 'build-quickjs-wasm.sh').read_text(encoding='utf-8')
abi = (root / 'src' / 'quickjs' / 'abi.hpp').read_text(encoding='utf-8')
scaffold = (root / 'src' / 'runtime' / 'quickjs_runtime_scaffold.c').read_text(encoding='utf-8')
engine = (root / 'src' / 'generated' / 'runtime' / 'quickjs_engine_module.cpp').read_text(encoding='utf-8')
windows_entry = (root / 'scripts' / 'windows' / 'build-emsdk.bat').read_text(encoding='utf-8')
linux_entry = (root / 'scripts' / 'linux' / 'build-emsdk.sh').read_text(encoding='utf-8')

for name, text in [('PowerShell implementation', ps), ('shell implementation', sh)]:
    m = re.search(r'STACK_SIZE=([0-9]+)', text)
    if not m or int(m.group(1)) < 2 * 1024 * 1024:
        raise SystemExit(f'{name} QuickJS WASM build must reserve at least 2 MiB of native stack')

if 'kDefaultStackLimitBytes = 256u * 1024u' not in abi:
    raise SystemExit('QuickJS logical stack limit must be 256 KiB')
if 'venom_qjs_wasm_native_stack_capacity' not in scaffold or 'VENOM_WASM_NATIVE_STACK_CAPACITY' not in scaffold:
    raise SystemExit('WASM runtime must expose its compiled native stack capacity')
if '#define DEFAULT_STACK_LIMIT (256u * 1024u)' not in scaffold:
    raise SystemExit('WASM runtime QuickJS stack limit must be 256 KiB')
if 'default_stack_limit=262144' not in scaffold:
    raise SystemExit('WASM runtime metadata must advertise the corrected stack limit')
if 'QuickJS WASM native stack is too small' not in engine:
    raise SystemExit('browser runtime must reject stale small-stack WASM artifacts')
if ': 262144;' not in engine:
    raise SystemExit('browser engine module must default to the corrected stack limit')
if 'build-emsdk.ps1' not in windows_entry:
    raise SystemExit('Windows build-emsdk launcher must delegate to the canonical PowerShell implementation')
if 'build_emscripten.py' not in linux_entry:
    raise SystemExit('Linux build-emsdk launcher must delegate to the canonical Emscripten build tool')
print('quickjs wasm stack safety smoke: PASS')
