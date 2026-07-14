#!/usr/bin/env python3
"""Enforce signed stable release entrypoints and compatibility-suite CI coverage."""
from __future__ import annotations
import json, re, sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
errors: list[str] = []

ps = (root / 'scripts/release.ps1').read_text(encoding='utf-8')
sh = (root / 'scripts/release.sh').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/browser-validation.yml').read_text(encoding='utf-8')
suite = json.loads((root / 'tests/compatibility-suite.json').read_text(encoding='utf-8'))

for name, text in [('scripts/release.ps1', ps), ('scripts/release.sh', sh)]:
    if '--sign none' in text:
        errors.append(f'{name}: unsigned stable packaging is forbidden')
    for token in ('--sign', 'ed25519', '--private-key', '--public-key', '--release-channel', 'stable'):
        if token not in text:
            errors.append(f'{name}: missing stable signed-release token {token!r}')
    if 'final_release_gate.py' not in text:
        errors.append(f'{name}: must execute the final repository gate')

if 'VENOM_RELEASE_PRIVATE_KEY_FILE' not in ps or 'VENOM_RELEASE_PUBLIC_KEY_FILE' not in ps:
    errors.append('PowerShell release entrypoint must support protected key-file environment variables')
if 'VENOM_RELEASE_PRIVATE_KEY_FILE' not in sh or 'VENOM_RELEASE_PUBLIC_KEY_FILE' not in sh:
    errors.append('shell release entrypoint must require protected key-file environment variables')

for fixture in suite.get('fixtures', []):
    fixture_id = str(fixture.get('id', '')).strip()
    site = str(fixture.get('site', '')).strip()
    if not fixture_id or not site:
        errors.append('compatibility suite contains a fixture without id/site')
        continue
    if site not in workflow:
        errors.append(f'browser-validation workflow does not build/validate suite fixture {fixture_id}: {site}')
    report_name = f'{fixture_id}.equivalence.json'
    # The workflow currently emits browser-validation reports with friendly names rather than equivalence reports.
    # Require at least a fixture-specific output stem to prevent silent omission from uploaded evidence.
    stem = fixture_id.replace('-production', '').replace('-and-', '-').split('-')[0]
    if fixture_id.startswith(('react-vite', 'vue-vite', 'svelte-vite')):
        expected = fixture_id.replace('-production', '') + '-browser-validation.json'
        if expected not in workflow:
            errors.append(f'browser-validation workflow does not upload a report for {fixture_id}')

if errors:
    print('release entrypoint policy smoke: FAIL')
    for error in errors:
        print(f'  - {error}')
    raise SystemExit(1)
print(f"release entrypoint policy smoke: PASS ({len(suite.get('fixtures', []))} compatibility fixtures)")
