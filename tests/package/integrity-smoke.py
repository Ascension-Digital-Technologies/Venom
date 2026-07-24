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
inspect = subprocess.run([str(venom), 'inspect', str(find_package(out))], check=True, text=True, stdout=subprocess.PIPE)
text = inspect.stdout
required = [
    'version: 40',
    'integrity-metadata',
    'integrity name="s.',
    'turbojs_engine_module name="s.',
    'host_bridge name="s.',
    'fetch_bridge name="s.',
    'async_host_queue name="s.',
    'flags: protect|polymorphic|compressed|crypto-ready|integrity-metadata|aead-ready|runtime-hardened|host-bridge|fetch-bridge|async-host-queue',
]
missing = [item for item in required if item not in text]
if missing:
    raise SystemExit('missing expected integrity inspect output: ' + ', '.join(missing))
for forbidden in ('integrity-auth.vsig', 'turbojs-engine-module.vqem', 'runtime-policy.vhrd', 'host-calls.vhcb', 'fetch-bridge.vfch', 'async-host-queue.vahq'):
    if forbidden in text:
        raise SystemExit('protect inspect leaked canonical internal name: ' + forbidden)
print('integrity metadata smoke passed')
