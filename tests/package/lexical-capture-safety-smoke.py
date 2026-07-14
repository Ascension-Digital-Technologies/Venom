#!/usr/bin/env python3
from __future__ import annotations
import pathlib, subprocess, sys, tempfile

root = pathlib.Path(__file__).resolve().parents[2]
venom = pathlib.Path(sys.argv[1])
lock = (root/'examples/protected-chess/venom.lock').read_text(encoding='utf-8')

def run_case(code: str, expect_ok: bool, marker: str = '') -> None:
    with tempfile.TemporaryDirectory(prefix='venom-scope-') as td:
        base=pathlib.Path(td); site=base/'site'; out=base/'out'; site.mkdir()
        (site/'index.html').write_text('<script data-venom="browser" src="app.js"></script>',encoding='utf-8')
        (site/'app.js').write_text(code,encoding='utf-8')
        (site/'venom.lock').write_text(lock,encoding='utf-8')
        p=subprocess.run([str(venom),'build',str(site),'--out',str(out),'--profile','dev'],text=True,capture_output=True)
        text=p.stdout+p.stderr
        if expect_ok and p.returncode != 0: raise SystemExit(text)
        if not expect_ok and (p.returncode == 0 or marker not in text):
            raise SystemExit(f'expected failure containing {marker!r}\n{text}')

run_case('''// @venom: browser\nconst SCALE=3;\nfunction square(v){return v*v;}\n// @venom: protected\nfunction score(v){return square(v)*SCALE;}\nasync function boot(){return await score(4);}\nboot();\n''', True)
run_case('''// @venom: browser\nlet SCALE=3;\n// @venom: protected isolated\nfunction score(v){return v*SCALE;}\nasync function boot(){return await score(4);}\nboot();\n''', False, 'isolated verification failed:')
run_case('''// @venom: browser\nconst bootMarker=1;\n// @venom: protected\nfunction title(){return document.title;}\nasync function boot(){return await title();}\nboot();\n''', False, 'browser-only lexical capture requires an explicit capability: document')
print('lexical capture safety smoke: PASS')
