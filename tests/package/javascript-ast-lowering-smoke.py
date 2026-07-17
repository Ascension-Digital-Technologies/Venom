#!/usr/bin/env python3
from __future__ import annotations
import json, os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
env = dict(os.environ); env['SOURCE_DATE_EPOCH'] = '1704067200'

def build(script: str, expect_success: bool = True, profile: str = "prod"):
    with tempfile.TemporaryDirectory(prefix='venom-ast-lowering-') as tmp:
        root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'
        root.mkdir()
        (root / 'index.html').write_text('<!doctype html><script data-venom="browser" src="app.js"></script>\n', encoding='utf-8')
        (root / 'app.js').write_text(script, encoding='utf-8')
        run = subprocess.run([str(venom), 'build', str(root), '--profile', profile, '--out', str(out)],
                             text=True, capture_output=True, env=env)
        if expect_success:
            if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
            report = json.loads((out / 'build/reports/bridge-rewrite-plan.json').read_text(encoding='utf-8'))
            browser = next((out / 'assets').rglob('*.js')).read_text(encoding='utf-8')
            return report, browser
        if run.returncode == 0: raise SystemExit('expected AST lowering rejection')
        return run.stdout + run.stderr

report, browser = build('''// @venom: browser
const SCALE = 3;
// @venom: protected
const evaluate = async ({ value = 2 }, factor = SCALE) => ({ score: value * factor });
async function boot() { return await evaluate({ value: 4 }); }
boot();
''')
record = next(item for item in report['functions'] if item['function'] == 'evaluate')
if record['status'] != 'extracted': raise SystemExit(record)
if 'async-arrow-function extracted' not in record['reason']: raise SystemExit(record)
if set(record['lifted_dependencies']) != {'SCALE'}: raise SystemExit(record)
if 'value * factor' in browser: raise SystemExit('protected arrow body remained in browser output')

report, browser = build('''// @venom: browser
const OFFSET = 7;
// @venom: protected
const evaluate = function named({ value }, extra = OFFSET) { return value + extra; };
async function boot() { return await evaluate({ value: 4 }); }
boot();
''')
record = next(item for item in report['functions'] if item['function'] == 'evaluate')
if record['status'] != 'extracted': raise SystemExit(record)
if 'function-expression extracted' not in record['reason']: raise SystemExit(record)
if 'return value + extra' in browser: raise SystemExit('protected function expression remained in browser output')

failure = build('''// @venom: browser
const marker = 1;
// @venom: protected
function* evaluate() { yield 1; }
async function boot() { return await evaluate(); }
boot();
''', expect_success=False, profile='prod')
if 'error[VENOM-E2303]: Generator protected functions are not supported' not in failure: raise SystemExit(failure)
if 'docs/reference/diagnostics.md#venom-e2303' not in failure: raise SystemExit(failure)

print('JavaScript AST lowering smoke: PASS')
