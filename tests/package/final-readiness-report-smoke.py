#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as td:
    j=Path(td)/'final.json'; m=Path(td)/'final.md'
    p=subprocess.run([sys.executable,str(root/'tools/final_readiness_report.py'),'--repo-root',str(root),'--json-out',str(j),'--markdown-out',str(m)],text=True,capture_output=True,timeout=180)
    if p.returncode: raise SystemExit(p.stdout+p.stderr)
    data=json.loads(j.read_text(encoding='utf-8'))
    assert data['schema']=='VENOM_FINAL_READINESS_RESULT_V1'
    assert data['version']=='3.0.0' and data['passed'] is True
    assert all(x['passed'] for x in data['checks'])
    assert all(x['passed'] for x in data['executions'])
    assert 'Final Readiness' in m.read_text(encoding='utf-8')
print('final readiness report smoke: PASS')
