#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve()
poly = (root / 'src/vm/polymorph.cpp').read_text(encoding='utf-8')
poly_h = (root / 'src/vm/include/venom/vm/polymorph.hpp').read_text(encoding='utf-8')
encoder = (root / 'src/vm/encoder.cpp').read_text(encoding='utf-8')
package_runtime = (root / 'src/runtime/package_runtime.c').read_text(encoding='utf-8')
wasm_runtime = (root / 'src/runtime/wasm_runtime.c').read_text(encoding='utf-8')

checks = [
    ('polymorphic plan must remain version 2', 'push_u32(2u);' in poly),
    ('variable instruction-size field must be absent from the plan', 'instruction_size' not in poly_h),
    ('variable stride diversification must be absent', 'route-instruction-stride' not in poly),
    ('encoder must reserve fixed 16-byte records', 'program.size() * 16u' in encoder or 'program.size() * 16' in encoder),
    ('native package runtime must require 16-byte records', 'vm->instruction_size != 16u' in package_runtime),
    ('WASM runtime must require 16-byte records', 'instr_size != 16u' in wasm_runtime),
    ('WASM polymorphic parser must require version 2', 'read_u32(data + 8u) != 2u' in wasm_runtime),
]
failed = [name for name, ok in checks if not ok]
if failed:
    for name in failed:
        print(f'[FAIL] {name}')
    raise SystemExit(1)
print('[PASS] compiler and embedded Route VM runtime share the fixed v2/16-byte contract')
