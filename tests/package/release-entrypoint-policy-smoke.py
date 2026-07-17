#!/usr/bin/env python3
"""Enforce signed stable release entrypoints and current certification coverage."""
from __future__ import annotations
import json
from pathlib import Path
root=Path(__file__).resolve().parents[2]
errors=[]
ps=(root/'scripts/windows/release.ps1').read_text(encoding='utf-8')
sh=(root/'scripts/linux/release.sh').read_text(encoding='utf-8')
workflow=(root/'.github/workflows/certification.yml').read_text(encoding='utf-8')
examples=json.loads((root/'contracts/examples.json').read_text(encoding='utf-8'))
browser=json.loads((root/'contracts/browser-certification.json').read_text(encoding='utf-8'))
for name,text in [('scripts/windows/release.ps1',ps),('scripts/linux/release.sh',sh)]:
    for token in ('--sign','ed25519','--private-key','--public-key','--release-channel','stable','final_release_gate.py'):
        if token not in text: errors.append(f'{name}: missing stable signed-release token {token!r}')
    if '--sign none' in text: errors.append(f'{name}: unsigned stable packaging is forbidden')
    for env in ('VENOM_RELEASE_PRIVATE_KEY_FILE','VENOM_RELEASE_PUBLIC_KEY_FILE'):
        if env not in text: errors.append(f'{name}: missing protected key-file environment variable {env}')
for token in ('linux-x64','windows-x64','macos-arm64','chromium','firefox','webkit','aggregate_certification.py'):
    if token not in workflow: errors.append(f'certification workflow missing {token!r}')
example_items=examples.get('examples',[])
browser_items=browser.get('examples',[])
ids=lambda items:{str(x.get('id')) for x in items}
if len(example_items)!=8: errors.append(f'expected 8 release examples, found {len(example_items)}')
if ids(example_items)!=ids(browser_items): errors.append('browser certification example inventory differs from authoritative example contract')
if errors:
    print('release entrypoint policy smoke: FAIL')
    for error in errors: print(f'  - {error}')
    raise SystemExit(1)
print(f'release entrypoint policy smoke: PASS ({len(example_items)} examples, signed stable entrypoints)')
