#!/usr/bin/env python3
from __future__ import annotations
import json, os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'; cache = Path(tmp) / 'cache'
    root.mkdir()
    (root/'index.html').write_text('<!doctype html><script type="module" src="./app.ts"></script>\n', encoding='utf-8')
    (root/'types.ts').write_text('export interface Input { value: number; bonus?: number }\nexport type Result = { result: number };\n', encoding='utf-8')
    (root/'math.ts').write_text('''// @venom: protected module
export function scale(): number { return 2; }
export function multiply(value: number, factor: number): number { return value * factor; }
''', encoding='utf-8')
    (root/'app.ts').write_text('''// @venom: protected module
import type { Input, Result } from './types';
import { scale, multiply } from './math';
interface Config { enabled: boolean }
type Numeric = number;
const config: Config = { enabled: true };
// @venom: input value:number, bonus?:number
// @venom: output result:number
export async function score(input: Input): Promise<Result> {
  const bonus: Numeric = input.bonus ?? 0;
  return { result: config.enabled ? multiply(input.value, scale()) + bonus : 0 } as Result;
}
''', encoding='utf-8')
    env = dict(os.environ); env['SOURCE_DATE_EPOCH'] = '1704067200'
    run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out), '--cache-dir', str(cache), '--verbose'], text=True, capture_output=True, env=env)
    if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
    combined = run.stdout + run.stderr
    if 'typescript=2' not in combined: raise SystemExit('TypeScript module count was not reported in project IR summary')
    ir = json.loads((cache/'project-ir.json').read_text(encoding='utf-8'))
    if ir.get('version') != 12: raise SystemExit(f"expected project IR v12, got {ir.get('version')}")
    modules = {m['source']: m for m in ir['protected_modules']}
    for name in ('app.ts', 'math.ts'):
        if not modules.get(name, {}).get('typescript_transpiled'): raise SystemExit(f'{name} was not transpiled')
    if modules['app.ts']['typescript_erased_annotations'] < 2: raise SystemExit('Type annotations were not erased')
    if modules['app.ts']['typescript_erased_declarations'] < 3: raise SystemExit('Type-only declarations/imports were not erased')
    if not (out/'assets/app/venom-exports.d.ts').exists(): raise SystemExit('typed protected API declaration was not generated')



    # Broader TypeScript compatibility corpus.
    advanced = root/'advanced.ts'
    advanced.write_text("""declare const BUILD_ID: string;
declare global { interface Window { venomReady?: boolean } }
interface Box<T> { value: T; optional?: string }
type Pair<T> = readonly [T, T];
abstract class Base<T> {
  abstract compute(value: T): number;
  readonly label?: string;
}
class Engine<T extends number> extends Base<T> implements Box<T> {
  optional?: string;
  definite!: number;
  constructor(public value: T, private readonly factor: number = 2) { super(); }
  compute(value: T): number { return value * this.factor; }
}
function parse(value: string): number;
function parse(value: number): number;
function parse(value: string | number): number { return Number(value); }
const pair = [1, 2] as const satisfies Pair<number>;
export function run(value: number): number { return new Engine(value).compute(parse(pair[0])); }
""", encoding='utf-8')
    (root/'index.html').write_text('<!doctype html><script type="module" src="./advanced.ts"></script>\n', encoding='utf-8')
    advanced_run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out), '--cache-dir', str(cache)], text=True, capture_output=True, env=env)
    if advanced_run.returncode != 0: raise SystemExit(advanced_run.stdout + advanced_run.stderr)

    enum_source = root/'unsupported-enum.ts'
    enum_source.write_text("""const before = 1;
enum Mode { Fast, Safe }
export const selected = Mode.Fast;
""", encoding='utf-8')
    (root/'index.html').write_text('<!doctype html><script type="module" src="./unsupported-enum.ts"></script>\n', encoding='utf-8')
    enum_failed = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out)], text=True, capture_output=True, env=env)
    enum_text = enum_failed.stdout + enum_failed.stderr
    if enum_failed.returncode == 0 or 'VENOM-E2502' not in enum_text or 'unsupported-enum.ts:2:' not in enum_text:
        raise SystemExit('enum diagnostic should fail with VENOM-E2502 at source line 2')

    (root/'jsx-runtime.js').write_text('''export const Fragment = Symbol("Fragment");
export function createElement(type, props, ...children) { return { type, props: props ?? {}, children }; }
const React = { createElement, Fragment };
export default React;
''', encoding='utf-8')
    jsx = root/'view.tsx'
    jsx.write_text('''import React from "./jsx-runtime.js";
interface Props { title: string; tags?: string[] }
function Card({ title, tags = [] }: Props) {
  return <section className="card"><h1>{title}</h1><>{tags.map(tag => <span>{tag}</span>)}</></section>;
}
const props: Props = { title: "Ready" };
export const view = <Card {...props} tags={["TSX", "Venom"]} />;
''', encoding='utf-8')
    (root/'index.html').write_text('<!doctype html><script type="module" src="./view.tsx"></script>\n', encoding='utf-8')
    jsx_run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out), '--cache-dir', str(cache)], text=True, capture_output=True, env=env)
    if jsx_run.returncode != 0: raise SystemExit(jsx_run.stdout + jsx_run.stderr)
    for leaked in out.rglob('*'):
        if leaked.is_file() and leaked.suffix.lower() in {'.tsx', '.jsx'}:
            raise SystemExit(f'TSX/JSX source leaked into distribution: {leaked}')

    malformed = root/'malformed.tsx'
    malformed.write_text('import React from "./jsx-runtime.js"; const view = <section><span>broken</section>;\n', encoding='utf-8')
    (root/'index.html').write_text('<!doctype html><script type="module" src="./malformed.tsx"></script>\n', encoding='utf-8')
    malformed_run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out)], text=True, capture_output=True, env=env)
    malformed_text = malformed_run.stdout + malformed_run.stderr
    if malformed_run.returncode == 0 or 'VENOM-E2503' not in malformed_text or 'malformed.tsx:1:' not in malformed_text:
        raise SystemExit('malformed JSX should fail with a source-located VENOM-E2503 diagnostic')
print('TypeScript input smoke: PASS')
