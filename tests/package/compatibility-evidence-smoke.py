#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as td:
    d=Path(td); report=d/'r.json'; fixture=d/'venom.browser.json'; matrix=d/'m.json'; bundle=d/'e.zip'; bundle2=d/'e2.zip'
    report.write_text(json.dumps({'fixture_id':'x','support_tier':'probe','results':[{'browser':'chromium','passed':True,'checks':[1,2,3]}]}))
    fixture.write_text(json.dumps({'schema_version':1,'id':'x','scenarios':[{}]}))
    matrix.write_text(json.dumps({'schema_version':2,'passed':True,'support_policy_passed':True,'required_browsers':['chromium'],'minimum_checks_per_browser':3}))
    subprocess.run([sys.executable,str(ROOT/'tools/package_compatibility_evidence.py'),'--matrix',str(matrix),'--reports',str(report),'--manifests',str(fixture),'--out',str(bundle)],check=True)
    subprocess.run([sys.executable,str(ROOT/'tools/verify_compatibility_evidence.py'),str(bundle)],check=True)
    subprocess.run([sys.executable,str(ROOT/'tools/package_compatibility_evidence.py'),'--matrix',str(matrix),'--reports',str(report),'--manifests',str(fixture),'--out',str(bundle2)],check=True)
    assert bundle.read_bytes()==bundle2.read_bytes(), 'evidence bundle is not deterministic'
    with zipfile.ZipFile(bundle,'a') as z: z.writestr('compatibility-evidence/r.json','tampered')
    r=subprocess.run([sys.executable,str(ROOT/'tools/verify_compatibility_evidence.py'),str(bundle)])
    assert r.returncode==60
print('compatibility evidence smoke: ok')
