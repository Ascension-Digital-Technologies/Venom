#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
sh = (root/'tools'/'linux-scripts'/'build-turbojs-wasm.sh').read_text(encoding='utf-8')
ps = (root/'tools'/'windows-scripts'/'build-turbojs-wasm.ps1').read_text(encoding='utf-8')
checker = (root / 'tools/check_wasm_release_strings.py').read_text(encoding='utf-8')
required = ('-DNDEBUG', '-g0', '-ffile-prefix-map', '-fdebug-prefix-map', '-fmacro-prefix-map', '-sASSERTIONS=0', '-sSAFE_HEAP=0', '-sSTACK_OVERFLOW_CHECK=0')
failed = [token for token in required if token not in sh or token not in ps]
for token in ('--strip-debug', '--strip-dwarf', '--strip-producers'):
    if token not in sh or token not in ps:
        failed.append(token)
if '--strip-all' in sh or '--strip-all' in ps:
    failed.append('unsupported --strip-all assumption')
if '--help' not in sh or '--help' not in ps:
    failed.append('wasm-opt capability detection')
if 'check_wasm_release_strings.py' not in sh or 'check_wasm_release_strings.py' not in ps:
    failed.append('release string checker invocation')
if 'turbojs_runtime_scaffold.c' not in checker or 'absolute Windows path' not in checker:
    failed.append('release string checker policy')
if failed:
    raise SystemExit('TurboJS release build hygiene checks failed: ' + ', '.join(failed))
print('TurboJS release build hygiene smoke: ok')
