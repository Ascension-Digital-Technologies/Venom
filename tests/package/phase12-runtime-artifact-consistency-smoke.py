#!/usr/bin/env python3
from __future__ import annotations
import hashlib, json, re, subprocess, sys, tempfile
from pathlib import Path

root = Path(sys.argv[1]).resolve()
venom = Path(sys.argv[2]).resolve()
work = Path(tempfile.mkdtemp(prefix='venom-phase12-'))
dist = work / 'dist'
cmd = [str(venom), 'build', str(root/'tests/fixtures/production-site'), '--out', str(dist),
       '--profile', 'prod', '--runtime', 'wasm', '--hashed', '--strict-release',
       '--vendor-cache', str(root/'tests/fixtures/remote-cache'), '--offline']
subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
verify = subprocess.run([str(venom), 'verify-runtime', str(dist), '--target', 'browser', '--require-real-engine'],
                        check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
if 'release_status: PASS' not in verify.stdout:
    raise SystemExit(verify.stdout)
wasm_files = list((dist/'assets'/'runtime').glob('runtime.*.wasm'))
if len(wasm_files) != 1:
    raise SystemExit(f'expected one QuickJS runtime, got {wasm_files}')
wasm = wasm_files[0]
actual = hashlib.sha256(wasm.read_bytes()).hexdigest()
loaders = list((dist/'assets'/'loader').glob('loader.*.js'))
if len(loaders) != 1:
    raise SystemExit('expected exactly one loader')
loader = loaders[0].read_text(encoding='utf-8')
m = re.search(r"expectedQuickJsWasmSha256:\s*'([0-9a-f]{64})'", loader)
if not m or m.group(1) != actual:
    raise SystemExit(f'loader digest mismatch: expected={m.group(1) if m else None} actual={actual}')
# Verify the checked-in embedded blob digest is the same digest emitted to dist.
header = (root/'src/generated/runtime/quickjs_runtime_wasm_blob.hpp').read_text(encoding='utf-8')
h = re.search(r'kQuickJsRuntimeWasmBlobSha256\s*=\s*"([0-9a-f]{64})"', header)
if not h or h.group(1) != actual:
    raise SystemExit(f'embedded/runtime digest mismatch: embedded={h.group(1) if h else None} emitted={actual}')
print('phase12 runtime artifact consistency: PASS')
