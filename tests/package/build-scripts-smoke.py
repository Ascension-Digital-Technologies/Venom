#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1])
required = [
    'scripts/build.sh',
    'scripts/build.ps1',
    'scripts/build.bat',
    'scripts/check-release-metadata.bat',
    'scripts/build-site.sh',
    'scripts/build-site.ps1',
    'scripts/build-site.bat',
    'scripts/serve-site.sh',
    'scripts/serve-site.ps1',
    'scripts/serve-site.bat',
    'scripts/package-release.sh',
    'scripts/package-release.ps1',
    'scripts/package-release.bat',
    'scripts/verify-release.sh',
    'scripts/verify-release.ps1',
    'scripts/verify-release.bat',
]
missing = [p for p in required if not (root / p).is_file()]
if missing:
    raise SystemExit('missing build helper(s): ' + ', '.join(missing))
removed = [
    'scripts/build-basic-site.sh', 'scripts/build-basic-site.ps1', 'scripts/build-basic-site.bat',
    'scripts/build-hardened-site.sh', 'scripts/build-hardened-site.ps1', 'scripts/build-hardened-site.bat',
    'scripts/serve-basic-site.sh', 'scripts/serve-basic-site.ps1', 'scripts/serve-basic-site.bat',
    'build-basic-site.bat', 'build-hardened-site.bat', 'serve-basic-site.bat',
]
stale = [p for p in removed if (root / p).exists()]
if stale:
    raise SystemExit('stale pre-production helper(s) still present: ' + ', '.join(stale))
for rel in required:
    text = (root / rel).read_text(errors='ignore')
    if rel.startswith('scripts/build-site'):
        for needle in ('verify-runtime', '--require-real-engine', 'assets', 'app'):
            if needle not in text:
                raise SystemExit(f'{rel} must build and verify the production app layout; missing {needle!r}')
    if 'package-release' in rel and 'package_release.py' not in text:
        raise SystemExit(f'{rel} does not invoke tools/package_release.py')
    if 'verify-release' in rel and 'verify_release.py' not in text:
        raise SystemExit(f'{rel} does not invoke tools/verify_release.py')
print('production script smoke: PASS')
