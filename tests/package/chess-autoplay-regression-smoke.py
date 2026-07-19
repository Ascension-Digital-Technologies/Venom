from pathlib import Path
root = Path(__file__).resolve().parents[2]
engine = (root / 'examples/protected-chess/js/ai-engine.js').read_text(encoding='utf-8')
main = (root / 'examples/protected-chess/js/main.js').read_text(encoding='utf-8')
html = (root / 'examples/protected-chess/index.html').read_text(encoding='utf-8')

# Velocity engine identity and long-running search architecture.
assert 'const VELOCITY_ENGINE_VERSION = "0.5.0";' in engine
assert 'class TranspositionTable' in engine
assert 'class Position' in engine
assert 'class Search' in engine
assert 'quiescence(position, alpha, beta, ply)' in engine
assert 'negamax(position, depth, alpha, beta, ply' in engine
assert 'null-move-pruning' in engine
assert 'incremental-zobrist-hashing' in engine
assert 'function runChessEngine(request)' in engine
assert 'async function runChessEngine(request)' not in engine

# Browser autoplay remains asynchronous and fail-closed around the protected export.
assert 'async function startAutoPlay()' in main
assert 'function stopAutoPlay()' in main
assert 'scheduleAutoPlay' in main
assert "$button.prop('disabled', true).text('Starting…')" in main
assert 'await ensureProtectedRuntime()' in main
assert 'api.exports.runChessEngine' in main
assert 'void verifyProtectedRuntime();' in main
assert '\nverifyProtectedRuntime();\n' not in main
assert "await makeBestMove(color)" in main
assert "setStatus('Engine error: ' + error.message, true)" in main
assert 'type="button" class="btn btn-success" id="compVsCompBtn"' in html
print('chess autoplay regression smoke: PASS')
