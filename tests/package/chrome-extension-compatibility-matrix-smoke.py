#!/usr/bin/env python3
from pathlib import Path
import json

root = Path(__file__).resolve().parents[2]
fixture_root = root / 'tests/fixtures/chrome-extensions'
matrix = json.loads((fixture_root / 'matrix.json').read_text(encoding='utf-8'))
assert matrix['schema'] == 1
assert matrix['manifest_version'] == 3
fixtures = matrix['fixtures']
assert len(fixtures) >= 12
assert len({item['name'] for item in fixtures}) == len(fixtures)
for item in fixtures:
    directory = fixture_root / item['name']
    assert directory.is_dir(), item['name']
    manifest = json.loads((directory / 'manifest.json').read_text(encoding='utf-8'))
    assert manifest['manifest_version'] == 3, item['name']
    assert item['tier'] in ('A', 'B', 'C')
    assert item['native'] is True

cpp = (root / 'src/pipeline/chrome_extension.cpp').read_text(encoding='utf-8')
for token in ('wildcard_match', 'manifest.default_locale', 'registerContentScripts',
              'offscreen.createDocument', 'selected Chrome extension runtime host'):
    assert token in cpp, token
browser = (root / 'tests/browser/chrome-extension-compatibility.py').read_text(encoding='utf-8')
for token in ('popup-page', 'service-worker-module', 'content-isolated', 'content-main', 'offscreen-runtime'):
    assert token in browser, token
print(f"Chrome extension compatibility matrix source: PASS ({len(fixtures)} fixtures)")
