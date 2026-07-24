#!/usr/bin/env python3
from pathlib import Path
import json
root=Path(__file__).resolve().parents[2]
example=root/'examples/chrome-extension'
manifest=json.loads((example/'manifest.json').read_text())
assert 'offscreen' in manifest['permissions']
assert manifest['background']=={'service_worker':'background.js','type':'module'}
assert (example/'engine-bundle.js').is_file()
assert (example/'engine').is_dir()
protected=(example/'protected/velocity-engine.js').read_text()
assert protected.startswith('// @venom: protected module')
assert 'export function analyzeVelocity' in protected
background=(example/'background.js').read_text()
assert "import './venom-extension-rpc.js'" not in background
assert "import './venom-extension-rpc.js'" in (root/'src/pipeline/chrome_extension.cpp').read_text()
assert "VenomExtensionRPC.call('analyzeVelocity'" in background
content=(example/'content-script.js').read_text()
assert "type: 'VELOCITY_ANALYZE'" in content
assert 'VelocityChessEngine' not in content
popup=(example/'popup.js').read_text()
assert 'engine-bundle.js' not in popup
build=(root/'src/pipeline/build.cpp').read_text()
assert 'if (!chrome_extension::is_extension_target(options.project_target))' in build
print('Chrome extension runtime bridge smoke: PASS')
