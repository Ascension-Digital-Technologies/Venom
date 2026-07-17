#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as td:
    site=Path(td); (site/'a.ts').write_text('// @venom: browser\nimport {x} from "./b";\n'); (site/'b.ts').write_text('// @venom: protected\nexport const x=1;\n')
    cp=subprocess.run([sys.executable,str(root/'tools/venom_query.py'),str(site),'boundaries','--json'],capture_output=True,text=True,timeout=20)
    assert cp.returncode==0,cp.stderr; data=json.loads(cp.stdout); assert data and data[0]['resolved']=='b.ts'
print('module query smoke passed')
