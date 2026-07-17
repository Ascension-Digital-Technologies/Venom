#!/usr/bin/env python3
from __future__ import annotations
import os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
env = dict(os.environ); env['SOURCE_DATE_EPOCH'] = '1704067200'
with tempfile.TemporaryDirectory(prefix='venom-html-script-type-') as tmp:
    root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'
    root.mkdir()
    (root / 'index.html').write_text('''<!doctype html><html><body>
<div id="app">ok</div>
<script type="application/ld+json">{"@context":"https://schema.org","not javascript": true}</script>
<script type="importmap">{"imports":{"pkg":"./pkg.js"}}</script>
<script type="text/plain">this definitely is not javascript: {{{</script>
<script data-note="a > b">globalThis.classicReady = true;</script>
<script type="module">export const moduleReady = true;</script>
</body></html>''', encoding='utf-8')
    run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out), '--quiet'],
                         text=True, capture_output=True, env=env)
    if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
    if not (out / 'assets/app/.vbc').exists(): raise SystemExit('protected package missing')
print('HTML script type smoke: PASS')
