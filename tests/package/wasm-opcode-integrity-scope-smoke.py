#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]).resolve()
source = (root / 'src/generated/runtime/wasm_runtime_js.cpp').read_text(encoding='utf-8')
required = [
    'const std::string opcode_integrity_block',
    'activeRuntimeOpcodeMap = null;',
    'activeRuntimeIntegritySeal = computeRuntimeIntegritySeal(null);',
    'assertRuntimeIntegrity(null);',
    'source.replace(opcode_pos, opcode_integrity_block.size(), wasm_integrity_block);',
]
for token in required:
    if token not in source:
        raise SystemExit(f'missing WASM opcode integrity scope contract: {token}')
if 'source.erase(opcode_pos, opcode_line.size())' in source:
    raise SystemExit('stale opcodeMap declaration-only removal remains')
print('WASM opcode integrity scope smoke: PASS')
