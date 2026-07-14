#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]; TOOL=ROOT/'tools/dist_release_root.py'; PUB=ROOT/'tools/publish_dist.py'
def run(*args,ok=True):
 r=subprocess.run([sys.executable,*map(str,args)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 if ok and r.returncode: raise AssertionError(r.stdout)
 if not ok and not r.returncode: raise AssertionError('expected failure: '+r.stdout)
 return r
with tempfile.TemporaryDirectory() as td:
 d=Path(td); dist=d/'dist'; (dist/'assets/app').mkdir(parents=True)
 (dist/'index.html').write_text('<!doctype html>\n'); (dist/'assets/app/app.vbc').write_bytes(b'vbc')
 build={'product':'Venom','venom_version':'test','profile':'prod','security_target':'browser','runtime_abi_version':12,'package_format_version':1,'package_asset':'assets/app/app.vbc','package_sha256':'x','protection_closure':{'requested':1,'resolved':1,'registry_present':True,'whole_file_intents':0,'expected_quickjs_records':1}}
 (dist/'assets/app/build.json').write_text(json.dumps(build))
 priv=d/'private.pem'; pub=d/'public.pem'; subprocess.run(['openssl','genpkey','-algorithm','Ed25519','-out',priv],check=True); subprocess.run(['openssl','pkey','-in',priv,'-pubout','-out',pub],check=True)
 run(TOOL,dist,'--sign','--private-key',priv,'--public-key',pub)
 run(TOOL,dist,'--verify','--public-key',pub)
 run(PUB,dist,'--public-key',pub,'--out',d/'release.zip')
 (dist/'index.html').write_text('tampered')
 run(TOOL,dist,'--verify','--public-key',pub,ok=False)
 (dist/'VENOM_RELEASE_ROOT.sig').unlink(missing_ok=True)
 run(PUB,dist,'--public-key',pub,'--out',d/'bad.zip',ok=False)
print('PASS')
