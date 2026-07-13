#!/usr/bin/env python3
import json
from pathlib import Path
root=Path(__file__).resolve().parents[2]
fixture=root/'examples'/'production-bundle-site'
manifest=json.loads((fixture/'venom.browser.json').read_text(encoding='utf-8'))
assert manifest['schema_version']==1
assert manifest['profile']=='production-bundler-output'
assert manifest['evidence_level']=='behavioral'
assert 'hashed-static-assets' in manifest['claims']
assert 'minified-es-modules' in manifest['claims']
index=(fixture/'index.html').read_text(encoding='utf-8')
assert 'assets/index.7ab31f.js' in index
assert 'assets/index.84c1d2.css' in index
entry=(fixture/'assets'/'index.7ab31f.js').read_text(encoding='utf-8')
assert './vendor.31e42a.js' in entry
assert './chunk-list.c83f11.js' in entry
assert (fixture/'about.html').is_file()
print('production bundle fixture smoke: ok')
