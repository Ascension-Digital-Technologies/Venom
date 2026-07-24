#!/usr/bin/env python3
import shutil
import subprocess
import sys
from pathlib import Path

venom = Path(sys.argv[1])
script_site = Path(sys.argv[2])
no_script_site = Path(sys.argv[3])
out_root = Path(sys.argv[4])

if out_root.exists():
    shutil.rmtree(out_root)
out_root.mkdir(parents=True)

blocked = out_root / 'blocked-release'
failed = subprocess.run(
    [str(venom), 'build', str(script_site), '--out', str(blocked), '--profile', 'prod'],
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)
if failed.returncode == 0:
    raise SystemExit('release build with script chunks and scaffold backend unexpectedly succeeded')
combined = failed.stdout + failed.stderr
if 'release build denied' not in combined or 'turbojs backend' not in combined or 'protected and release profiles cannot enable host fallback' not in combined.lower():
    raise SystemExit('release failure did not explain the strict TurboJS fallback policy: ' + combined)

rejected = out_root / 'rejected-release-fallback'
rejected_run = subprocess.run(
    [str(venom), 'build', str(script_site), '--out', str(rejected), '--profile', 'prod', '--allow-host-fallback'],
    text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
)
if rejected_run.returncode == 0 or 'structurally fail-closed' not in (rejected_run.stdout + rejected_run.stderr):
    raise SystemExit('release host fallback override was not rejected structurally')

strict_ok = out_root / 'noscript-release'
strict_run = subprocess.run(
    [str(venom), 'build', str(no_script_site), '--out', str(strict_ok), '--profile', 'prod'],
    check=True,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
)
if 'release_policy=allow-build' not in (strict_run.stdout + strict_run.stderr):
    raise SystemExit('no-script release build did not report allow-build policy')
packages = sorted((strict_ok / 'assets').glob('app*.vbc'))
if len(packages) != 1:
    raise SystemExit('no-script strict release did not produce exactly one package')

print('release policy smoke passed')
