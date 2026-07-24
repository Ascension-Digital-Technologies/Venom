from __future__ import annotations
import re, subprocess, sys, tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
venom = Path(sys.argv[1])
with tempfile.TemporaryDirectory(prefix='venom-release-leak-') as temp:
    out = Path(temp) / 'dist'
    subprocess.run([str(venom), 'build', str(root/'examples/protected-chess'), '--out', str(out), '--profile', 'prod', '--hashed'], check=True, stdout=subprocess.DEVNULL)
    assert not (out/'build/reports').exists(), 'internal reports shipped in hardened dist'
    files = [p for p in out.rglob('*') if p.is_file()]
    blob = b'\n'.join(p.read_bytes() for p in files)
    forbidden = [
        b'bridge-rewrite-plan', b'function-extraction-plan', b'runtime-bridge-contract',
        b'js/ai-engine.js::', b'"source_root"', b'"config_file"',
        b'venom_tjs_context_alloc', b'venom_tjs_bridge_invoke'
    ]
    for marker in forbidden:
        assert marker not in blob, f'forbidden release marker leaked: {marker!r}'
print('release artifact leak smoke: PASS')
