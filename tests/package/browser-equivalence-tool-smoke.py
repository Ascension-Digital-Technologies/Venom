#!/usr/bin/env python3
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
tool = root / 'tools/browser_equivalence.py'
runner = root / 'tests/runtime/browser-equivalence-runner.mjs'
assert tool.is_file()
assert runner.is_file()
runner_text = runner.read_text(encoding='utf-8')
for token in ('source_expected', 'dist_expected', 'equivalent', 'post_action_checks', 'source_console_errors', 'dist_console_errors'):
    assert token in runner_text, token

with tempfile.TemporaryDirectory() as td:
    base = Path(td)
    source = base / 'source'; dist = base / 'dist'
    source.mkdir(); dist.mkdir()
    (source / 'index.html').write_text('<!doctype html><p id="status">ready</p>', encoding='utf-8')
    (dist / 'index.html').write_text('<!doctype html><p id="status">ready</p>', encoding='utf-8')
    manifest = source / 'venom.browser.json'
    manifest.write_text(json.dumps({
        'schema_version': 1,
        'id': 'equivalence-smoke',
        'scenarios': [{'id': 'main', 'path': '/index.html', 'checks': [{'id': 'status', 'type': 'text', 'selector': '#status', 'equals': 'ready'}]}],
    }), encoding='utf-8')
    payload = {
        'schema_version': 1,
        'browser': 'chromium',
        'fixture': 'equivalence-smoke',
        'passed': True,
        'scenarios': [{
            'id': 'main', 'passed': True,
            'comparisons': [{'id': 'status', 'source': 'ready', 'dist': 'ready', 'source_expected': True, 'dist_expected': True, 'equivalent': True}],
        }],
        'source_console_errors': [], 'dist_console_errors': [], 'source_page_errors': [], 'dist_page_errors': [],
    }
    if os.name == 'nt':
        fake_node = base / 'node.bat'
        escaped = json.dumps(payload).replace('"', '\\"')
        fake_node.write_text(f'@echo off\necho {escaped}\nexit /b 0\n', encoding='utf-8')
    else:
        fake_node = base / 'node'
        fake_node.write_text('#!/bin/sh\nprintf \'%s\\n\' ' + repr(json.dumps(payload)) + '\n', encoding='utf-8')
        fake_node.chmod(0o755)
    report = base / 'report.json'
    proc = subprocess.run([
        sys.executable, str(tool), str(source), str(dist), '--manifest', str(manifest),
        '--node', str(fake_node), '--json-out', str(report), '--format', 'json',
    ], text=True, capture_output=True)
    if proc.returncode != 0:
        raise SystemExit(proc.stdout + proc.stderr)
    data = json.loads(report.read_text(encoding='utf-8'))
    assert data['passed'] is True
    assert data['fixture_id'] == 'equivalence-smoke'
    assert data['source_sha256'] and data['dist_sha256'] and data['manifest_sha256']
    assert data['results'][0]['scenarios'][0]['comparisons'][0]['equivalent'] is True

print('browser equivalence tool smoke: PASS')
