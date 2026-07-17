from __future__ import annotations
import re, subprocess, sys, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
venom=Path(sys.argv[1]) if len(sys.argv)>1 else None
engine=(root/'examples/protected-chess/js/ai-engine.js').read_text(encoding='utf-8')
main=(root/'examples/protected-chess/js/main.js').read_text(encoding='utf-8')
assert '// @venom: protected isolated' in engine
assert 'function minimax' in engine and 'function evaluateBoard' in engine
assert 'function minimax' not in main and 'var weights =' not in main
assert 'new Chess(' not in main and 'var Chess =' not in main
assert not (root/'examples/protected-chess/js/chess.js').exists(), 'browser chess rules library must not exist'
index=(root/'examples/protected-chess/index.html').read_text(encoding='utf-8')
assert 'js/chess.js' not in index
assert 'api.exports.runChessEngine' in main
assert 'await api.ready()' in main
assert 'await runChessEngine' not in main
if venom:
    with tempfile.TemporaryDirectory(prefix='venom-chess-engine-') as temp:
        out=Path(temp)/'dist'
        built=subprocess.run([str(venom),'build',str(root/'examples/protected-chess'),'--out',str(out),'--profile','prod','--hashed'],check=True,text=True,capture_output=True)
        assert 'quickjs_bytecode_records: 1' in built.stdout, built.stdout
        assert 'release_status: PASS' in built.stdout, built.stdout
        assert not (out/'build/reports').exists(), 'hardened dist must not ship internal reports'
        shipped=''.join(p.read_text(encoding='utf-8',errors='ignore') for p in (out/'assets').rglob('*.js'))
        assert 'function minimax' not in shipped
        assert 'Piece Square Tables' not in shipped
        assert 'DEFAULT_POSITION' not in shipped
        assert 'PAWN_OFFSETS' not in shipped
        assert 'js/ai-engine.js::runChessEngine' not in shipped
        for marker in ('var Chess =', 'function Chess(', 'generate_moves', 'ugly_move', 'in_checkmate', 'in_stalemate'):
            assert marker not in shipped, f'protected chess rules leaked into browser JavaScript: {marker}'
print('protected chess engine smoke: PASS')
