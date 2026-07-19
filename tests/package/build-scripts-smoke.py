#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
examples = {
    'build-and-launch-chess',
    'build-and-launch-nova-trade',
    'build-and-launch-bot-detection',
    'build-and-launch-typescript-showcase',
    'build-and-launch-tsx-showcase',
    'build-and-launch-aegis-operations',
    'build-and-launch-javascript-playground',
    'build-and-launch-quickjs-benchmark',
    'build-and-launch-chrome-extension',
}
core = {'build', 'build-emsdk', 'package-release', 'verify-release', 'build-and-launch-website'}
failures = []

platform_sets = {}
for platform, suffix in [('windows', '.bat'), ('linux', '.sh')]:
    folder = root / 'scripts' / platform
    names = {p.stem for p in folder.glob(f'*{suffix}')}
    platform_sets[platform] = names
    missing = sorted((examples | core) - names)
    if missing:
        failures.append(f'{platform} launchers missing: {missing}')
    numbered = sorted(name for name in names if name.startswith('build-and-launch-example'))
    if numbered:
        failures.append(f'{platform} still exposes numbered example launchers: {numbered}')

for name in sorted(examples):
    if name not in platform_sets['windows'] or name not in platform_sets['linux']:
        failures.append(f'launcher parity missing for {name}')

for path in (root / 'scripts' / 'linux').glob('*.sh'):
    first_line = path.read_text(encoding='utf-8').splitlines()[0] if path.read_text(encoding='utf-8') else ''
    if first_line not in {'#!/usr/bin/env bash', '#!/usr/bin/env sh'}:
        failures.append(f'{path}: missing supported shell shebang')

for path in [
    root / 'tools' / 'build_emscripten.py',
    root / 'tools' / 'launch_example.py',
    root / 'tools' / 'package_release.py',
    root / 'tools' / 'verify_release.py',
]:
    if not path.is_file():
        failures.append(f'missing {path}')

if failures:
    print('\n'.join('failure: ' + item for item in failures))
    raise SystemExit(1)
print('build scripts smoke: PASS')
