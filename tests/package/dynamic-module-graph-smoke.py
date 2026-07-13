#!/usr/bin/env python3
import json, pathlib, subprocess, sys, tempfile
venom=pathlib.Path(sys.argv[1]).resolve()
repo=pathlib.Path(__file__).resolve().parents[2]
site=repo/'examples'/'dynamic-bundle-site'
with tempfile.TemporaryDirectory() as td:
    out=pathlib.Path(td)/'dist'
    r=subprocess.run([str(venom),'build',str(site),'--out',str(out),'--format','json'],text=True,capture_output=True)
    if r.returncode==0:
        meta=json.loads((out/'assets'/'app'/'build.json').read_text())
        graph=meta['module_graph']
        assert graph['chunks']==3,graph
        assert graph['edges']>=4,graph
        assert graph['dynamic_literal_edges']==1,graph
    else:
        assert 'VQJSMB04' in (r.stdout+r.stderr),(r.stdout,r.stderr)
        assert 'build-quickjs-wasm' in (r.stdout+r.stderr),(r.stdout,r.stderr)
    c=subprocess.run([str(venom),'compatibility','check',str(site),'--format','json'],text=True,capture_output=True)
    assert c.returncode==0,(c.stdout,c.stderr)
    report=json.loads(c.stdout)
    assert report['capability_graph']['module_features']['dynamic_import']==1
    assert any(f['feature']=='javascript.dynamic-import' and f['status']=='partial' for f in report['findings'])
print('v1.29 dynamic module graph smoke: PASS')
