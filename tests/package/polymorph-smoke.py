#!/usr/bin/env python3
import pathlib
from pathlib import Path


def find_package(dist: Path) -> Path:
    candidates = sorted((dist / 'assets' / 'app').glob('app.*.vbc'))
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit('missing app package under assets/app/app.<hash>.vbc')
import shutil
import subprocess
import sys

venom = pathlib.Path(sys.argv[1]).resolve()
site = pathlib.Path(sys.argv[2]).resolve()
out_a = pathlib.Path(sys.argv[3]).resolve()
out_b = pathlib.Path(sys.argv[4]).resolve()

vendor_cache = pathlib.Path(__file__).resolve().parents[1] / 'fixtures' / 'remote-cache'
for out in (out_a, out_b):
    if out.exists():
        shutil.rmtree(out)
    subprocess.check_call([
        str(venom), 'build', str(site), '--out', str(out), '--profile', 'protect',
        '--vendor-cache', str(vendor_cache), '--offline'
    ])

pkg_a = find_package(out_a)
pkg_b = find_package(out_b)
bytes_a = pkg_a.read_bytes()
bytes_b = pkg_b.read_bytes()
if bytes_a == bytes_b:
    raise SystemExit('protect profile produced identical package bytes across two builds')

inspect_a = subprocess.check_output([str(venom), 'inspect', str(pkg_a)], text=True)
for needle in ('flags: protect|release|polymorphic|compressed|crypto-ready', 'vm_bytecode name="s.', 'integrity name="s.', 'flags=encrypted'):
    if needle not in inspect_a:
        raise SystemExit(f'missing polymorphism inspect marker: {needle!r}')
for forbidden in ('vm-polymorph.vpol', 'route-bytecode.vmbc', 'routes.vbrt', 'strings.vstr'):
    if forbidden in inspect_a:
        raise SystemExit(f'protect inspect leaked canonical internal name: {forbidden!r}')
for needle in (b'VENOM_POLYMORPH 8', b'word_layout=', b'opcode_xor=', b'operand_xor=', b'shuffle_strings=1', b'shuffle_routes=1', b'vm-polymorph.vpol', b'route-bytecode.vmbc'):
    if needle in bytes_a + bytes_b:
        raise SystemExit(f'protect package leaked plaintext polymorphism marker: {needle!r}')

print('protect polymorphism smoke passed')
