from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
venom = Path(sys.argv[1]) if len(sys.argv) > 1 else None
source = (root / 'src/pipeline/function_dependencies.cpp').read_text(encoding='utf-8') + (root / 'src/pipeline/js.cpp').read_text(encoding='utf-8')
source += (root / 'src/pipeline/js_discovery.cpp').read_text(encoding='utf-8')
source += (root / 'src/pipeline/js_rewriting.cpp').read_text(encoding='utf-8')
for marker in [
    'FunctionDependencyResolution', 'index_declarations',
    'resolve_liftable_function_dependencies', 'lifted_dependencies',
    'frontend::analyze_function_scope', 'dependency.unsafe_features.front()',
]:
    assert marker in source, marker

if venom is not None:
    with tempfile.TemporaryDirectory(prefix='venom-dependency-lift-') as temp:
        base = Path(temp)
        site = base / 'site'
        dist = base / 'dist'
        site.mkdir()
        (site / 'index.html').write_text(
            '<!doctype html><html><body><script data-venom="browser" src="app.js"></script></body></html>',
            encoding='utf-8',
        )
        (site / 'app.js').write_text(
            '''// @venom: browser\n'''
            '''const SCALE = 3;\n'''
            '''function square(value) { return value * value; }\n'''
            '''function score(value) { return square(value) * SCALE; }\n'''
            '''// @venom: protected\n'''
            '''function evaluate(value) { return score(value) + SCALE; }\n'''
            '''async function boot() { return await evaluate(4); }\n'''
            '''boot();\n''',
            encoding='utf-8',
        )
        subprocess.run(
            [str(venom), 'build', str(site), '--out', str(dist), '--format', 'text', '--profile', 'prod'],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
        report = json.loads((dist / 'build/reports/bridge-rewrite-plan.json').read_text(encoding='utf-8'))
        record = next(item for item in report['functions'] if item['function'] == 'evaluate')
        assert record['status'] == 'extracted', record
        assert set(record['lifted_dependencies']) == {'SCALE', 'square', 'score'}, record
        assert 'pure lexical dependencies' in record['reason'], record

print('function dependency lifting smoke: PASS')
