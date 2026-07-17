#!/usr/bin/env python3
from pathlib import Path
import json, sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
contract=json.loads((root/'contracts/release-certification.json').read_text(encoding='utf-8'))
assert contract['schema']=='VENOM_RELEASE_CERTIFICATION_V1'
assert contract['version'] >= 1
assert len(contract['staticGates']) >= 8
assert len(contract['examples']) >= 5
assert all((root/x['site']).is_dir() for x in contract['examples'])
assert all(x.get('required') is True for x in contract['browserFixtures'])
for rel in ('tools/certify_release.py','tools/aggregate_certification.py','.github/workflows/certification.yml','docs/operations/release-certification.md'):
    assert (root/rel).is_file(), rel
print('release certification contract: PASS')
