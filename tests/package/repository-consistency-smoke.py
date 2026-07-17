#!/usr/bin/env python3
from pathlib import Path
import re, sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
cm=(root/'CMakeLists.txt').read_text(encoding='utf-8')
m=re.search(r'project\(venom\s+VERSION\s+(\d+\.\d+\.\d+)',cm,re.S)
if not m: raise SystemExit('project version missing')
version=m.group(1)
expected={'build','build-emsdk','package-release','verify-release','build-and-launch-example1','build-and-launch-example2','build-and-launch-example3','build-and-launch-example4'}
win={p.stem for p in (root/'scripts/windows').glob('*.bat')}
linux={p.stem for p in (root/'scripts/linux').glob('*.sh')}
required={
 'generated version template': 'VENOM_VERSION_STRING "@PROJECT_VERSION@"' in (root/'cmake/templates/version.hpp.in').read_text(),
 'CLI uses generated version': 'VENOM_VERSION_STRING' in (root/'src/compiler/commands/cli.cpp').read_text(),
 'README current': (root/'README.md').read_text().startswith('# Venom Secure Web Runtime'),
 'minimal Windows launchers': win == expected,
 'minimal Linux launchers': linux == expected,
 'no implementation scripts in public folders': not any((root/'scripts/windows').glob('*.ps1')) and not any((root/'scripts/linux').glob('*.py')),
 'canonical Emscripten controller': (root/'tools/build_emscripten.py').is_file(),
 'production leak gate present': (root/'tools/check_production_leaks.py').is_file(),
 'example launcher helper': (root/'tools/build_and_launch_example.py').is_file(),
 'canonical public examples': sorted(p.name for p in (root/'examples').iterdir() if p.is_dir()) == ['bot-detection','nova-trade','protected-chess','typescript-showcase'],
}
failed=[k for k,v in required.items() if not v]
if failed: raise SystemExit('repository consistency failed: '+', '.join(failed))
print(f'repository consistency smoke passed: {version}')
