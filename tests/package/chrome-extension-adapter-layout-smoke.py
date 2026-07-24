#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
cpp=(root/'src/pipeline/chrome_extension.cpp').read_text()
assert 'stable_chrome_adapter' in cpp
assert 'standard_shell.count(file.relative)' in cpp
checker=(root/'tools/verify_chrome_extension_store.py').read_text()
assert 'Root-level hardened adapters' in checker
build=(root/'src/pipeline/build.cpp').read_text()
assert 'cross-world mismatch warnings' in build
print('Chrome extension adapter layout smoke: PASS')
