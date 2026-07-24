#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, sys
root = Path(__file__).resolve().parents[2]
pkg = json.loads((root/'packages/vite/package.json').read_text())
assert pkg['name'] == '@venom/vite'
assert pkg['type'] == 'module'
source = (root/'packages/vite/src/index.js').read_text()
for token in ['virtual:venom-status','handleHotUpdate','configureServer','digestTree','vite-write-bundle']:
    assert token in source, token
node = subprocess.run(['node', str(root/'tests/integration/vite-plugin-smoke.mjs')], cwd=root, text=True, capture_output=True)
if node.returncode:
    print(node.stdout); print(node.stderr, file=sys.stderr); raise SystemExit(node.returncode)
print(node.stdout.strip())
