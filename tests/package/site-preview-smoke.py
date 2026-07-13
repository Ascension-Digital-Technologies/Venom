#!/usr/bin/env python3
import pathlib
import subprocess
import sys

if len(sys.argv) != 4:
    print('usage: site-preview-smoke.py <venom-exe> <site-dir> <out-dir>', file=sys.stderr)
    sys.exit(2)

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    import shutil
    shutil.rmtree(out)

remote_cache = pathlib.Path(__file__).resolve().parents[1] / 'fixtures' / 'remote-cache'
cmd = [str(venom), 'build', str(site), '--out', str(out), '--vendor-cache', str(remote_cache), '--offline']
subprocess.run(cmd, check=True)

required_dirs = [
    out / 'assets' / 'app',
    out / 'assets' / 'style',
    out / 'assets' / 'loader',
    out / 'assets' / 'runtime',
    out / 'assets' / 'workers',
]
missing = [str(path) for path in [out / 'index.html', *required_dirs] if not path.exists()]
if missing:
    raise SystemExit('missing production files: ' + ', '.join(missing))
if (out / 'about.html').exists() or (out / 'sw.js').exists() or (out / 'favicon.ico').exists():
    raise SystemExit('production-only build emitted preview route shells or preview stubs')
if len(list((out / 'assets' / 'app').glob('app.*.vbc'))) != 1:
    raise SystemExit('missing hashed production app package under assets/app/')
if len(list((out / 'assets' / 'style').glob('s.*.css'))) != 1:
    raise SystemExit('missing hashed production CSS under assets/style/')
index = (out / 'index.html').read_text(encoding='utf-8')
if 'assets/loader/loader.' not in index or 'assets/style/s.' not in index:
    raise SystemExit('index.html does not point at production nested assets correctly')

print('site preview files OK')
