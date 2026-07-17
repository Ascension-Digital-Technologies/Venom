#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as td:
    root = Path(td)
    bad = root / 'missing-dist'
    proc = subprocess.run([str(venom), 'verify', str(bad)], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if proc.returncode == 0:
        raise SystemExit('verify unexpectedly succeeded')
    if 'Protection report:' in proc.stdout:
        raise SystemExit('normal verification leaked verbose report')
    proc = subprocess.run([str(venom), 'verify', str(bad), '--format', 'json'], text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    # Invalid input may fail before report generation, but JSON mode must never emit ANSI.
    if '\x1b[' in proc.stdout + proc.stderr:
        raise SystemExit('JSON output contained ANSI')
print('console output smoke: PASS')
