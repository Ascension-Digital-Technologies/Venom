#!/usr/bin/env python3
from __future__ import annotations
import json, pathlib, subprocess, sys, tempfile
root = pathlib.Path(__file__).resolve().parents[2]
venom = pathlib.Path(sys.argv[1])
site = root / 'tests/fixtures/sites/protected-contract-function'
with tempfile.TemporaryDirectory(prefix='venom-contract-function-') as td:
    temp_root = pathlib.Path(td)
    out = temp_root / 'dist'
    p = subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'prod', '--seed', '1550001', '--cache-dir', str(temp_root / 'cache')], text=True, capture_output=True)
    if p.returncode != 0:
        raise SystemExit(p.stdout + p.stderr)
    private = out.parent / '.venom' / out.name / 'api'
    dts = (private / 'venom-exports.d.ts').read_text(encoding='utf-8')
    contracts = json.loads((private / 'protected-contracts.json').read_text(encoding='utf-8'))
    assert 'score(input: { value: number; bonus?: number; }' in dts, dts
    item = next(x for x in contracts['exports'] if x['name'] == 'score')
    assert item['input'] == [
        {'name':'value','type':'number','optional':False},
        {'name':'bonus','type':'integer','optional':True},
    ], item
    assert item['output'] == [{'name':'result','type':'number','optional':False}], item
    ir_files = list((temp_root / 'cache').rglob('project-ir.json'))
    assert ir_files, 'project IR missing'
    ir = json.loads(ir_files[-1].read_text(encoding='utf-8'))
    assert ir['version'] == 9
    assert any(x['name'] == 'score' for x in ir['protected_contracts']['exports'])
print('protected function contracts smoke: PASS')
