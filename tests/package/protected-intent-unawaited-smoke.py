#!/usr/bin/env python3
import pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
venom=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
fixture=root/'tests/fixtures/sites/protected-intent-unawaited'
with tempfile.TemporaryDirectory() as td:
    out=pathlib.Path(td)/'prod'
    p=subprocess.run([str(venom),'build',str(fixture),'--out',str(out),'--profile','prod'],text=True,capture_output=True)
    if p.returncode == 0: raise SystemExit('production accepted an unawaited protected call')
    if 'VNM-PROT-1001' not in p.stdout+p.stderr: raise SystemExit('missing unresolved-intent diagnostic')
print('[PASS] unawaited protected arrow call fails closed')
