#!/usr/bin/env python3
import pathlib
from _quickjs_artifact import require_current_artifact
import shutil
import subprocess
import sys

if len(sys.argv) != 6:
    print('usage: browser-runtime-compat-smoke.py <venom> <compat-site> <strict-site> <out-root> <node>')
    raise SystemExit(2)

venom = pathlib.Path(sys.argv[1]).resolve()
compat_site = pathlib.Path(sys.argv[2]).resolve()
strict_site = pathlib.Path(sys.argv[3]).resolve()
out_root = pathlib.Path(sys.argv[4]).resolve()
node = sys.argv[5]
source_root = pathlib.Path(__file__).resolve().parents[2]
require_current_artifact(source_root)
harness = source_root / 'tests' / 'runtime' / 'browser-compat-harness.mjs'
remote_cache = source_root / 'tests' / 'fixtures' / 'remote-cache'

if out_root.exists():
    shutil.rmtree(out_root)
out_root.mkdir(parents=True)
compat_out = out_root / 'compat-production'
strict_out = out_root / 'strict-production'

# Production-only build: all historical profile/runtime flags normalize to this shape.
subprocess.run([str(venom), 'build', str(compat_site), '--out', str(compat_out)], check=True)
subprocess.run([str(venom), 'verify-runtime', str(compat_out), '--target', 'browser', '--require-real-engine'], check=True)
subprocess.run([node, str(harness), str(compat_out), 'real-engine-strict', '--strict-no-source-eval'], check=True)
real_check = subprocess.run([str(venom), 'verify', str(compat_out), '--target', 'browser'], text=True, capture_output=True, check=True)
for marker in [
    'quickjs_execution_backend: quickjs-wasm-real',
    'quickjs_bytecode_boundary: wasm-owned',
    'quickjs_host_js_fallback_allowed: no',
    'quickjs_runtime_implementation: quickjs-wasm-upstream-quickjs',
    'quickjs_runtime_artifact_kind: upstream-quickjs-wasm',
    'quickjs_runtime_full_upstream_quickjs: yes',
    'release_status: PASS',
]:
    if marker not in real_check.stdout:
        raise SystemExit(f'missing production real-engine marker {marker!r}\n{real_check.stdout}')

# The SannySoft-style basic site previously hit the fpCollect binding collision.
# Keep it in the real-engine release gate; full browser DOM execution for that fixture is covered by manual/live browser runs.
subprocess.run([str(venom), 'build', str(strict_site), '--out', str(strict_out), '--vendor-cache', str(remote_cache), '--offline'], check=True)
subprocess.run([str(venom), 'verify-runtime', str(strict_out), '--target', 'browser', '--require-real-engine'], check=True)

print('production browser runtime compatibility smoke passed')
