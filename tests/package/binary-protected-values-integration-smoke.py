#!/usr/bin/env python3
from __future__ import annotations
import os, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp) / 'site'; out = Path(tmp) / 'dist'; cache = Path(tmp) / 'cache'
    root.mkdir()
    (root/'index.html').write_text('<!doctype html><script type="module" src="./app.js"></script>\n', encoding='utf-8')
    (root/'app.js').write_text('''// @venom: protected module
// @venom: input values:uint16array
// @venom: output values:uint16array
export function increment(input) {
  const output = new Uint16Array(input.values.length);
  for (let i = 0; i < input.values.length; ++i) output[i] = input.values[i] + 1;
  return { values: output };
}
''', encoding='utf-8')
    env = dict(os.environ); env['SOURCE_DATE_EPOCH'] = '1704067200'
    run = subprocess.run([str(venom), 'build', str(root), '--profile', 'prod', '--out', str(out), '--cache-dir', str(cache)], text=True, capture_output=True, env=env)
    if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
    dts = (out/'assets/app/venom-exports.d.ts').read_text(encoding='utf-8')
    if 'values: Uint16Array;' not in dts: raise SystemExit('binary contract did not generate Uint16Array declaration')
    worker = next((out/'assets/workers').glob('worker*.js')).read_text(encoding='utf-8')
    loader = next((out/'assets/loader').glob('loader*.js')).read_text(encoding='utf-8')
    for marker in ('binary-json-v2', 'encodeBridgePayload', 'decodeBridgePayload'):
        if marker not in worker and marker not in loader: raise SystemExit(f'missing binary bridge marker: {marker}')
print('binary protected values integration smoke: PASS')
