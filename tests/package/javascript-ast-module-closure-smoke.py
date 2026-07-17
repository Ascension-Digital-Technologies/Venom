#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='venom-ast-module-closure-') as tmp:
    root = Path(tmp)
    (root / 'browser.js').write_text("export function title(){ return document.title; }\n", encoding='utf-8')
    (root / 'middle.js').write_text("import { title } from './browser.js'; export function wrapped(){ return title(); }\n", encoding='utf-8')
    (root / 'entry.js').write_text("import { wrapped } from './middle.js'; export function entry(){ return wrapped(); }\n", encoding='utf-8')
    (root / 'mixed.js').write_text(
        "export function safe(value){ return value * 2; }\n"
        "export function browserOnly(){ return document.title; }\n",
        encoding='utf-8')
    (root / 'safe-entry.js').write_text(
        "import { safe as calculate } from './mixed.js'; export function run(value){ return calculate(value); }\n",
        encoding='utf-8')
    (root / 'browser-entry.js').write_text(
        "import { browserOnly } from './mixed.js'; export function run(){ return browserOnly(); }\n",
        encoding='utf-8')
    (root / 'namespace-entry.js').write_text(
        "import * as mixed from './mixed.js'; export function run(value){ return mixed.safe(value); }\n",
        encoding='utf-8')
    (root / 'side-effect-entry.js').write_text(
        "import './mixed.js'; export function run(value){ return value; }\n",
        encoding='utf-8')
    (root / 'cycle-a.js').write_text("import { b } from './cycle-b.js'; export function a(n){ return n ? b(n-1) : 0; }\n", encoding='utf-8')
    (root / 'cycle-b.js').write_text("import { a } from './cycle-a.js'; export function b(n){ return n ? a(n-1) : 0; }\n", encoding='utf-8')
    (root / 'external.js').write_text("import thing from 'third-party'; export function use(){ return thing(); }\n", encoding='utf-8')
    (root / 'default-safe.js').write_text("export default function calculate(value){ return value * 3; }\n", encoding='utf-8')
    (root / 'default-browser.js').write_text("export default function title(){ return document.title; }\n", encoding='utf-8')
    (root / 'default-anonymous.js').write_text("export default function(value){ return value * 5; }\n", encoding='utf-8')
    (root / 'default-safe-entry.js').write_text("import calculate from './default-safe.js'; export function run(value){ return calculate(value); }\n", encoding='utf-8')
    (root / 'default-browser-entry.js').write_text("import title from './default-browser.js'; export function run(){ return title(); }\n", encoding='utf-8')
    (root / 'local-alias.js').write_text("function calculate(value){ return value + 4; } export { calculate as renamed };\n", encoding='utf-8')
    (root / 'local-alias-entry.js').write_text("import { renamed } from './local-alias.js'; export function run(value){ return renamed(value); }\n", encoding='utf-8')
    (root / 'barrel.js').write_text("export { safe as calculate, browserOnly as title } from './mixed.js';\n", encoding='utf-8')
    (root / 'barrel-safe-entry.js').write_text("import { calculate } from './barrel.js'; export function run(value){ return calculate(value); }\n", encoding='utf-8')
    (root / 'barrel-browser-entry.js').write_text("import { title } from './barrel.js'; export function run(){ return title(); }\n", encoding='utf-8')
    (root / 'default-barrel.js').write_text("export { default as calculate } from './default-safe.js';\n", encoding='utf-8')
    (root / 'default-barrel-entry.js').write_text("import { calculate } from './default-barrel.js'; export function run(value){ return calculate(value); }\n", encoding='utf-8')
    (root / 'star-barrel.js').write_text("export * from './mixed.js';\n", encoding='utf-8')
    (root / 'star-safe-entry.js').write_text("import { safe } from './star-barrel.js'; export function run(value){ return safe(value); }\n", encoding='utf-8')
    (root / 'star-browser-entry.js').write_text("import { browserOnly } from './star-barrel.js'; export function run(){ return browserOnly(); }\n", encoding='utf-8')
    (root / 'dynamic-literal.js').write_text("export async function load(){ return import('./mixed.js'); }\n", encoding='utf-8')
    (root / 'dynamic-expression.js').write_text("export async function load(name){ return import('./' + name + '.js'); }\n", encoding='utf-8')
    (root / 'commonjs.js').write_text("const helper = require('./mixed.js'); module.exports = function run(value){ return helper.safe(value); };\n", encoding='utf-8')
    (root / 'annotated-dynamic.js').write_text("// @venom: browser\nexport async function load(){ return import('./mixed.js'); }\n", encoding='utf-8')
    run = subprocess.run([str(venom), 'plan', str(root), '--format', 'json'], text=True, capture_output=True)
    if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
    report = json.loads(run.stdout)
    files = {item['file']: item for item in report['recommendations']}
    functions = {name: {fn['name']: fn for fn in item['functions']} for name, item in files.items()}

    for name in ('browser.js', 'middle.js', 'entry.js'):
        if files[name]['runtime'] != 'browser': raise SystemExit((name, files[name]))
    if not any('imports browser-bound symbol: wrapped from middle.js' in reason for reason in functions['entry.js']['entry']['reasons']):
        raise SystemExit(functions['entry.js']['entry'])

    if files['safe-entry.js']['runtime'] != 'protected': raise SystemExit(files['safe-entry.js'])
    if not any('resolved protected import: safe from mixed.js' in reason for reason in functions['safe-entry.js']['run']['reasons']):
        raise SystemExit(functions['safe-entry.js']['run'])
    if files['browser-entry.js']['runtime'] != 'browser': raise SystemExit(files['browser-entry.js'])
    if not any('imports browser-bound symbol: browserOnly from mixed.js' in reason for reason in functions['browser-entry.js']['run']['reasons']):
        raise SystemExit(functions['browser-entry.js']['run'])
    if files['namespace-entry.js']['runtime'] != 'browser': raise SystemExit(files['namespace-entry.js'])
    if files['side-effect-entry.js']['runtime'] != 'browser': raise SystemExit(files['side-effect-entry.js'])

    for name in ('cycle-a.js', 'cycle-b.js'):
        if files[name]['runtime'] != 'protected': raise SystemExit((name, files[name]))
        if not any('module dependency cycle validated' in reason for reason in files[name]['reasons']): raise SystemExit(files[name])

    if files['default-safe-entry.js']['runtime'] != 'protected': raise SystemExit(files['default-safe-entry.js'])
    if files['default-browser-entry.js']['runtime'] != 'browser': raise SystemExit(files['default-browser-entry.js'])
    if files['default-anonymous.js']['runtime'] != 'manual-review': raise SystemExit(files['default-anonymous.js'])
    if files['local-alias-entry.js']['runtime'] != 'protected': raise SystemExit(files['local-alias-entry.js'])
    if files['barrel-safe-entry.js']['runtime'] != 'protected': raise SystemExit(files['barrel-safe-entry.js'])
    if files['barrel-browser-entry.js']['runtime'] != 'browser': raise SystemExit(files['barrel-browser-entry.js'])
    if files['default-barrel-entry.js']['runtime'] != 'protected': raise SystemExit(files['default-barrel-entry.js'])
    if files['star-safe-entry.js']['runtime'] != 'protected': raise SystemExit(files['star-safe-entry.js'])
    if files['star-browser-entry.js']['runtime'] != 'browser': raise SystemExit(files['star-browser-entry.js'])
    if not any('resolved re-export: calculate <- safe from mixed.js' in reason for reason in files['barrel.js']['reasons']):
        raise SystemExit(files['barrel.js'])
    if not any('resolved export-star dependency: mixed.js' in reason for reason in files['star-barrel.js']['reasons']):
        raise SystemExit(files['star-barrel.js'])
    if files['external.js']['runtime'] != 'manual-review': raise SystemExit(files['external.js'])
    if files['dynamic-literal.js']['runtime'] != 'manual-review': raise SystemExit(files['dynamic-literal.js'])
    if not any('literal dynamic import: ./mixed.js' in reason for reason in files['dynamic-literal.js']['reasons']): raise SystemExit(files['dynamic-literal.js'])
    if files['dynamic-expression.js']['runtime'] != 'manual-review': raise SystemExit(files['dynamic-expression.js'])
    if not any('non-literal dynamic import' in reason for reason in files['dynamic-expression.js']['reasons']): raise SystemExit(files['dynamic-expression.js'])
    if files['commonjs.js']['runtime'] != 'manual-review': raise SystemExit(files['commonjs.js'])
    if not any('CommonJS require: ./mixed.js' in reason for reason in files['commonjs.js']['reasons']): raise SystemExit(files['commonjs.js'])
    if not any('CommonJS export assignment' in reason for reason in files['commonjs.js']['reasons']): raise SystemExit(files['commonjs.js'])
    if files['annotated-dynamic.js']['runtime'] != 'browser': raise SystemExit(files['annotated-dynamic.js'])
print('JavaScript AST module closure smoke: PASS')
