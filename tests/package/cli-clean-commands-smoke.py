#!/usr/bin/env python3
from __future__ import annotations
import subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
help_text = subprocess.check_output([str(venom), '--help'], text=True)
for required in ('venom analyze <dist-dir>', 'venom verify <dist-or-package>'):
    if required not in help_text:
        raise SystemExit(f'missing clean command in help: {required}')
for removed in ('analyze-dist', 'release-check'):
    if removed in help_text:
        raise SystemExit(f'removed command still present in help: {removed}')
    result = subprocess.run([str(venom), removed, 'dist'], text=True, capture_output=True)
    if result.returncode == 0 or f'unknown command: {removed}' not in (result.stdout + result.stderr):
        raise SystemExit(f'removed command did not fail as unknown: {removed}')
with tempfile.TemporaryDirectory() as tmp:
    missing = Path(tmp) / 'missing'
    analyze = subprocess.run([str(venom), 'analyze', str(missing)], text=True, capture_output=True)
    verify = subprocess.run([str(venom), 'verify', str(missing)], text=True, capture_output=True)
    if analyze.returncode == 0 or 'analyze requires a distribution directory' not in (analyze.stdout + analyze.stderr):
        raise SystemExit('analyze did not route to the distribution analyzer')
    if verify.returncode == 0 or 'verify target does not exist' not in (verify.stdout + verify.stderr):
        raise SystemExit('verify did not route to the production verification gate')
print('clean CLI commands smoke: PASS')
