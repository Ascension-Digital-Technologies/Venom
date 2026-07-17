#!/usr/bin/env python3
import pathlib, subprocess, sys, tempfile
venom=pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='venom-capability-policy-') as td:
    root=pathlib.Path(td); site=root/'site'; site.mkdir()
    (site/'index.html').write_text('<!doctype html><script src="app.js"></script>\n', encoding='utf-8')
    (site/'app.js').write_text('// @venom: browser\nasync function load(){ return fetch("/data.json"); }\n', encoding='utf-8')
    config=root/'venom.toml'
    config.write_text('[project]\ninput="site"\noutput="dist"\n[build]\nprofile="prod"\n[capabilities]\nfetch="deny"\ntimers="auto"\nstorage="auto"\ncrypto="auto"\n', encoding='utf-8')
    denied=subprocess.run([str(venom),'build','--config',str(config)], text=True, capture_output=True)
    if denied.returncode == 0 or "capability 'fetch' is required" not in denied.stderr + denied.stdout:
        raise SystemExit('denied fetch capability did not fail closed')
    config.write_text(config.read_text().replace('fetch="deny"','fetch="allow"'), encoding='utf-8')
    shown=subprocess.run([str(venom),'config','print',str(config),'--format','json'], text=True, capture_output=True, check=True)
    if '"fetch":"allow"' not in shown.stdout or '"storage":"auto"' not in shown.stdout:
        raise SystemExit('capability policy missing from config output')
print('capability policy smoke: PASS')
