from pathlib import Path
root = Path(__file__).resolve().parents[2]
engine = (root / 'examples/protected-chess/js/ai-engine.js').read_text(encoding='utf-8')
main = (root / 'examples/protected-chess/js/main.js').read_text(encoding='utf-8')
html = (root / 'examples/protected-chess/index.html').read_text(encoding='utf-8')
assert 'var positionCount = 0;' in engine
assert '\n  positionCount = 0;' not in engine
assert 'async function startAutoPlay()' in main
assert 'function stopAutoPlay()' in main
assert 'scheduleAutoPlay' in main
assert "$button.prop('disabled', true).text('Starting…')" in main
assert 'await ensureProtectedRuntime()' in main
assert 'api.exports.runChessEngine' in main
assert 'void verifyProtectedRuntime();' in main
assert '\nverifyProtectedRuntime();\n' not in main
assert "await makeBestMove(color)" in main
assert "automatic game failed" in main
assert 'type="button" class="btn btn-success" id="compVsCompBtn"' in html
print('chess autoplay regression smoke: PASS')

engine=(root/'examples/protected-chess/js/ai-engine.js').read_text(encoding='utf-8')
assert 'function runChessEngine(request)' in engine
assert 'async function runChessEngine(request)' not in engine
