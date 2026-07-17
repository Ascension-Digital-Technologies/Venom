#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path

root=Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parents[2]).resolve()
contract=json.loads((root/'contracts/release-certification.json').read_text(encoding='utf-8'))
assert contract['schema']=='VENOM_RELEASE_CERTIFICATION_V1'
assert {'linux-x64','windows-x64','macos-arm64'} <= set(contract['requiredPlatforms'])
assert {'chromium','firefox','webkit'} <= set(contract['requiredBrowsers'])
ids={x['id'] for x in contract['examples']}
assert {'protected-chess','aegis-operations','tsx-showcase','typescript-showcase','javascript-playground'} <= ids
for rel in ('tools/certify_release.py','tools/aggregate_certification.py','.github/workflows/certification.yml','docs/operations/release-certification.md'):
    assert (root/rel).is_file(), rel
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'out'
    subprocess.run([sys.executable,str(root/'tools/certify_release.py'),'--repo-root',str(root),'--static-only','--output-dir',str(out)],check=True,cwd=root)
    result=json.loads((out/'certification.json').read_text(encoding='utf-8'))
    assert result['schema']=='VENOM_RELEASE_CERTIFICATION_RESULT_V1'
    assert result['passed'] is True
    assert result['staticGates'] and all(x['passed'] for x in result['staticGates'])
    assert (out/'certification.md').is_file()
print('release certification smoke: PASS')
