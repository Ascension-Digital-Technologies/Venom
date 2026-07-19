#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
html = (root/'examples/protected-chess/index.html').read_text(encoding='utf-8')
main = (root/'examples/protected-chess/js/main.js').read_text(encoding='utf-8')
css = (root/'examples/protected-chess/css/main.css').read_text(encoding='utf-8')
for token in ('benchmarkBtn','benchmark-preset','benchmark-results','benchmark-progress-bar','metric-qnodes','metric-tt-rate','metric-cutoffs','metric-tt-stores'):
    assert token in html, f'missing dashboard control: {token}'
for token in ('BENCHMARK_PRESETS','runBenchmark','executeBenchmarkTest','applyEnginePreset','ENGINE_PRESETS','renderBenchmarkRows','benchmarkResultValue','benchmark-final-summary','benchmarkClockMs','wallElapsedMs','engineElapsedMs'):
    assert token in main, f'missing dashboard implementation: {token}'
for action in ("action: 'perft'", "action: 'search'"):
    assert action in main, f'benchmark must call protected engine action: {action}'
assert 'benchmark-table' in css and 'benchmark-summary-grid' in css
assert 'benchmark-final-summary' in html and 'benchmark-final-summary' in css
assert "row.id = 'benchmark-row-' + index" in main
assert "document.getElementById('benchmark-row-' + index)" in main
assert 'ai-engine.js' in html and 'data-venom="browser"' in html
print('protected chess performance dashboard smoke: PASS')
