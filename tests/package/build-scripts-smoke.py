#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
expected={'build','build-emsdk','package-release','verify-release','build-and-launch-example1','build-and-launch-example2','build-and-launch-example3'}
fail=[]
for platform,suffix in [('windows','.bat'),('linux','.sh')]:
    folder=root/'scripts'/platform
    got={p.stem for p in folder.glob(f'*{suffix}')}
    if got!=expected: fail.append(f'{platform} launchers: expected {sorted(expected)}, got {sorted(got)}')
for p in (root/'scripts/linux').glob('*.sh'):
    if not p.read_text().startswith('#!/usr/bin/env bash'): fail.append(f'{p}: missing bash shebang')
for p in [root/'tools/build_emscripten.py',root/'tools/build_and_launch_example.py',root/'tools/package_release.py',root/'tools/verify_release.py']:
    if not p.is_file(): fail.append(f'missing {p}')
if fail:
    print('\n'.join('failure: '+x for x in fail)); raise SystemExit(1)
print('build scripts smoke: PASS')
