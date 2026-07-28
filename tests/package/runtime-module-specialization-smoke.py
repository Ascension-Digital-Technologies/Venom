#!/usr/bin/env python3
from _private_reports import read_json, root_for
import json, pathlib, subprocess, sys, tempfile
from _turbojs_artifact import require_current_artifact

if len(sys.argv) != 2:
    raise SystemExit('usage: runtime-module-specialization-smoke.py <venom>')
venom = pathlib.Path(sys.argv[1]).resolve()
root = pathlib.Path(__file__).resolve().parents[2]
require_current_artifact(root)

def build(site, out):
    subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--format', 'json'], check=True, stdout=subprocess.PIPE, text=True)
    meta = read_json(out)
    javascript_dir = out / 'assets' / 'javascript'
    runtime_assets = sorted(javascript_dir.glob('*.js'))
    if not runtime_assets:
        raise SystemExit('build emitted no JavaScript runtime assets')
    runtime_size = sum(asset.stat().st_size for asset in runtime_assets)
    return meta, runtime_size

with tempfile.TemporaryDirectory(prefix='venom-runtime-modules-') as td:
    td = pathlib.Path(td)
    static_meta, static_size = build(root / 'tests' / 'fixtures' / 'sites' / 'no-script-site', td / 'static')
    browser_meta, browser_size = build(root / 'tests' / 'fixtures' / 'sites' / 'browser-compat-site', td / 'browser')

    static_modules = set(static_meta['runtime_modules'])
    browser_modules = set(browser_meta['runtime_modules'])
    required_core = {'core', 'route', 'dom', 'turbojs'}
    if not required_core.issubset(static_modules):
        raise SystemExit('static runtime omitted a required core module')
    if {'network', 'timers', 'events'} & static_modules:
        raise SystemExit('static runtime retained unused optional modules')
    if not {'network', 'timers', 'events'}.issubset(browser_modules):
        raise SystemExit('interactive runtime did not include required optional modules')
    if static_size >= browser_size:
        raise SystemExit(f'static runtime was not smaller: {static_size} >= {browser_size}')

print('runtime module specialization: PASS')
