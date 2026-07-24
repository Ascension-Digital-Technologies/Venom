#!/usr/bin/env python3
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
if len(package_files) != 1:
    raise SystemExit('expected exactly one package')
inspect = subprocess.run([str(venom), 'inspect', str(package_files[0])], check=True, text=True, stdout=subprocess.PIPE)
text = inspect.stdout
required = [
    'version: 40',
    'package_features name="s.',
    'turbojs_runtime_denylist name="s.',
    'integrity name="s.',
]
missing = [item for item in required if item not in text]
if missing:
    raise SystemExit('missing expected package-feature inspect output: ' + ', '.join(missing))
for forbidden in ('package-features.vfeat', 'turbojs-runtime-denylist.vqrd', 'integrity-auth.vsig'):
    if forbidden in text:
        raise SystemExit('release inspect leaked canonical package-feature name: ' + forbidden)
print('package features smoke passed')
