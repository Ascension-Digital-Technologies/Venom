#!/usr/bin/env python3
import pathlib
import shutil
import subprocess
import sys

venom = pathlib.Path(sys.argv[1])
site = pathlib.Path(sys.argv[2])
out = pathlib.Path(sys.argv[3])

if out.exists():
    shutil.rmtree(out)

subprocess.run([str(venom), 'build', str(site), '--out', str(out), '--profile', 'browser-protect', '--runtime', 'wasm', '--hashed', '--strict-release'], check=True, text=True, capture_output=True)
release = subprocess.run([str(venom), 'release-check', str(out), '--target', 'browser'], check=True, text=True, capture_output=True)
text = release.stdout
for marker in [
    'quickjs_host_js_fallback_allowed: no',
    'quickjs_release_fail_closed: yes',
    'quickjs_runtime_full_upstream_quickjs: yes',
    'quickjs_runtime_finish_blocker: none',
    'release_status: PASS',
]:
    if marker not in text:
        raise SystemExit(f'missing hardened release marker {marker!r}\n{text}')
subprocess.run([str(venom), 'verify-runtime', str(out), '--target', 'browser', '--require-real-engine'], check=True, text=True, capture_output=True)
assets = out / 'assets'
if list(assets.glob('runtime-js-bridge*.js')):
    raise SystemExit('hardened wasm runtime bridge used readable runtime-js-bridge asset stem')
if not list(assets.glob('r*.js')):
    raise SystemExit('hardened wasm runtime bridge did not use compact r*.js asset stem')
js_text = ''
for path in assets.glob('*.js'):
    js_text += path.read_text(encoding='utf-8') + '\n'
for forbidden in ['new Function', 'eval(']:
    if forbidden in js_text:
        raise SystemExit(f'hardened JS asset still contains source execution token: {forbidden}')
if 'host-js-fallback' in js_text:
    raise SystemExit('hardened JS asset still contains host-js-fallback marker')
print('hardened release enforcement smoke passed')
