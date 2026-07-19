#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
source=(root/'src/pipeline/build_support.cpp').read_text(encoding='utf-8')
required=[
    'if (!hashed && !stem.empty())',
    'return stem.empty() ? hash + normalized_ext',
]
missing=[x for x in required if x not in source]
if missing:
    raise SystemExit('empty redacted asset naming guard missing: '+', '.join(missing))
print('empty redacted asset naming: PASS')
