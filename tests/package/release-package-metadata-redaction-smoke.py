#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from _turbojs_artifact import require_current_artifact

source_root = Path(__file__).resolve().parents[2]
require_current_artifact(source_root)

venom = Path(sys.argv[1]).resolve()
site = Path(sys.argv[2]).resolve()
out = Path(sys.argv[3]).resolve()
checker = Path(sys.argv[4]).resolve()
if out.exists():
    shutil.rmtree(out)
subprocess.run([str(venom), 'build', str(site), '--out', str(out)], check=True)
packages = list((out / 'assets' / 'app').glob('*.vbc'))
if len(packages) != 1:
    raise SystemExit(f'expected one package, found {len(packages)}')
forbidden = [
    'module-entry.js', 'module-util.js', 'module-extra.js',
    'module-branch.js', 'module-leaf.js', 'compat.js',
]
command = [sys.executable, str(checker), str(packages[0]), '--strict']
for item in forbidden:
    command.extend(['--forbid', item])
subprocess.run(command, check=True)
print('release package metadata redaction smoke: PASS')
