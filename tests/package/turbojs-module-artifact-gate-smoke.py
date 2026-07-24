#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

from _turbojs_artifact import artifact_is_current

if len(sys.argv) != 4:
    raise SystemExit('usage: turbojs-module-artifact-gate-smoke.py <root> <venom> <out>')
root = Path(sys.argv[1]).resolve()
venom = Path(sys.argv[2]).resolve()
out = Path(sys.argv[3]).resolve()
if artifact_is_current(root):
    print('TurboJS module artifact gate: current VTJSMB04 artifact embedded')
    raise SystemExit(0)
if out.exists():
    shutil.rmtree(out)
result = subprocess.run(
    [str(venom), 'build', str(root / 'examples' / 'browser-compat-site'), '--out', str(out)],
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if result.returncode == 0:
    raise SystemExit('stale TurboJS module artifact was unexpectedly accepted')
marker = 'protected module graph requires TurboJS/WASM module bundle contract VTJSMB04'
if marker not in result.stdout:
    print(result.stdout)
    raise SystemExit('stale artifact rejection did not contain the expected fail-closed marker')
print('TurboJS module artifact gate: stale artifact rejected fail-closed')
