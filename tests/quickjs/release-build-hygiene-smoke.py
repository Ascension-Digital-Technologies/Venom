#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
sh = (root / 'scripts/build-quickjs-wasm.sh').read_text(encoding='utf-8')
ps = (root / 'scripts/build-quickjs-wasm.ps1').read_text(encoding='utf-8')
checker = (root / 'tools/check_wasm_release_strings.py').read_text(encoding='utf-8')
required = ('-DNDEBUG', '-g0', '-ffile-prefix-map', '-fdebug-prefix-map', '-fmacro-prefix-map', '-sASSERTIONS=0', '-sSAFE_HEAP=0', '-sSTACK_OVERFLOW_CHECK=0', '--strip-all')
failed = [token for token in required if token not in sh or token not in ps]
if 'check_wasm_release_strings.py' not in sh or 'check_wasm_release_strings.py' not in ps:
    failed.append('release string checker invocation')
if 'quickjs_runtime_scaffold.c' not in checker or 'absolute Windows path' not in checker:
    failed.append('release string checker policy')
if failed:
    raise SystemExit('QuickJS release build hygiene checks failed: ' + ', '.join(failed))
print('QuickJS release build hygiene smoke: ok')
