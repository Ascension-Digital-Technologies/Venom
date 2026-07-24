#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
launcher = (root / 'tools' / 'launch_example.py').read_text(encoding='utf-8')
if "dist=site/'dist-venom'" not in launcher:
    raise SystemExit('shared example launcher does not emit examples/<name>/dist-venom')
if "root/f'dist-{name}-{profile}'" in launcher:
    raise SystemExit('shared example launcher still emits repository-root distributions')
package_script = (root / 'scripts' / 'windows' / 'package-chrome-extension.bat').read_text(encoding='utf-8')
if r'examples\chrome-extension\dist-venom' not in package_script:
    raise SystemExit('Chrome packaging default does not use the example-local distribution')
for config in (root / 'examples').glob('*/vite.config.js'):
    text = config.read_text(encoding='utf-8')
    if '@venom/vite' in text and 'outDir:' in text and 'dist-venom' not in text:
        raise SystemExit(f'Venom Vite example does not use example-local dist-venom: {config}')
print('example output layout smoke: PASS')
