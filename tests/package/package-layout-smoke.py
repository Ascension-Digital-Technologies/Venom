#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

venom = pathlib.Path(sys.argv[1]).resolve()
site = pathlib.Path(sys.argv[2]).resolve()
out_a = pathlib.Path(sys.argv[3]).resolve()
out_b = pathlib.Path(sys.argv[4]).resolve()

for out in (out_a, out_b):
    if out.exists():
        shutil.rmtree(out)
    subprocess.check_call([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod', '--hashed'])
    check = subprocess.check_output([str(venom), 'release-check', str(out), '--target', 'browser'], text=True)
    for needle in ('layout_polymorphic: yes', 'payload_padding_bytes:', 'release_status: PASS'):
        if needle not in check:
            raise SystemExit(f'missing release-check layout marker {needle!r}\n{check}')
    padding_line = next((line for line in check.splitlines() if 'payload_padding_bytes:' in line), '')
    padding = int(padding_line.rsplit(':', 1)[1].strip())
    if padding <= 0:
        raise SystemExit('protected package did not contain payload layout jitter/padding')

pkg_a = next((out_a / 'assets').glob('app*.vbc'))
pkg_b = next((out_b / 'assets').glob('app*.vbc'))
bytes_a = pkg_a.read_bytes()
bytes_b = pkg_b.read_bytes()
if bytes_a == bytes_b:
    raise SystemExit('browser-protect profile produced identical package bytes across two layout-polymorphic builds')

for forbidden in (b'package-layout.vlay', b'VENOM_PACKAGE_LAYOUT_V1', b'layout_seed=', b'payload_jitter_max='):
    if forbidden in bytes_a or forbidden in bytes_b:
        raise SystemExit(f'protected package leaked plaintext layout metadata: {forbidden!r}')

inspect_a = subprocess.check_output([str(venom), 'inspect', str(pkg_a)], text=True)
if 'flags: protect|polymorphic|compressed|crypto-ready' not in inspect_a:
    raise SystemExit('inspect output is missing protect/polymorphic flags')
if 'package-layout.vlay' in inspect_a:
    raise SystemExit('inspect output leaked canonical package layout section name')

print('package layout polymorphism smoke passed')
