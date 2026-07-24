#!/usr/bin/env python3
from pathlib import Path
import subprocess, sys, tempfile
venom=Path(sys.argv[1]).resolve(); repo=Path(__file__).resolve().parents[2]; example=repo/'examples'/'javascript-playground'
with tempfile.TemporaryDirectory(prefix='venom-playground-') as td:
    out=Path(td)/'dist'
    subprocess.run([str(venom),'build',str(example),'--out',str(out),'--profile','prod'],check=True)
    if not any(p.name == '.vbc' or p.suffix == '.vbc' for p in out.rglob('*')): raise SystemExit('missing VBC package')
    source=(example/'browser'/'app.js').read_text()
    required=['executeTurboJsChunk','turboJsWasm','clearTurboJsConsoleEvents']
    missing=[token for token in required if token not in source]
    if missing: raise SystemExit('playground runtime bridge coverage missing: '+', '.join(missing))
    if 'new Function' in source or 'globalThis.eval' in source or 'window.eval' in source: raise SystemExit('browser source evaluation is forbidden')
    if 'await eval(${JSON.stringify(source)})' not in source: raise SystemExit('TurboJS completion-value wrapper is missing')
    index=(out/'index.html').read_text(encoding='utf-8')
    if 'assets/javascript/.js' in index or 'assets/.js' in index:
        raise SystemExit('empty JavaScript asset filename emitted')
    package=next(p for p in out.rglob('*.vbc'))
    inspection=subprocess.run([str(venom),'inspect',str(package)],check=True,capture_output=True,text=True).stdout
    flags_line=next((line for line in inspection.splitlines() if line.strip().startswith('flags:')), '')
    if 'release' in flags_line.split(':',1)[-1].split('|'): raise SystemExit('development playground package incorrectly carries release profile')
    if 'release-diversification.vrd3' in inspection or 'turbojs-abi-fingerprint.vqaf' in inspection:
        raise SystemExit('development playground emitted release-only contracts')
    empty_assets=[p for p in out.rglob('*') if p.is_file() and p.name in {'.js','.css','.wasm','.vbc'}]
    if empty_assets:
        raise SystemExit('empty role asset filenames emitted: '+', '.join(str(p.relative_to(out)) for p in empty_assets))

