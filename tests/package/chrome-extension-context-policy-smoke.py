#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
hpp = (root / 'include/chrome_extension.hpp').read_text()
cpp = (root / 'src/pipeline/chrome_extension.cpp').read_text()
doc = (root / 'docs/guides/chrome-extensions.md').read_text()
for token in ('RuntimeHost', 'ContextPolicy', 'compatibility_summary', 'background_is_module'):
    assert token in hpp, token
for token in ('offscreen-document', 'ContentScriptIsolated', 'ContentScriptMain', 'dynamic import could not be resolved statically', 'use a visible RPC adapter'):
    assert token in cpp, token
assert 'Context-aware compilation' in doc
assert 'JavaScript imports inherit the world' in doc
print('Chrome extension context policy smoke: PASS')
