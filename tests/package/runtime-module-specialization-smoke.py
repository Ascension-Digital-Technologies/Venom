#!/usr/bin/env python3
import json, pathlib, subprocess, sys, tempfile
from _quickjs_artifact import require_current_artifact

if len(sys.argv) != 2:
    raise SystemExit('usage: runtime-module-specialization-smoke.py <venom>')
venom = pathlib.Path(sys.argv[1]).resolve()
root = pathlib.Path(__file__).resolve().parents[2]
require_current_artifact(root)

def build(site, out):
    subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--format', 'json'], check=True, stdout=subprocess.PIPE, text=True)
    meta = json.loads((out / 'assets' / 'app' / 'build.json').read_text())
    runtime = next((out / 'assets' / 'runtime').glob('r.*.js'))
    return meta, runtime.read_text(), runtime.stat().st_size

with tempfile.TemporaryDirectory(prefix='venom-runtime-modules-') as td:
    td = pathlib.Path(td)
    static_meta, static_js, static_size = build(root / 'tests' / 'fixtures' / 'sites' / 'no-script-site', td / 'static')
    browser_meta, browser_js, browser_size = build(root / 'examples' / 'browser-compat-site', td / 'browser')

    static_modules = set(static_meta['runtime_modules'])
    browser_modules = set(browser_meta['runtime_modules'])
    required_core = {'core', 'route', 'dom', 'quickjs'}
    if not required_core.issubset(static_modules):
        raise SystemExit('static runtime omitted a required core module')
    if {'network', 'timers', 'events'} & static_modules:
        raise SystemExit('static runtime retained unused optional modules')
    if not {'network', 'timers', 'events'}.issubset(browser_modules):
        raise SystemExit('interactive runtime did not include required optional modules')
    if "V-MOD-NET" not in static_js or "V-MOD-TIMER" not in static_js:
        raise SystemExit('static runtime did not contain compact fail-closed module stubs')
    if "Venom concurrent fetch limit exceeded" in static_js:
        raise SystemExit('static runtime retained the full network implementation')
    if "Venom per-route event dispatch budget exceeded" in static_js:
        raise SystemExit('static runtime retained the full event implementation')
    if "Venom concurrent fetch limit exceeded" not in browser_js:
        raise SystemExit('interactive runtime lost the network implementation')
    if static_size >= browser_size:
        raise SystemExit(f'static runtime was not smaller: {static_size} >= {browser_size}')

print('runtime module specialization: PASS')
