#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
dist = pathlib.Path(sys.argv[3])

if dist.exists():
    shutil.rmtree(dist)

subprocess.run([str(venom), 'build', str(site), '--out', str(dist), '--profile', 'prod', '--hashed'], check=True)
passed = subprocess.run([str(venom), 'release-check', str(dist), '--target', 'browser'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
if 'package_binding: yes' not in passed.stdout or 'loader_binding: yes' not in passed.stdout or 'bound_assets: 6' not in passed.stdout:
    print(passed.stdout)
    raise SystemExit('release-check did not report package/loader binding')

runtime_files = sorted(p for p in (dist / 'assets' / 'runtime').glob('*.js') if not p.name.startswith('engine.'))
if not runtime_files:
    raise SystemExit('missing runtime asset')
with runtime_files[0].open('ab') as f:
    f.write(b'\n// tamper package binding smoke\n')

failed = subprocess.run([str(venom), 'release-check', str(dist), '--target', 'browser'], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
if failed.returncode == 0:
    print(failed.stdout)
    raise SystemExit('release-check accepted tampered bound runtime asset')
if 'bound asset hash mismatch: runtime' not in failed.stdout:
    print(failed.stdout)
    raise SystemExit('release-check failed for the wrong reason')
