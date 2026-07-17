#!/usr/bin/env python3
from __future__ import annotations
import json, os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
env = dict(os.environ); env['SOURCE_DATE_EPOCH'] = '1704067200'

def build(script: str, expect_success: bool = True):
    with tempfile.TemporaryDirectory(prefix='venom-ast-scope-') as tmp:
        root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'
        root.mkdir()
        (root / 'index.html').write_text('<!doctype html><script data-venom="browser" src="app.js"></script>\n', encoding='utf-8')
        (root / 'app.js').write_text(script, encoding='utf-8')
        run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out)],
                             text=True, capture_output=True, env=env)
        if expect_success:
            if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
            return json.loads((out / 'build/reports/bridge-rewrite-plan.json').read_text(encoding='utf-8'))
        if run.returncode == 0: raise SystemExit('expected AST scope rejection')
        return run.stdout + run.stderr

report = build('''// @venom: browser
const SCALE = 3;
// window and document in comments must not become captures.
function helper({ value }) {
  const inner = factor => factor * value;
  return inner(SCALE);
}
// @venom: protected
function evaluate({ value }) { return helper({ value }) + SCALE; }
async function boot() { return await evaluate({ value: 4 }); }
boot();
''')
record = next(item for item in report['functions'] if item['function'] == 'evaluate')
if record['status'] != 'extracted': raise SystemExit(record)
if set(record['lifted_dependencies']) != {'SCALE', 'helper'}: raise SystemExit(record)

failure = build('''// @venom: browser
let count = 1;
// @venom: protected
function evaluate(value) { return value + count; }
async function boot() { return await evaluate(4); }
boot();
''', expect_success=False)
if 'mutable lexical capture is not safe to lift: count' not in failure: raise SystemExit(failure)

failure = build('''// @venom: browser
function helper() { return document.title; }
// @venom: protected
function evaluate() { return helper(); }
async function boot() { return await evaluate(); }
boot();
''', expect_success=False)
if 'browser-only lexical capture requires an explicit capability: document' not in failure: raise SystemExit(failure)

print('JavaScript AST scope smoke: PASS')
