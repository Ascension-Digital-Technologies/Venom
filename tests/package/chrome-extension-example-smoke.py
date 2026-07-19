#!/usr/bin/env python3
from pathlib import Path
import json

root = Path(__file__).resolve().parents[2]
example = root / 'examples' / 'chrome-extension'
manifest = json.loads((example / 'manifest.json').read_text(encoding='utf-8'))
assert manifest['manifest_version'] == 3
assert manifest['action']['default_popup'] == 'popup.html'
config = (example / 'venom.toml').read_text(encoding='utf-8')
assert 'target = "chrome-extension"' in config
assert (root / 'scripts' / 'windows' / 'build-and-launch-chrome-extension.bat').is_file()
source = (root / 'src' / 'compiler' / 'pipeline' / 'chrome_extension.cpp').read_text(encoding='utf-8')
assert "wasm-unsafe-eval" in source
assert 'Manifest V3' in source
print('Chrome extension example smoke: PASS')
