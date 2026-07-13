#!/usr/bin/env python3
from pathlib import Path
import json, subprocess, sys, tempfile
root=Path(sys.argv[1]).resolve()
tool=root/'tools/check_reproducibility.py'
assert tool.is_file()
assert (root/'.github/workflows/reproducibility.yml').is_file()
assert 'publish-release-artifacts' in (root/'.github/workflows/release.yml').read_text()
with tempfile.TemporaryDirectory() as td:
    base=Path(td); a=base/'a'; b=base/'b'; a.mkdir(); b.mkdir()
    (a/'x').write_bytes(b'same'); (b/'x').write_bytes(b'same')
    p=subprocess.run([sys.executable,str(tool),str(a),str(b),'--format','json'],capture_output=True,text=True)
    assert p.returncode==0,p.stdout+p.stderr
    assert json.loads(p.stdout)['reproducible'] is True
    (b/'x').write_bytes(b'different')
    p=subprocess.run([sys.executable,str(tool),str(a),str(b),'--format','json'],capture_output=True,text=True)
    assert p.returncode==1
    assert json.loads(p.stdout)['changed']==['x']
print('v1.4.0 production assurance smoke passed')
