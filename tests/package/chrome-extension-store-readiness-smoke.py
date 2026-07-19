#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
verify=(root/'tools/verify_chrome_extension_store.py').read_text('utf-8')
package=(root/'tools/package_chrome_extension.py').read_text('utf-8')
bat=(root/'scripts/windows/package-chrome-extension.bat').read_text('utf-8')
for token in ('manifest_version','unsafe-eval','web_accessible_resources','high-impact permissions','venom-chrome-store-readiness-v1','JavaScript contains source comments'):
    assert token in verify
for token in ('ZIP_DEFLATED','SHA256SUMS.txt','store-readiness.json','build-report.json'):
    assert token in package
assert 'pause >nul' in bat
print('Chrome extension store readiness smoke: PASS')
