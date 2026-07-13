#!/usr/bin/env python3
import json, subprocess, sys, tempfile
from pathlib import Path
root=Path(sys.argv[1]).resolve()
out=Path(tempfile.mkdtemp())/'report.json'
cp=subprocess.run([sys.executable,str(root/'tools/runtime_performance.py'),'--repo-root',str(root),'--format','json','--json-out',str(out)],text=True,capture_output=True)
if cp.returncode:
    print(cp.stdout); print(cp.stderr,file=sys.stderr); raise SystemExit(cp.returncode)
r=json.loads(out.read_text())
assert r['schema']=='venom.runtime-performance.v1'
assert r['passed'] is True
assert r['quickjs_wasm']['raw_bytes']>100000
assert r['quickjs_wasm']['gzip_bytes']<r['quickjs_wasm']['raw_bytes']
assert r['quickjs_wasm']['brotli_bytes']<r['quickjs_wasm']['gzip_bytes']
assert r['quickjs_wasm']['release_abi_exports']==16
print('runtime performance budget smoke passed')
