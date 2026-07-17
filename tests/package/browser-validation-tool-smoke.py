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

FAKE_PLAYWRIGHT = r'''
class _Message:
    type = "log"
    text = ""
class _Response:
    ok = True
    status = 200
class _Page:
    url = "http://127.0.0.1/index.html"
    def on(self, *_): pass
    def goto(self, *_args, **_kwargs): return _Response()
    def wait_for_selector(self, *_args, **_kwargs): pass
    def wait_for_function(self, *_args, **_kwargs): pass
    def locator(self, *_args): return self
    def count(self): return 1
    def text_content(self, selector): return "Protected runtime verified" if selector == "#runtime-state" else "fixture"
    def get_attribute(self, *_args): return None
    def evaluate(self, *_args): return True
    def click(self, *_args): pass
    def fill(self, *_args): pass
    def select_option(self, *_args): pass
class _Context:
    def new_page(self): return _Page()
class _Browser:
    def new_context(self, **_kwargs): return _Context()
    def close(self): pass
class _BrowserType:
    def launch(self, **_kwargs): return _Browser()
class _Playwright:
    chromium = _BrowserType()
    firefox = _BrowserType()
    webkit = _BrowserType()
class _Manager:
    def __enter__(self): return _Playwright()
    def __exit__(self, *_args): pass
def sync_playwright(): return _Manager()
'''

with tempfile.TemporaryDirectory() as td:
    d = Path(td)
    fake_package = d / 'playwright'
    fake_package.mkdir()
    (fake_package / '__init__.py').write_text('', encoding='utf-8')
    (fake_package / 'sync_api.py').write_text(FAKE_PLAYWRIGHT, encoding='utf-8')

    dist = d / 'dist'
    dist.mkdir()
    (dist / 'index.html').write_text('<!doctype html><p id="runtime-state">Protected runtime verified</p>', encoding='utf-8')
    manifest = d / 'venom.browser.json'
    manifest.write_text(json.dumps({
        'schema_version': 2,
        'id': 'smoke',
        'profile': 'prod',
        'evidence_level': 'runtime-qualification',
        'claims': ['protected-export-executes'],
        'scenarios': [{
            'id': 'boot',
            'path': '/',
            'checks': [
                {'id': 'ready', 'type': 'contains-text', 'selector': '#runtime-state', 'contains': 'Protected'},
                {'id': 'export', 'type': 'evaluate', 'expression': 'true'}
            ]
        }]
    }), encoding='utf-8')
    report = d / 'browser.json'
    env = {**os.environ, 'PYTHONPATH': str(d) + os.pathsep + os.environ.get('PYTHONPATH', '')}
    p = subprocess.run([sys.executable, str(tool), str(dist), '--manifest', str(manifest), '--json-out', str(report), '--format', 'json'], text=True, capture_output=True, env=env)
    if p.returncode != 0:
        raise SystemExit(p.stdout + p.stderr)
    data = json.loads(report.read_text(encoding='utf-8'))
    assert data['passed'] is True
    assert data['dist_sha256']
    assert data['results'][0]['browser'] == 'chromium'
    assert data['fixture_id'] == 'smoke'
    assert data['manifest_sha256']
    assert data['fixture_profile'] == 'prod'
    assert data['evidence_level'] == 'runtime-qualification'
    assert data['claims'] == ['protected-export-executes']

    (dist / 'changed.txt').write_text('changed', encoding='utf-8')
    site = d / 'site'; site.mkdir(); (site / 'index.html').write_text('<!doctype html>', encoding='utf-8')
    fake_venom = d / ('venom.bat' if os.name == 'nt' else 'venom')
    if os.name == 'nt':
        fake_venom.write_text('@echo off\nif "%1"=="--version" (echo venom 1.1.0& exit /b 0)\nif "%1"=="compatibility" (echo {"schema_version":1,"compatible":true,"findings":[]}& exit /b 0)\nif "%1"=="verify-runtime" exit /b 0\nexit /b 1\n', encoding='utf-8')
    else:
        fake_venom.write_text('#!/bin/sh\nif [ "$1" = "--version" ]; then echo "venom 1.1.0"; exit 0; fi\nif [ "$1" = "compatibility" ]; then echo \'{"schema_version":1,"compatible":true,"findings":[]}\'; exit 0; fi\nif [ "$1" = "verify-runtime" ]; then exit 0; fi\nexit 1\n', encoding='utf-8')
        fake_venom.chmod(0o755)
    assets = dist / 'assets'; assets.mkdir(); (assets / 'runtime.wasm').write_bytes(b'wasm'); (assets / 'app.vbc').write_bytes(b'vbc')
    (dist / 'assets' / 'app').mkdir(parents=True, exist_ok=True)
    (dist / 'assets' / 'app' / 'build.json').write_text('{"venom_version":"1.1.0","package_sha256":"x"}', encoding='utf-8')
    p = subprocess.run([sys.executable, str(readiness), str(site), '--dist', str(dist), '--venom', str(fake_venom), '--browser-report', str(report), '--format', 'json'], text=True, capture_output=True)
    assert p.returncode == 60
    result = json.loads(p.stdout)
    check = next(c for c in result['checks'] if c['id'] == 'browser.validation')
    assert check['status'] == 'fail'

print('browser validation tool smoke: ok')
