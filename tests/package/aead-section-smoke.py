#!/usr/bin/env python3
import shutil
import subprocess
import sys
from pathlib import Path


def find_package(dist: Path) -> Path:
    candidates = sorted((dist / 'assets' / 'app').glob('app.*.vbc'))
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise SystemExit('missing app package under assets/app/app.<hash>.vbc')

venom = Path(sys.argv[1])
site = Path(sys.argv[2])
out = Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod'], check=True)
package = find_package(out)
raw = package.read_bytes()
if b'VAEAD001' not in raw:
    raise SystemExit('protect package did not contain AEAD section envelopes')
if b'VSEAL001' in raw:
    raise SystemExit('protect package still used legacy VSEAL envelopes')
for forbidden in (b'console.log', b'basic site script loaded', b'route-bytecode.vmbc', b'runtime-policy.vhrd', b'package-features.vfeat'):
    if forbidden in raw:
        raise SystemExit('AEAD package leaked forbidden raw text: ' + forbidden.decode('utf-8', 'replace'))
inspect = subprocess.run([str(venom), 'inspect', str(package)], check=True, text=True, stdout=subprocess.PIPE)
if 'aead-ready' not in inspect.stdout:
    raise SystemExit('inspect output did not advertise aead-ready flag')
print('AEAD section smoke passed')
