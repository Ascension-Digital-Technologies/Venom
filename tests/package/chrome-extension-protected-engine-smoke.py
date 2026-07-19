#!/usr/bin/env python3
from pathlib import Path
import json
root = Path(__file__).resolve().parents[2]
example = root / 'examples/chrome-extension'
manifest = json.loads((example / 'manifest.json').read_text())
assert manifest.get('manifest_version') == 3
assert manifest.get('background', {}).get('service_worker') == 'background.js'
assert 'offscreen' in manifest.get('permissions', [])
assert (example / 'engine.html').is_file()
assert (example / 'engine-host.js').is_file()
assert (example / 'background.js').is_file()
protected = (example / 'protected/velocity-engine.js').read_text()
assert protected.startswith('// @venom: protected module')
assert 'export function analyzeVelocity' in protected
assert 'VelocityChessEngine' in protected and 'analyzeFen' in protected
assert not (example / 'engine-bundle.js').exists()
assert not (example / 'engine').exists()
content = (example / 'content-script.js').read_text()
assert "target: 'velocity-background'" in content
assert 'VelocityChessEngine' not in content
popup = (example / 'popup.js').read_text()
assert 'engine-bundle.js' not in popup
emitter = (root / 'src/pipeline/chrome_extension.cpp').read_text()
assert 'relative == "protected/velocity-engine.js"' in emitter
print('Chrome extension protected engine smoke: PASS')
