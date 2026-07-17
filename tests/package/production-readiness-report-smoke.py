#!/usr/bin/env python3
import json, os, subprocess, sys, tempfile
from pathlib import Path

root=Path(__file__).resolve().parents[2]
tool=root/'tools'/'production_readiness.py'
with tempfile.TemporaryDirectory() as td:
    d=Path(td); site=d/'site'; site.mkdir(); (site/'index.html').write_text('<!doctype html><script>console.log(1)</script>',encoding='utf-8')
    fake=d/('venom.bat' if os.name=='nt' else 'venom')
    if os.name=='nt':
        fake.write_text('@echo off\nif "%1"=="--version" (echo venom 2.0.0& exit /b 0)\nif "%1"=="compatibility" (echo {"schema_version":1,"compatible":true,"findings":[]}& exit /b 0)\nexit /b 1\n')
    else:
        fake.write_text('#!/bin/sh\nif [ "$1" = "--version" ]; then echo "venom 2.0.0"; exit 0; fi\nif [ "$1" = "compatibility" ]; then echo "{\\"schema_version\\":1,\\"compatible\\":true,\\"findings\\":[]}"; exit 0; fi\nexit 1\n')
        fake.chmod(0o755)
    p=subprocess.run([sys.executable,str(tool),str(site),'--venom',str(fake),'--format','json'],text=True,capture_output=True)
    if p.returncode != 0:
        raise SystemExit(p.stdout+p.stderr)
    data=json.loads(p.stdout)
    assert data['ready'] is True
    assert data['warnings'] >= 1
    ids={c['id'] for c in data['checks']}
    assert 'source.compatibility' in ids and 'dist.supplied' in ids
print('production readiness report smoke: ok')
