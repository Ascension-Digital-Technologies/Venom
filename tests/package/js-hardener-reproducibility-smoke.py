#!/usr/bin/env python3
from pathlib import Path
import json, sys
root = Path(__file__).resolve().parents[2]
pkg = json.loads((root/'tools/js-hardener/package.json').read_text())
lock = json.loads((root/'tools/js-hardener/package-lock.json').read_text())
assert pkg.get('dependencies') == {'javascript-obfuscator':'5.4.7','terser':'5.49.0'}
assert lock.get('lockfileVersion') in (2,3)
root_pkg = lock.get('packages',{}).get('',{})
assert root_pkg.get('dependencies') == pkg['dependencies']
for script in ('scripts/setup-js-hardener.sh','scripts/setup-js-hardener.ps1'):
    text=(root/script).read_text()
    assert 'npm ci' in text or "@('ci'" in text
    assert 'npm install' not in text
print('js hardener reproducibility smoke: PASS')
