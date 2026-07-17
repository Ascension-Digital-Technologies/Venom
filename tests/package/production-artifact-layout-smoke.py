#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys, tempfile
root=Path(__file__).resolve().parents[2]
venom=Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
with tempfile.TemporaryDirectory(prefix='venom-v091-') as td:
    out=Path(td)/'dist'
    cmd=[str(venom),'build',str(root/'tests'/'fixtures'/'production-site'),'--out',str(out),'--profile','prod','--runtime','wasm','--hashed','--strict-release','--vendor-cache',str(root/'tests'/'fixtures'/'remote-cache'),'--offline']
    subprocess.run(cmd,check=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
    subprocess.run([sys.executable,str(root/'tools'/'scan_protected_dist.py'),str(out)],check=True,timeout=30)
    roots=sorted(p.name for p in out.iterdir() if p.is_file())
    assert roots==['index.html'], roots
print('v1.0.1 production artifact layout smoke: PASS')
