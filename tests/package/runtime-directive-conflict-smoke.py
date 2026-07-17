#!/usr/bin/env python3
import pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
venom=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
lock=(root/'tests/fixtures/sites/directive-binding/venom.lock').read_text(encoding='utf-8')

def run(js, attrs='type="module"'):
    td=tempfile.TemporaryDirectory(); base=pathlib.Path(td.name); site=base/'site'; site.mkdir()
    (site/'index.html').write_text(f'<script {attrs} src="app.js"></script>',encoding='utf-8')
    (site/'app.js').write_text(js,encoding='utf-8')
    (site/'venom.lock').write_text(lock,encoding='utf-8')
    p=subprocess.run([str(venom),'build',str(site),'--out',str(base/'out'),'--profile','prod'],text=True,capture_output=True)
    return td,p

td,p=run('// @venom: browser\n// @venom: protected\nexport function x(){return 1;}\n')
if p.returncode==0 or 'conflicting file-scope Venom directives' not in p.stdout+p.stderr:
    raise SystemExit('conflicting file directives did not fail closed\n'+p.stdout+p.stderr)
td.cleanup()

td,p=run('// @venom: protected\nexport function x(){return 1;}\n','type="module" data-venom="browser"')
if p.returncode==0 or 'conflicting HTML data-venom and file-scope Venom directives' not in p.stdout+p.stderr:
    raise SystemExit('attribute/file conflict did not fail closed\n'+p.stdout+p.stderr)
td.cleanup()
print('[PASS] conflicting file and HTML runtime directives fail closed')
