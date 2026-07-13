from __future__ import annotations
import subprocess, sys, tempfile
from pathlib import Path
root = Path(__file__).resolve().parents[2]
venom = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else None
runtime_source = (root / 'src/compiler/runtime_js.cpp').read_text(encoding='utf-8')
assert "protected package decoder is unavailable" in runtime_source
assert "js.erase(begin, end - begin)" in runtime_source
if venom:
    with tempfile.TemporaryDirectory(prefix='venom-wasm-decoder-') as temp:
        out = Path(temp) / 'dist'
        subprocess.run([str(venom), 'build', str(root / 'examples/protected-chess'), '--out', str(out)], check=True, cwd=root, stdout=subprocess.DEVNULL)
        forbidden = (b'VAEAD001', b'VSEAL001', b'VSODIUM1', b'deriveAeadStreamSeed', b'xorSealStream', b'decompressRleV1')
        for path in (out / 'assets').rglob('*.js'):
            data = path.read_bytes()
            for marker in forbidden:
                assert marker not in data, f'{marker!r} leaked into {path.name}'
        subprocess.run([sys.executable, str(root / 'scripts/check-production-leaks.py'), str(out)], check=True, cwd=root, stdout=subprocess.DEVNULL)
print('WASM-owned package decoder smoke: PASS')
