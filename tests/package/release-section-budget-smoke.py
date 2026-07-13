#!/usr/bin/env python3
import pathlib, struct, sys
root = pathlib.Path(sys.argv[1])
packages = list(root.rglob('*.vbc'))
if len(packages) != 1:
    raise SystemExit(f'expected one VBC package, found {len(packages)}')
data = packages[0].read_bytes()
if len(data) < 80 or data[:8] != b'VENOMVBC':
    raise SystemExit('invalid VBC package')
count = struct.unpack_from('<I', data, 20)[0]
if count > 70:
    raise SystemExit(f'release package metadata budget exceeded: {count} sections (max 70)')
print(f'[venom] release section budget: PASS ({count} sections)')
