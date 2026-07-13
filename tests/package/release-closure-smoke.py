#!/usr/bin/env python3
from pathlib import Path
import json,re,sys,tempfile,subprocess
root=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'closure.json'
    p=subprocess.run([sys.executable,str(root/'tools/release_closure.py'),'--repo-root',str(root),'--json-out',str(out)],text=True,capture_output=True)
    if p.returncode: raise SystemExit(p.stdout+p.stderr)
    data=json.loads(out.read_text())
    assert data['schema']=='VENOM_RELEASE_CLOSURE_V1'
    assert data['release_abi_export_count']==23
    assert data['emscripten_version'] and data['emscripten_version']!='latest'
print('release closure smoke: PASS')
