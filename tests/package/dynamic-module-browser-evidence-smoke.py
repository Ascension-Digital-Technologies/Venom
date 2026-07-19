#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[2]
fixture=root/'tests/fixtures/sites/dynamic-bundle-site/venom.browser.json'
assert fixture.is_file(), 'dynamic module browser manifest missing'
for rel in ['.github/workflows/browser-validation.yml','.github/workflows/release.yml']:
    text=(root/rel).read_text(encoding='utf-8')
    required=[
      'tests/fixtures/sites/dynamic-bundle-site',
      'build/dynamic-bundle-dist',
      'dynamic-bundle-browser-validation.json',
      'tests/fixtures/sites/dynamic-bundle-site/venom.browser.json',
    ]
    for marker in required:
        assert marker in text, f'{rel} missing {marker}'
cut=(root/'tools/quickjs_wasm_cutover.py').read_text(encoding='utf-8')
for marker in ["'module_bundle_contract': 'VQJSMB04'", "'literal_dynamic_import': 'true'"]:
    assert marker in cut, f'cutover missing {marker}'
build=(root / 'src/pipeline/build.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_package_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_audit_metadata.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/build_runtime_module_metadata.cpp').read_text(encoding='utf-8')
assert 'protected module graph requires QuickJS/WASM module bundle contract VQJSMB04' in build
print('dynamic module browser evidence smoke: PASS')
