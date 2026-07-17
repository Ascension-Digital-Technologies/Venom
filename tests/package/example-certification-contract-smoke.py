#!/usr/bin/env python3
import json
from pathlib import Path
root=Path(__file__).resolve().parents[2]; data=json.loads((root/'contracts/examples.json').read_text())
assert data['schema']=='VENOM_EXAMPLE_CERTIFICATION_V1'; assert data['buildProfile']=='prod'
ids={x['id'] for x in data['examples']}; actual={x.name for x in (root/'examples').iterdir() if x.is_dir()}
assert ids==actual,(ids,actual)
assert all(x['requiresRealQuickJs'] and x['leakScan'] and x['browser'] for x in data['examples'])
assert not any(x.get('profile')=='dev' for x in data['examples'])
print('example certification contract smoke: PASS')
