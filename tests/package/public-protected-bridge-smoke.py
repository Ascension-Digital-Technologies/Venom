#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile, sys
root=Path(__file__).resolve().parents[2]
venom=Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'dev'/'venom'
with tempfile.TemporaryDirectory() as td:
    out=Path(td)/'dist'
    subprocess.run([str(venom),'build',str(root/'examples/protected-chess'),'--out',str(out)],check=True,stdout=subprocess.DEVNULL)
    subprocess.run([str(venom),'verify-runtime',str(out),'--target','browser','--require-real-engine'],check=True,stdout=subprocess.DEVNULL)
    subprocess.run([sys.executable,str(root/'scripts/check-production-leaks.py'),str(out)],check=True,stdout=subprocess.DEVNULL)
    loader=next((out/'assets'/'loader').glob('loader.*.js'))
    worker=next((out/'assets'/'workers').glob('worker.*.js'))
    subprocess.run(['node','--check',str(loader)],check=True,stdout=subprocess.DEVNULL)
    subprocess.run(['node','--check',str(worker)],check=True,stdout=subprocess.DEVNULL)
source=(root/'src/compiler/pipeline/js.cpp').read_text(encoding='utf-8')
source += (root / 'src/compiler/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
worker_source=(root/'src/generated/runtime/worker_runtime_js.cpp').read_text(encoding='utf-8')
for token in ["Object.defineProperty(globalThis,'venom'", 'venomApi=Object.freeze', '__venomInvokeProtectedByName', 'candidateSlot', 'bridgeSession', 'bridgeCounter']:
    if token not in source: raise SystemExit('public bridge source missing: '+token)
for token in ['BRIDGE_CANDIDATES[candidateSlot]', 'counter <= bridgeCounter', 'stale or replayed bridge request']:
    if token not in worker_source: raise SystemExit('worker bridge source missing: '+token)
if '__venomEngineWorker' in source: raise SystemExit('raw worker leaked through generated loader')
print('public protected bridge smoke: PASS')
