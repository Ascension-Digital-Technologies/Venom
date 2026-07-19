#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
contract=json.loads((root/'contracts/runtime-performance.json').read_text())
assert contract['schema']=='VENOM_RUNTIME_PERFORMANCE_V1'
html=(root/'src/pipeline/html.cpp').read_text()
build=(root/'src/pipeline/build.cpp').read_text()
for token in ['HtmlPreloadHint','modulepreload','application/wasm','application/octet-stream']:
    assert token in html+build, token
assert contract['preloadPolicy']['persistentPlaintextCacheAllowed'] is False
with tempfile.TemporaryDirectory() as td:
    td=Path(td); report=td/'browser.json'
    report.write_text(json.dumps({'results':[{'bootStatus':{'durationMs':1200,'timeline':[{'phase':'package-load','state':'complete','durationMs':100},{'phase':'package-policy','state':'complete','durationMs':120},{'phase':'runtime-install','state':'complete','durationMs':450},{'phase':'route-decode','state':'complete','durationMs':20},{'phase':'script-execution','state':'complete','durationMs':40}]}}]}))
    out=td/'summary.json'; md=td/'summary.md'
    cp=subprocess.run([sys.executable,str(root/'tools/summarize_startup.py'),str(report),'--json-out',str(out),'--markdown-out',str(md)],capture_output=True,text=True)
    assert cp.returncode==0,cp.stdout+cp.stderr
    data=json.loads(out.read_text()); assert data['passed']; assert data['metrics']['complete']['medianMs']==1200
    assert 'Startup Performance' in md.read_text()
print('runtime startup performance smoke passed')
