#!/usr/bin/env python3
from __future__ import annotations
import os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
env = dict(os.environ); env['SOURCE_DATE_EPOCH'] = '1704067200'
with tempfile.TemporaryDirectory(prefix='venom-diagnostic-') as tmp:
    root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'
    root.mkdir()
    (root / 'index.html').write_text('<!doctype html><script src="app.js"></script>\n', encoding='utf-8')
    source = root / 'app.js'
    source.write_text('const ok = 1;\nfunction broken( {\n  return ok;\n}\n', encoding='utf-8')
    run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out)],
                         text=True, capture_output=True, env=env)
    text = run.stdout + run.stderr
    if run.returncode == 0: raise SystemExit('malformed JavaScript unexpectedly built')
    required = ['error[VENOM-E2101]: JavaScript parse failed', '  --> app.js:', ' | ', '^',
                '= help:', 'docs/reference/diagnostics.md#venom-e2101']
    missing = [item for item in required if item not in text]
    if missing: raise SystemExit(f'missing diagnostic fields {missing}:\n{text}')
print('Source-aware diagnostics smoke: PASS')
