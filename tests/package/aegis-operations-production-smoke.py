#!/usr/bin/env python3
from pathlib import Path
import shutil, subprocess, sys, tempfile
venom=Path(sys.argv[1]).resolve(); repo=Path(__file__).resolve().parents[2]; example=repo/'examples'/'aegis-operations'
with tempfile.TemporaryDirectory(prefix='venom-aegis-') as td:
    out=Path(td)/'dist'
    subprocess.run([str(venom),'build',str(example),'--out',str(out),'--profile','prod'],check=True)
    subprocess.run([str(venom),'verify',str(out),'--target','browser'],check=True)
    subprocess.run([str(venom),'verify-runtime',str(out),'--require-real-engine'],check=True)
    subprocess.run([sys.executable,str(repo/'tools/check_production_leaks.py'),str(out)],check=True)
    leaked=[p for p in out.rglob('*') if p.suffix.lower() in {'.ts','.tsx','.jsx','.mts','.cts'} and p.name!='venom-exports.d.ts']
    if leaked: raise SystemExit('source leak: '+', '.join(str(p.relative_to(out)) for p in leaked))
    if not any(p.suffix == '.vbc' for p in out.rglob('*')): raise SystemExit('missing VBC package')
