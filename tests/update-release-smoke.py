#!/usr/bin/env python3
import hashlib,json,subprocess,sys,tempfile,zipfile
from pathlib import Path
repo=Path(__file__).resolve().parents[1]; tool=repo/'tools'/'update_release.py'
with tempfile.TemporaryDirectory() as td:
 p=Path(td); (p/'artifacts').mkdir(); pkg=p/'artifacts'/'venom-v1.47.0-linux-x64.zip'
 with zipfile.ZipFile(pkg,'w') as z:
  z.writestr('venom-v1.47.0-linux-x64/CONTRACTS.json',json.dumps({'contracts':{'public_bridge_api':1}}))
 m={'schema_version':1,'product':'Venom - Secure Web Runtime','version':'1.47.0','packages':[{'name':pkg.name,'size':pkg.stat().st_size,'sha256':hashlib.sha256(pkg.read_bytes()).hexdigest()}]}
 (p/'RELEASE_SET.json').write_text(json.dumps(m))
 r=subprocess.run([sys.executable,str(tool),'check','--manifest',str(p/'RELEASE_SET.json'),'--format','json'],text=True,capture_output=True)
 if r.returncode: print(r.stdout,r.stderr); raise SystemExit(r.returncode)
 data=json.loads(r.stdout); assert data['available']=='1.47.0' and data['update_available']
print('update release smoke: PASS')
