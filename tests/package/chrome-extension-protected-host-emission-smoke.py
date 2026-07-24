#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
cpp = (root / 'src/pipeline/chrome_extension.cpp').read_text(encoding='utf-8')
assert 'has_protected_file_directive' in cpp
assert 'runtime_host == RuntimeHost::OffscreenDocument' in cpp
assert 'engine_host_html(relocate_engine_asset_urls(protected_shell_html), analysis.runtime_host, false)' in cpp
assert 'venom-extension-host.js' in cpp
example = root / 'examples/chrome-extension'
protected = (example / 'protected/velocity-engine.js').read_text()
assert protected.startswith('// @venom: protected module')
assert 'export function analyzeVelocity' in protected
host = (example / 'engine-host.js').read_text()
assert host.startswith('// @venom: browser')
assert "import { analyzeVelocity }" in host  # compile-time graph facade only
print('Chrome extension protected host emission smoke: PASS')
