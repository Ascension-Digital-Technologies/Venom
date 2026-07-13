#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
if len(sys.argv)!=2: raise SystemExit('usage: umd-compatibility-smoke.py <venom>')
exe=Path(sys.argv[1])
with tempfile.TemporaryDirectory() as td:
    d=Path(td)
    umd=d/'umd.js'; umd.write_text('(function(g,f){typeof exports==="object"&&typeof module!=="undefined"?module.exports=f(require("react")):g.X=f(g.React)})(this,function(React){return React});',encoding='utf-8')
    r=subprocess.run([str(exe),'compatibility','check',str(umd),'--format','json'],text=True,capture_output=True)
    assert r.returncode==0, (r.returncode,r.stdout,r.stderr)
    data=json.loads(r.stdout)
    assert data['compatible'] is True
    assert any(x['feature']=='javascript.umd-commonjs-branch' and x['status']=='partial' for x in data['findings'])
    raw=d/'raw.js'; raw.write_text('const x = require("node-only");',encoding='utf-8')
    r=subprocess.run([str(exe),'compatibility','check',str(raw),'--format','json'],text=True,capture_output=True)
    assert r.returncode==20, (r.returncode,r.stdout,r.stderr)
    data=json.loads(r.stdout)
    assert data['compatible'] is False
    assert any(x['feature']=='node.require' and x['status']=='unsupported' for x in data['findings'])
print('umd compatibility smoke: ok')
