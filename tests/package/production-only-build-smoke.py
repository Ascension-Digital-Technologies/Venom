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
    for folder in ['app', 'style', 'loader', 'runtime', 'workers']:
        if not (assets / folder).is_dir():
            raise SystemExit(f'production dist missing assets/{folder}/')
    if (out / 'sw.js').exists() or (out / 'favicon.ico').exists():
        raise SystemExit('production dist must not emit preview sw.js/favicon.ico')
    package_files = list((assets / 'app').glob('app.*.vbc'))
    if len(package_files) != 1:
        raise SystemExit(f'expected one hashed app.<hash>.vbc, got {[p.name for p in package_files]}')
    if (assets / 'app.vbc').exists() or (assets / 'app' / 'app.vbc').exists() or (assets / 'asset-manifest.txt').exists():
        raise SystemExit('production dist must not emit unhashed app.vbc or external debug asset manifest')
    required = {
        'loader': 'loader/loader.*.js',
        'runtime js bridge': 'runtime/r.*.js',
        'quickjs engine': 'runtime/engine.*.js',
        'quickjs wasm': 'runtime/runtime.*.wasm',
        'wasm runtime': 'runtime/rw.*.wasm',
        'worker': 'workers/worker.*.js',
        'css': 'style/s.*.css',
    }
    for label, pattern in required.items():
        if not list(assets.glob(pattern)):
            raise SystemExit(f'production dist missing {label}: {pattern}')
    loader = next((assets / 'loader').glob('loader.*.js')).read_text(errors='ignore')
    if "assetBaseUrl: new URL('../', import.meta.url).href" not in loader:
        raise SystemExit('nested production loader must resolve assetBaseUrl to assets/ root')
    if "new URL('../runtime/" not in loader or "new URL('../app/app." not in loader or "new URL('../style/s." not in loader:
        raise SystemExit('nested production loader must use parent-relative runtime/package URLs')
    runtime_text = next((assets / 'runtime').glob('r.*.js')).read_text(errors='ignore')
    engine_text = next((assets / 'runtime').glob('engine.*.js')).read_text(errors='ignore')
    if 'new Function(...names, wrapped)' in runtime_text or 'new Function(...names, wrapped)' in engine_text:
        raise SystemExit('protected production runtime must not ship host Function-constructor source fallback')
    if 'host JavaScript fallback is unavailable in protected releases' not in runtime_text:
        raise SystemExit('protected runtime fallback-denial marker missing')
    run([str(venom), 'inspect', str(package_files[0])])


normal = out_root / 'normal'
legacy = out_root / 'legacy-flags'
run([str(venom), 'build', str(site), '--out', str(normal), '--vendor-cache', str(remote_cache), '--offline'])
assert_production_dist(normal)
run([str(venom), 'build', str(site), '--out', str(legacy), '--profile', 'debug', '--runtime', 'js', '--quickjs-backend', 'scaffold', '--allow-host-fallback', '--vendor-cache', str(remote_cache), '--offline'])
assert_production_dist(legacy)
print('production-only build smoke: PASS')
