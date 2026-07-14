from __future__ import annotations
import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
venom = Path(sys.argv[1])
site = root / 'tests/fixtures/sites/protected-module'

with tempfile.TemporaryDirectory(prefix='venom-protected-module-') as temp:
    temp_root = Path(temp)
    out = temp_root / 'dist'
    subprocess.run([
        str(venom), 'build', str(site), '--out', str(out),
        '--profile', 'dev', '--seed', '1400001'
    ], check=True, stdout=subprocess.DEVNULL)

    report = json.loads((out / 'build/reports/bridge-rewrite-plan.json').read_text())
    funcs = {item['function']: item for item in report['functions']}
    assert funcs['calculate']['status'] == 'extracted'
    assert funcs['calculateAsync']['status'] == 'extracted'
    assert 'score' not in funcs, 'private dependency exports must not become browser-callable bridge exports'

    plan = json.loads((out / 'build/reports/execution-plan.json').read_text())
    reasons = {item['source']: item['reason'] for item in plan['scripts']}
    assert reasons['engine.js'] == 'generated browser facade for isolated protected module graph'
    assert reasons['core.js'] == 'generated browser facade for isolated protected module graph'

    loader = (out / 'assets/loader/loader.js').read_text()
    assert '__venomRegisterProtectedExport("calculate"' in loader
    assert '__venomRegisterProtectedExport("calculateAsync"' in loader
    assert '__venomRegisterProtectedExport("score"' not in loader

    dts = (out / 'assets/app/venom-exports.d.ts').read_text()
    assert 'calculate(input: { value: number; }' in dts
    assert 'calculateAsync(input: { value?: number; }' in dts
    package_bytes = (out / 'assets/app/app.vbc').read_bytes()
    assert b'input.value must be number' in package_bytes
    assert b'output.result must be number' in package_bytes

    invalid = temp_root / 'invalid-contract'
    invalid.mkdir()
    (invalid / 'index.html').write_text('<!doctype html><script type="module" src="main.js"></script>')
    (invalid / 'main.js').write_text('// @venom: protected module\n// @venom: input value:Date\nexport function bad(input){return input;}\n')
    (invalid / 'venom.lock').write_text('VENOM_VENDOR_LOCK_V1\nversion=1\nentry_count=0\nurl\tsha256\tbytes\tintegrity\n')
    failed = subprocess.run([
        str(venom), 'build', str(invalid), '--out', str(temp_root / 'invalid-dist'),
        '--profile', 'dev', '--seed', '1420001'
    ], text=True, capture_output=True)
    assert failed.returncode != 0
    assert 'VENOM-E2302' in (failed.stdout + failed.stderr)

    # Multiple modules intentionally use the same private helper name. A successful build
    # proves that each module is wrapped in an independent lexical scope.

    cycle = temp_root / 'cycle'
    cycle.mkdir()
    (cycle / 'index.html').write_text('<!doctype html><script type="module" src="a.js"></script>')
    (cycle / 'a.js').write_text('// @venom: protected module\nimport { b } from "./b.js";\nexport function a(x){return b(x);}\n')
    (cycle / 'b.js').write_text('// @venom: protected module\nimport { a } from "./a.js";\nexport function b(x){return a(x);}\n')
    (cycle / 'venom.lock').write_text('VENOM_VENDOR_LOCK_V1\nversion=1\nentry_count=0\nurl\tsha256\tbytes\tintegrity\n')
    failed = subprocess.run([
        str(venom), 'build', str(cycle), '--out', str(temp_root / 'cycle-dist'),
        '--profile', 'dev', '--seed', '1400002'
    ], text=True, capture_output=True)
    assert failed.returncode != 0
    assert 'VENOM-E2216' in (failed.stdout + failed.stderr)

    dynamic = temp_root / 'dynamic'
    dynamic.mkdir()
    (dynamic / 'index.html').write_text('<!doctype html><script type="module" src="main.js"></script>')
    (dynamic / 'main.js').write_text('// @venom: protected module\nexport async function load(){return import("./other.js");}\n')
    (dynamic / 'other.js').write_text('export const value=1;\n')
    (dynamic / 'venom.lock').write_text('VENOM_VENDOR_LOCK_V1\nversion=1\nentry_count=0\nurl\tsha256\tbytes\tintegrity\n')
    failed = subprocess.run([
        str(venom), 'build', str(dynamic), '--out', str(temp_root / 'dynamic-dist'),
        '--profile', 'dev', '--seed', '1400003'
    ], text=True, capture_output=True)
    assert failed.returncode != 0
    assert 'VENOM-E2205' in (failed.stdout + failed.stderr)

print('protected module graph smoke: PASS')
