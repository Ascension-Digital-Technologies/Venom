#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
m=re.search(r'project\(venom\s+VERSION\s+(\d+\.\d+\.\d+)',cm,re.S)
if not m: raise SystemExit('project version missing')
version=m.group(1)
required={
 'generated version template': 'VENOM_VERSION_STRING "@PROJECT_VERSION@"' in (root/'src/compiler/version.hpp.in').read_text(),
 'CLI uses generated version': 'VENOM_VERSION_STRING' in (root/'src/compiler/cli.cpp').read_text(),
 'remote UA uses generated version': 'VENOM_REMOTE_VENDOR_USER_AGENT' in (root/'src/compiler/remote.cpp').read_text(),
 'README current': (root/'README.md').read_text().startswith('# Venom - Secure Web Runtime'),
 'canonical scripts directory': all((root/'scripts'/name).is_file() for name in ('build.bat','build.ps1','build.sh','build-site.bat','build-site.ps1','build-site.sh','test.bat','test.ps1','test.sh')),
 'clean metadata-only root': not any((root/name).exists() for name in ('build.bat','build-site.bat','test.bat','serve-site.bat','rebuild-runtime.bat','rebuild-site.bat')), 
 'preset configurations': (root/'CMakePresets.json').is_file(),
 'tests isolated from root graph': 'include(cmake/tests.cmake)' in cm and (root/'cmake/tests.cmake').is_file(),
 'metadata command canonicalized': (root/'scripts/check-release-metadata.bat').is_file(),
}
failed=[k for k,v in required.items() if not v]
if failed: raise SystemExit('repository consistency failed: '+', '.join(failed))
# Current source code must not carry stale product-version literals outside historical docs/tests.
for path in list((root/'src').rglob('*')):
    if path.is_file() and path.suffix in {'.c','.cpp','.h','.hpp'}:
        text=path.read_text(errors='ignore')
        for hit in re.findall(r'(?<!ABI )(?<!v)(?:Venom/|venom |Venom )(\d+\.\d+\.\d+)', text):
            if hit != version:
                raise SystemExit(f'stale product version {hit} in {path.relative_to(root)}')
print(f'repository consistency smoke passed: {version}')
