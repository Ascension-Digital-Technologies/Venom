#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(sys.argv[1]); exe=Path(sys.argv[2])
subprocess.run([sys.executable,str(root/'tools/generate_host_api.py'),'--check'],check=True)
contracts=json.loads(subprocess.check_output([str(exe),'contracts','--format','json'],text=True))
assert contracts['host_bridge_abi']==3 and contracts['config_schema']==1
with tempfile.TemporaryDirectory() as td:
 p=Path(td); (p/'index.html').write_text('<script>eval("1")</script>')
 r=subprocess.run([str(exe),'compatibility','check',str(p),'--format','json'],text=True,capture_output=True)
 assert r.returncode==20, r
 data=json.loads(r.stdout); assert not data['compatible']; assert data['findings'][0]['feature']=='javascript.eval'
print('v1.2.0 contract smoke passed')
