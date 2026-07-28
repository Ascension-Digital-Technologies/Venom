#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys

venom = Path(sys.argv[1]).resolve()
site = Path(sys.argv[2]).resolve()
out_root = Path(sys.argv[3]).resolve()
remote_cache = Path(__file__).resolve().parents[1] / 'fixtures' / 'remote-cache'


def run(cmd):
    subprocess.run(cmd, check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=120)


def assert_production_dist(out: Path):
    run([str(venom), 'verify-runtime', str(out), '--target', 'browser', '--require-real-engine'])
    roots = sorted(p.name for p in out.iterdir() if p.is_file())
    if roots != ['index.html']:
        raise SystemExit(f'production dist root must only contain index.html, got {roots}')
    assets = out / 'assets'
    if not assets.is_dir():
        raise SystemExit('production dist missing assets/')
    expected_folders = {'app', 'javascript', 'wasm'}
    actual_folders = {p.name for p in assets.iterdir() if p.is_dir()}
    if not expected_folders.issubset(actual_folders):
        raise SystemExit(f'production dist missing current asset folders: expected {sorted(expected_folders)}, got {sorted(actual_folders)}')
    if (out / 'sw.js').exists() or (out / 'favicon.ico').exists():
        raise SystemExit('production dist must not emit preview sw.js/favicon.ico')
    package_files = list((assets / 'app').glob('*.vbc'))
    if len(package_files) != 1:
        raise SystemExit(f'expected one hashed VBC package, got {[p.name for p in package_files]}')
    if (assets / 'app.vbc').exists() or (assets / 'asset-manifest.txt').exists():
        raise SystemExit('production dist must not emit unhashed package or external debug asset manifest')
    required = {
        'bootstrap/runtime JavaScript': 'javascript/*.js',
        'runtime WASM': 'wasm/*.wasm',
        'application CSS': 'app/*.css',
    }
    for label, pattern in required.items():
        if not list(assets.glob(pattern)):
            raise SystemExit(f'production dist missing {label}: {pattern}')
    runtime_text = '\n'.join(path.read_text(errors='ignore') for path in (assets / 'javascript').glob('*.js'))
    if 'new Function(...names, wrapped)' in runtime_text:
        raise SystemExit('protected production runtime must not ship host Function-constructor source fallback')
    runtime_generator = (Path(__file__).resolve().parents[2] / 'src' / 'generated' / 'runtime' / 'runtime_js.cpp').read_text(errors='ignore')
    if 'host JavaScript fallback is unavailable in protected releases' not in runtime_generator:
        raise SystemExit('protected runtime fallback-denial marker missing from runtime generator')
    run([str(venom), 'inspect', str(package_files[0])])


normal = out_root / 'normal'
run([str(venom), 'build', str(site), '--out', str(normal), '--vendor-cache', str(remote_cache), '--offline'])
assert_production_dist(normal)
print('production-only build smoke: PASS')
