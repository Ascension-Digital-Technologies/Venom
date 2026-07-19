#!/usr/bin/env python3
from pathlib import Path
import sys
root = Path(sys.argv[1])
for rel in ('src/vm/polymorph.cpp', 'src/pipeline/build.cpp', 'src/pipeline/html.cpp'):
    text = (root / rel).read_text(encoding='utf-8')
    if 'std::mt19937' in text:
        raise SystemExit(f'legacy mt19937 remains in protection path: {rel}')
poly = (root / 'src/vm/polymorph.cpp').read_text(encoding='utf-8')
for needle in ('venom-diversification-domain-v1', 'route-opcode-map', 'route-masks'):
    if needle not in poly:
        raise SystemExit(f'missing diversification marker: {needle}')
print('cryptographic diversification source checks passed')
