#!/usr/bin/env python3
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
tool = root / 'tools' / 'browser_validation.py'
readiness = root / 'tools' / 'production_readiness.py'

with tempfile.TemporaryDirectory() as td:
    d = Path(td)
    dist = d / 'dist'
    dist.mkdir()
    (dist / 'index.html').write_text('<!doctype html><p id="compat-output">fixture</p>', encoding='utf-8')

    if os.name == 'nt':
        fake_node = d / 'node.bat'
        fake_node.write_text('@echo off\necho {"browser":"chromium","passed":true,"checks":[{"id":"fake","passed":true,"detail":"ok"}]}\nexit /b 0\n', encoding='utf-8')
    else:
        fake_node = d / 'node'
        fake_node.write_text('#!/bin/sh\necho \'{"browser":"chromium","passed":true,"checks":[{"id":"fake","passed":true,"detail":"ok"}]}\'\n', encoding='utf-8')
        fake_node.chmod(0o755)

    manifest = d / 'venom.browser.json'
    manifest.write_text('{"schema_version":1,"id":"smoke","profile":"production-bundler-output","evidence_level":"behavioral","claims":["hashed-static-assets"],"scenarios":[]}', encoding='utf-8')
    report = d / 'browser.json'
    p = subprocess.run([sys.executable, str(tool), str(dist), '--node', str(fake_node), '--manifest', str(manifest), '--json-out', str(report), '--format', 'json'], text=True, capture_output=True)
    if p.returncode != 0:
        raise SystemExit(p.stdout + p.stderr)
    data = json.loads(report.read_text(encoding='utf-8'))
    assert data['passed'] is True
    assert data['dist_sha256']
    assert data['results'][0]['browser'] == 'chromium'
    assert data['fixture_id'] == 'smoke'
    assert data['manifest_sha256']
    assert data['fixture_profile'] == 'production-bundler-output'
    assert data['evidence_level'] == 'behavioral'
    assert data['claims'] == ['hashed-static-assets']

    # Mutating the tested dist must invalidate the report binding in readiness.
    (dist / 'changed.txt').write_text('changed', encoding='utf-8')
    site = d / 'site'; site.mkdir(); (site / 'index.html').write_text('<!doctype html>', encoding='utf-8')
    fake_venom = d / ('venom.bat' if os.name == 'nt' else 'venom')
    if os.name == 'nt':
        fake_venom.write_text('@echo off\nif "%1"=="--version" (echo venom 1.6.0& exit /b 0)\nif "%1"=="compatibility" (echo {"schema_version":1,"compatible":true,"findings":[]}& exit /b 0)\nif "%1"=="verify-runtime" exit /b 0\nexit /b 1\n', encoding='utf-8')
    else:
        fake_venom.write_text('#!/bin/sh\nif [ "$1" = "--version" ]; then echo "venom 1.6.0"; exit 0; fi\nif [ "$1" = "compatibility" ]; then echo \'{"schema_version":1,"compatible":true,"findings":[]}\'; exit 0; fi\nif [ "$1" = "verify-runtime" ]; then exit 0; fi\nexit 1\n', encoding='utf-8')
        fake_venom.chmod(0o755)
    assets = dist / 'assets'; assets.mkdir(); (assets / 'runtime.wasm').write_bytes(b'wasm'); (assets / 'app.vbc').write_bytes(b'vbc')
    (dist / 'assets' / 'app').mkdir(parents=True, exist_ok=True)
    (dist / 'assets' / 'app' / 'build.json').write_text('{"venom_version":"1.6.0","package_sha256":"x"}', encoding='utf-8')
    p = subprocess.run([sys.executable, str(readiness), str(site), '--dist', str(dist), '--venom', str(fake_venom), '--browser-report', str(report), '--format', 'json'], text=True, capture_output=True)
    assert p.returncode == 60
    result = json.loads(p.stdout)
    check = next(c for c in result['checks'] if c['id'] == 'browser.validation')
    assert check['status'] == 'fail'

print('browser validation tool smoke: ok')
