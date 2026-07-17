#!/usr/bin/env python3
import pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
venom=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
lock=(root/'tests/fixtures/sites/directive-binding/venom.lock').read_text(encoding='utf-8')

def run(js):
    td=tempfile.TemporaryDirectory(); base=pathlib.Path(td.name); site=base/'site'; site.mkdir()
    (site/'index.html').write_text('<script type="module" src="app.js"></script>',encoding='utf-8')
    (site/'app.js').write_text(js,encoding='utf-8')
    (site/'venom.lock').write_text(lock,encoding='utf-8')
    p=subprocess.run([str(venom),'build',str(site),'--out',str(base/'out'),'--profile','prod'],text=True,capture_output=True)
    return td,p

for source in (
    '// @venom: protectedd\nexport function x(){return 1;}\n',
    '// @venom: notbrowser\nexport function x(){return 1;}\n',
    '// @venom: browser isolated\nexport function x(){return 1;}\n',
):
    td,p=run(source)
    if p.returncode == 0 or 'malformed file-scope Venom runtime directive' not in p.stdout+p.stderr:
        raise SystemExit('malformed file directive did not fail closed\n'+p.stdout+p.stderr)
    td.cleanup()

td,p=run('// @venom: browser\nconst x=1;\n// @venom: protected extra\nexport function y(){return x;}\n')
if p.returncode == 0 or 'malformed function-level Venom runtime directive' not in p.stdout+p.stderr:
    raise SystemExit('malformed function directive did not fail closed\n'+p.stdout+p.stderr)
td.cleanup()
print('[PASS] exact runtime directive grammar rejects typos and illegal modifiers')
