#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
extension = root / 'examples' / 'chrome-extension'
protected = extension / 'protected' / 'velocity-engine.js'
assert protected.read_text().startswith('// @venom: protected module')
assert 'export function analyzeVelocity' in protected.read_text()
adapters = [p for p in extension.rglob('*.js') if 'tests' not in p.parts and 'scripts' not in p.parts and p != protected and 'engine' not in p.parts and p.name != 'engine-bundle.js']
assert adapters
for path in adapters:
    text = path.read_text()
    assert text.startswith('// @venom: browser'), f'{path.relative_to(root)} must be a Chrome/DOM adapter'
assert '// @venom: protected module' in protected.read_text()
print(f'Chrome extension runtime split smoke: PASS ({len(adapters)} adapters, 1 protected engine)')
