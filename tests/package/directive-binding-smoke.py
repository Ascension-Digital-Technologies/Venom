#!/usr/bin/env python3
import json, pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
venom=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
fixture=root/'tests/fixtures/sites/directive-binding'
with tempfile.TemporaryDirectory() as td:
    out=pathlib.Path(td)/'dev'
    p=subprocess.run([str(venom),'build',str(fixture),'--out',str(out),'--profile','prod'],text=True,capture_output=True)
    if p.returncode != 0: raise SystemExit('directive binding fixture failed\n'+p.stdout+p.stderr)
    closure=json.loads((out/'assets/app/build.json').read_text(encoding='utf-8')).get('protection_closure',{})
    if closure.get('requested') != 1 or closure.get('resolved') != 1:
        raise SystemExit('directive did not bind across comments: '+repr(closure))

with tempfile.TemporaryDirectory() as td:
    bad=pathlib.Path(td)/'site'; bad.mkdir()
    (bad/'index.html').write_text('<script type="module" src="app.js"></script>',encoding='utf-8')
    (bad/'app.js').write_text('// @venom: browser\nconst prefix = 0;\n// @venom: protected\nconst unrelated = 1;\n',encoding='utf-8')
    (bad/'venom.lock').write_text((fixture/'venom.lock').read_text(encoding='utf-8'),encoding='utf-8')
    p=subprocess.run([str(venom),'build',str(bad),'--out',str(pathlib.Path(td)/'out'),'--profile','prod'],text=True,capture_output=True)
    if p.returncode == 0 or 'orphaned function-level Venom directive' not in p.stdout+p.stderr:
        raise SystemExit('orphaned directive did not fail closed\n'+p.stdout+p.stderr)
print('[PASS] directives bind across comments and orphaned directives fail closed')
