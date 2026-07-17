#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(sys.argv[1]).resolve(); td=Path(tempfile.mkdtemp()); dist=td/'dist'; dist.mkdir(); (dist/'a.js').write_text('x')
out=td/'out.json'
cp=subprocess.run([sys.executable,str(root/'tools/benchmark_runtime.py'),'--dist',str(dist),'--command',f'"{sys.executable}" -c "pass"','--command-runs','2','--json-out',str(out)],capture_output=True,text=True)
assert cp.returncode==0,(cp.stdout,cp.stderr)
r=json.loads(out.read_text()); assert r['schema']=='venom.performance.v3'; assert r['artifacts'][0]['js_bytes']==1; assert r['commands'][0]['timing']['runs']==2
bad=td/'bad.json'; bad.write_text(json.dumps({'profile':'smoke','budgets':[{'metric':'artifacts.0.bytes','operator':'max','threshold':0}]}))
cp=subprocess.run([sys.executable,str(root/'tools/benchmark_runtime.py'),'--dist',str(dist),'--budget-file',str(bad)],capture_output=True,text=True)
assert cp.returncode==60,(cp.stdout,cp.stderr)
print('runtime benchmark tool smoke passed')
