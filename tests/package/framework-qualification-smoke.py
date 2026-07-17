#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
tool=root/'tools/framework_qualification.py'
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'report.json'; md=Path(td)/'matrix.md'
    p=subprocess.run([sys.executable,str(tool),'--repo-root',str(root),'--json-out',str(out),'--markdown-out',str(md)],text=True,capture_output=True)
    if p.returncode: raise SystemExit(p.stdout+p.stderr)
    data=json.loads(out.read_text(encoding='utf-8'))
    assert data['schema']=='venom.framework-qualification.v1'
    assert data['passed'] is True
    names={x['name'] for x in data['frameworks']}
    assert {'React','Vue','Svelte'} <= names
    assert all(x['scenario_count']>0 for x in data['frameworks'])
    text=md.read_text(encoding='utf-8')
    for token in ('React','Vue','Svelte','qualified-pattern'): assert token in text
print('framework qualification smoke: PASS')
