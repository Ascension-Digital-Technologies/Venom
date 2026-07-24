#!/usr/bin/env python3
import json
from pathlib import Path
root=Path(__file__).resolve().parents[2]
fixture=root/'tests'/'fixtures'/'sites'/'react16-umd-site'
manifest=json.loads((fixture/'venom.browser.json').read_text(encoding='utf-8'))
assert manifest['schema_version']==1
assert manifest['id']=='react16-umd-production'
assert manifest['profile']=='pinned-framework-production'
assert manifest['evidence_level']=='behavioral'
assert {'react-16-umd','react-dom-render','react-class-state','react-event-update'} <= set(manifest['claims'])
scenario=manifest['scenarios'][0]
assert any(a.get('type')=='click' and a.get('selector')=='#increment' for a in scenario.get('actions',[]))
assert any(c.get('id')=='updated-count' and c.get('equals')=='1' for c in scenario['checks'])
index=(fixture/'index.html').read_text(encoding='utf-8')
assert 'react.16.0.0.production.min.js' in index
assert 'react-dom.16.0.1.production.min.js' in index
for rel in ['assets/react.16.0.0.production.min.js','assets/react-dom.16.0.1.production.min.js','assets/REACT-MIT-LICENSE.txt']:
    assert (fixture/rel).is_file() and (fixture/rel).stat().st_size>100
print('react16 framework fixture smoke: ok')
