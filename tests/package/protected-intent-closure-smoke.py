#!/usr/bin/env python3
import json, pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
venom=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
fixture=root/'tests/fixtures/sites/protected-intent-arrow'
with tempfile.TemporaryDirectory() as td:
    out=pathlib.Path(td)/'dev'
    p=subprocess.run([str(venom),'build',str(fixture),'--out',str(out),'--profile','dev'],text=True,capture_output=True)
    combined=p.stdout+p.stderr
    if p.returncode != 0: raise SystemExit('protected arrow function was not lowered\n'+combined)
    build=json.loads((out/'assets/app/build.json').read_text(encoding='utf-8'))
    closure=build.get('protection_closure',{})
    if closure.get('requested') != 1 or closure.get('resolved') != 1:
        raise SystemExit('protected arrow closure mismatch: '+repr(closure))
    if closure.get('expected_quickjs_records') != 1 or not closure.get('registry_present'):
        raise SystemExit('protected arrow registry missing: '+repr(closure))
print('[PASS] protected arrow function lowers into one protected registry record')
