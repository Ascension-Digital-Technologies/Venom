#!/usr/bin/env python3
import json, pathlib, subprocess, sys, tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: dom-compatibility-modules-smoke.py <venom>')
venom = pathlib.Path(sys.argv[1]).resolve()
root = pathlib.Path(__file__).resolve().parents[2]
site = root / 'examples' / 'dom-compat-site'

analysis = subprocess.run([str(venom), 'analyze', str(site), '--format', 'json'], check=True, stdout=subprocess.PIPE, text=True)
graph = json.loads(analysis.stdout)
for name in ['MutationObserver', 'FormData', 'requestAnimationFrame', 'addEventListener', 'input.checked', 'input.value', 'event.preventDefault']:
    if graph.get('browser_apis', {}).get(name, 0) < 1:
        raise SystemExit(f'capability graph missed {name}')

with tempfile.TemporaryDirectory(prefix='venom-dom-compat-') as td:
    out = pathlib.Path(td) / 'dist'
    subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--format', 'json'], check=True, stdout=subprocess.PIPE, text=True)
    meta = json.loads((out / 'assets' / 'app' / 'build.json').read_text())
    modules = set(meta.get('runtime_modules', []))
    required = {'core', 'route', 'dom', 'turbojs', 'events', 'forms', 'observers', 'animation'}
    missing = required - modules
    if missing:
        raise SystemExit(f'DOM compatibility fixture omitted modules: {sorted(missing)}')
    runtime = next((out / 'assets' / 'runtime').glob('r.*.js')).read_text()
    for marker in ['MutationObserver', 'FormData', 'SubmitEvent', 'requestAnimationFrame', '__venomRuntimeModuleEnabled']:
        if marker not in runtime:
            raise SystemExit(f'protected runtime omitted DOM compatibility binding: {marker}')

print('DOM compatibility modules: PASS')
