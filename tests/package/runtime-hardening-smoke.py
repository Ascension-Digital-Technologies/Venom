#!/usr/bin/env python3
import re
import shutil
import subprocess
import sys
from pathlib import Path

venom = Path(sys.argv[1])
site = Path(sys.argv[2])
out = Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod', '--hashed'], check=True)
package_files = sorted((out / 'assets').glob('app*.vbc'))
loader_files = sorted((out / 'assets' / 'loader').glob('loader*.js'))
if len(package_files) != 1 or len(loader_files) != 1:
    raise SystemExit('expected exactly one hashed app package and loader')

inspect = subprocess.run([str(venom), 'inspect', str(package_files[0])], check=True, text=True, stdout=subprocess.PIPE)
text = inspect.stdout
required = [
    'version: 40',
    'flags: release|polymorphic|compressed|crypto-ready|integrity-metadata|aead-ready|runtime-hardened|host-bridge|fetch-bridge|async-host-queue',
    'host_bridge name="s.',
    'fetch_bridge name="s.',
    'async_host_queue name="s.',
    'turbojs_engine_module name="s.',
    'integrity name="s.',
]
missing = [item for item in required if item not in text]
if missing:
    raise SystemExit('missing expected runtime hardening inspect output: ' + ', '.join(missing))

for forbidden in ('script-diagnostics.txt', 'bundle-preview.js', 'dom-templates.txt', 'turbojs-probe.txt', 'turbojs-bridge-plan.txt', 'profile-diagnostics.txt', 'runtime-policy.vhrd', 'host-calls.vhcb', 'integrity-auth.vsig'):
    if forbidden in text:
        raise SystemExit('release package leaked internal/debug section name: ' + forbidden)

loader = loader_files[0].read_text(encoding='utf-8')
if "hardened: true" not in loader:
    raise SystemExit('loader did not request hardened runtime boot')
match = re.search(r"expectedPackageHash: '0x[0-9a-f]{16}'", loader)
if not match:
    raise SystemExit('loader is missing package hash allowlist metadata')

print('runtime hardening smoke passed')
