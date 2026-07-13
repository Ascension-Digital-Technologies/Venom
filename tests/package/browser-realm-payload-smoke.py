#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[2]
build = (root / 'src/compiler/build.cpp').read_text(encoding='utf-8')
runtime = (root / 'src/compiler/runtime_js.cpp').read_text(encoding='utf-8')
js = (root / 'src/compiler/js.cpp').read_text(encoding='utf-8')

required_build = [
    'const bool browser_side = (chunk.flags & JsChunkBrowser) != 0u;',
    'payload.assign(chunk.code.begin(), chunk.code.end());',
    'chunk.flags & ~JsChunkBytecodeEncoded',
    'if ((chunk.flags & JsChunkBrowser) != 0u) continue;',
    'chunk_count=\" << protected_count',
]
for marker in required_build:
    if marker not in build:
        raise SystemExit(f'missing lazy browser-source packaging marker: {marker}')

required_runtime = [
    'browser script chunk cannot contain QuickJS bytecode',
    'browser script chunk was packaged as QuickJS bytecode',
]
for marker in required_runtime:
    if marker not in runtime:
        raise SystemExit(f'missing browser-bytecode fail-closed guard: {marker}')

# The normal non-lazy encoder must retain the same invariant as the lazy route
# encoder. This catches future drift between the two packaging paths.
for marker in [
    'if (browser_side) payload.assign(chunk.code.begin(), chunk.code.end());',
    'entry.flags = browser_side ? chunk.flags : (chunk.flags | JsChunkBytecodeEncoded);',
]:
    if marker not in js:
        raise SystemExit(f'missing primary browser-source packaging marker: {marker}')

print('browser realm payload smoke passed')
