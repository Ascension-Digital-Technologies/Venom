#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
for framework in ('react','vue','svelte'):
    fixture = root/'tests/fixtures/frameworks'/framework
    assert (fixture/'package.json').exists()
    files = list((fixture/'src').glob('*'))
    authored = '\n'.join(p.read_text() for p in files)
    assert '@venom: browser' in authored
    assert '@venom: protected' in authored
    assert 'vite build' in (fixture/'package.json').read_text()
print('vite framework fixtures passed')
