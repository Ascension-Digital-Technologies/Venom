#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys, tempfile
root = Path(__file__).resolve().parents[2]
venom = Path(sys.argv[1]).resolve()
site = root/'tests/fixtures/sites/protected-contract-function'
with tempfile.TemporaryDirectory(prefix='venom-runtime-client-') as td:
    base = Path(td); out = base/'dist'
    run = subprocess.run([str(venom),'build',str(site),'--out',str(out),'--profile','prod','--seed','2000001','--cache-dir',str(base/'cache')], text=True, capture_output=True)
    if run.returncode: raise SystemExit(run.stdout + run.stderr)
    private = base/'.venom'/'dist'/'api'
    js = (private/'venom-client.js').read_text(encoding='utf-8')
    dts = (private/'venom-client.d.ts').read_text(encoding='utf-8')
    assert "from '@venom/runtime'" in js
    assert 'export function score(input, options)' in js
    assert 'callProtected("score", input, options)' in js
    assert "import type { VenomCallOptions } from '@venom/runtime';" in dts
    assert 'export function score(input:' in dts
    assert (out/'assets/app/venom-client.js').is_file()
    assert (out/'assets/app/venom-client.d.ts').is_file()
print('generated runtime client smoke: PASS')
