#!/usr/bin/env python3
import json, pathlib, subprocess, sys, tempfile

exe = pathlib.Path(sys.argv[1])
with tempfile.TemporaryDirectory(prefix='venom-dist-analyzer-') as td:
    root = pathlib.Path(td) / 'dist'
    (root / 'assets' / 'app').mkdir(parents=True)
    (root / 'assets' / 'runtime').mkdir(parents=True)
    (root / 'index.html').write_text('<!doctype html><title>x</title>', encoding='utf-8')
    (root / 'assets' / 'app' / 'app.test.vbc').write_bytes(b'VBC')
    (root / 'assets' / 'runtime' / 'runtime.test.wasm').write_bytes(b'\0asm')
    proc = subprocess.run([str(exe), 'analyze-dist', str(root), '--format', 'json'], text=True, capture_output=True)
    if proc.returncode != 0:
        raise SystemExit(proc.stdout + proc.stderr)
    data = json.loads(proc.stdout)
    assert data['clean'] is True
    assert data['fileCount'] == 3
    assert data['categories']['Protected package'] == 3
    assert data['categories']['WebAssembly'] == 4
    (root / 'assets' / 'loose.json').write_text('{}', encoding='utf-8')
    proc = subprocess.run([str(exe), 'analyze-dist', str(root), '--format', 'json'], text=True, capture_output=True)
    assert proc.returncode != 0
    data = json.loads(proc.stdout)
    assert data['clean'] is False
    assert 'assets/loose.json' in data['looseAssets']
print('dist analyzer smoke: PASS')
