#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(sys.argv[1] if len(sys.argv)>1 else Path(__file__).resolve().parents[2]).resolve()
with tempfile.TemporaryDirectory() as td:
    td=Path(td); reports=[]
    for pid in ('linux-x64','windows-x64','macos-arm64'):
        p=td/f'{pid}.json'; p.write_text(json.dumps({'passed':True,'environment':{'platformId':pid}}),encoding='utf-8'); reports.append(p)
    matrix=td/'matrix.json'; matrix.write_text(json.dumps({'passed':True,'rows':[{'browser':b,'passed':True} for b in ('chromium','firefox','webkit')]}),encoding='utf-8')
    out=td/'final.json'; md=td/'final.md'
    cmd=[sys.executable,str(root/'tools/aggregate_certification.py'),'--contract',str(root/'contracts/release-certification.json'),'--browser-matrix',str(matrix),'--json-out',str(out),'--markdown-out',str(md)]
    for p in reports: cmd += ['--platform-report',str(p)]
    subprocess.run(cmd,check=True,cwd=root)
    data=json.loads(out.read_text(encoding='utf-8')); assert data['passed'] is True
    assert not data['missingPlatforms'] and not data['missingBrowsers'] and md.is_file()
    missing=subprocess.run([sys.executable,str(root/'tools/aggregate_certification.py'),'--contract',str(root/'contracts/release-certification.json'),'--platform-report',str(reports[0]),'--browser-matrix',str(matrix),'--json-out',str(td/'missing.json'),'--markdown-out',str(td/'missing.md')])
    assert missing.returncode != 0
print('aggregated certification smoke: PASS')
