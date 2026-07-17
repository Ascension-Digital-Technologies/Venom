#!/usr/bin/env python3
from __future__ import annotations
import json, os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'; cache = Path(tmp) / 'cache'
    root.mkdir()
    (root/'index.html').write_text('<!doctype html><script type="module" src="main.js"></script>\n', encoding='utf-8')
    (root/'main.js').write_text("""// import './fake-comment.js'\nconst fake = \"export * from './fake-string.js'\";\nimport { value } from './dep.js';\nexport { other } from './other.js';\nasync function load(){ return import('./lazy.js'); }\nconsole.log(value, fake, load);\n""", encoding='utf-8')
    (root/'dep.js').write_text('export const value = 1;\n', encoding='utf-8')
    (root/'other.js').write_text('export const other = 2;\n', encoding='utf-8')
    (root/'lazy.js').write_text('export default 3;\n', encoding='utf-8')
    env = dict(os.environ); env['SOURCE_DATE_EPOCH']='1704067200'
    run = subprocess.run([str(venom),'build',str(root),'--profile','prod','--out',str(out),'--cache-dir',str(cache),'--verbose'], text=True, capture_output=True, env=env)
    if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
    if 'module graph: 3 edges' not in run.stdout + run.stderr: raise SystemExit('AST module graph did not resolve all static/dynamic imports')
    ir = json.loads((cache/'project-ir.json').read_text(encoding='utf-8'))
    if ir.get('version') != 12 or ir.get('module_edge_count') != 3: raise SystemExit('unexpected project IR AST metadata')
    modules = {m['source']:m for m in ir['protected_modules']}
    if set(modules) != {'main.js','dep.js','other.js','lazy.js'}: raise SystemExit(f'unexpected AST module set: {set(modules)}')
    if modules['main.js']['ast_imports'] != 1 or modules['main.js']['ast_exports'] != 1: raise SystemExit('AST import/export counts are wrong')
    if modules['main.js']['ast_lexical_scopes'] < 2 or modules['main.js']['ast_global_references'] < 1: raise SystemExit('AST scope metadata is missing')
    if any('fake-' in name for name in modules): raise SystemExit('comment/string false-positive became a module')
print('JavaScript AST frontend smoke: PASS')
