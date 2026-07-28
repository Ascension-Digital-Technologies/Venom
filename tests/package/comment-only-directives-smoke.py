#!/usr/bin/env python3
from _private_reports import read_json, root_for
import json, pathlib, subprocess, sys, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
venom=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'build'/'venom'
fixture=root/'tests/fixtures/sites/comment-only-directives'
with tempfile.TemporaryDirectory() as td:
    out=pathlib.Path(td)/'dev'
    p=subprocess.run([str(venom),'build',str(fixture),'--out',str(out),'--profile','prod'],text=True,capture_output=True)
    if p.returncode != 0: raise SystemExit('comment-aware directive fixture failed\n'+p.stdout+p.stderr)
    closure=read_json(out).get('protection_closure',{})
    if closure.get('requested') != 1 or closure.get('resolved') != 1 or closure.get('expected_turbojs_records') != 1:
        raise SystemExit('string/template text was mistaken for a directive: '+repr(closure))
print('[PASS] only JavaScript comments can carry function-level Venom directives')
