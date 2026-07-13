#!/usr/bin/env python3
import pathlib
import subprocess
import sys

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'browser-protect'], check=True)
check = subprocess.run([str(venom), 'release-check', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
stdout = check.stdout
required = [
    'lazy_sections: yes',
    'lazy_route_sections: 2',
    'lazy_script_sections: 1',
    'release_status: PASS',
]
for marker in required:
    if marker not in stdout:
        raise SystemExit(f'missing release-check marker: {marker}\n{stdout}')

packages = list((out / 'assets').glob('app*.vbc'))
if not packages:
    raise SystemExit('missing package artifact')
raw = packages[0].read_bytes()
for marker in [b'VENOM_LAZY_SECTIONS_V1', b'lazy-sections.vlazy', b'route-chunk.', b'script-chunk.']:
    if marker in raw:
        raise SystemExit(f'raw package leaked lazy marker: {marker!r}')
print('lazy sections smoke passed')
