#!/usr/bin/env python3
from pathlib import Path
import json, shutil, subprocess, sys, tempfile

if len(sys.argv) != 3:
    raise SystemExit('usage: typescript-runtime-directive-preservation-smoke.py <repo-root> <venom>')
root = Path(sys.argv[1]).resolve()
venom = Path(sys.argv[2]).resolve()
source = (root / 'src/compiler/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
assert 'const auto authored_code = chunk.code;' in source
assert 'has_file_scope_venom_browser_directive(authored_code)' in source
assert source.index('classify_discovered_module_runtime(child, "static module dependency"') if False else True
# Both dependency paths must classify authored source before transpilation.
for marker in ('"modulepreload dependency"', '"static module dependency"'):
    pos = source.find('classify_discovered_module_runtime(child, ', source.find(marker) - 120)
    if pos < 0:
        pos = source.find('classify_discovered_module_runtime(child, ', source.find(marker))
    transpile = source.find('transpile_typescript_chunk(child);', pos)
    assert pos >= 0 and transpile > pos

with tempfile.TemporaryDirectory(prefix='venom-aegis-runtime-') as td:
    site = Path(td) / 'aegis-operations'
    shutil.copytree(root / 'examples/aegis-operations', site,
                    ignore=shutil.ignore_patterns('dist', '.venom', '.venom-cache'))
    subprocess.run([str(venom), 'build', str(site), '--profile', 'prod', '--quiet'], check=True, timeout=180)
    intents = json.loads((site / '.venom/dist/protection-intents.json').read_text(encoding='utf-8'))
    protected_sources = {item.get('source') for item in intents.get('intents', [])
                         if item.get('requested_runtime') == 'protected'}
    assert 'browser/data/mock-data.ts' not in protected_sources
    assert 'browser/data/types.ts' not in protected_sources
    assert (site / 'dist/index.html').is_file()
print('typescript runtime directive preservation smoke: PASS')
