#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
verify = root / 'tools' / 'verify_chrome_extension_store.py'

manifest = {
    'manifest_version': 3,
    'name': 'Runtime URL smoke',
    'version': '1.0.0',
    'background': {'service_worker': 'background.js'},
    'content_security_policy': {
        'extension_pages': "script-src 'self' 'wasm-unsafe-eval'; object-src 'self'"
    },
}

with tempfile.TemporaryDirectory(prefix='venom-runtime-url-') as tmp:
    dist = Path(tmp) / 'dist'
    dist.mkdir()
    (dist / 'manifest.json').write_text(json.dumps(manifest), encoding='utf-8')
    (dist / 'background.js').write_text(
        "fetch(chrome.runtime.getURL('assets/extension/allowed_sites.txt'));",
        encoding='utf-8',
    )
    failed = subprocess.run(
        [sys.executable, str(verify), str(dist)],
        capture_output=True,
        text=True,
    )
    assert failed.returncode == 1, failed.stdout + failed.stderr
    assert 'referenced resource is missing: assets/extension/allowed_sites.txt' in failed.stdout

    target = dist / 'assets' / 'extension' / 'allowed_sites.txt'
    target.parent.mkdir(parents=True)
    target.write_text('https://example.com/*\n', encoding='utf-8')
    passed = subprocess.run(
        [sys.executable, str(verify), str(dist)],
        capture_output=True,
        text=True,
    )
    assert passed.returncode == 0, passed.stdout + passed.stderr

print('Chrome extension runtime URL readiness smoke: PASS')
