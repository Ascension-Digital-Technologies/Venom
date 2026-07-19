#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
example = root / 'examples/chrome-extension'
visible = ['popup.js','content-script.js','background.js','engine-host.js','page-bridge.js','visual-board.js','humanize.js','match-modes.js']
for name in visible:
    text=(example/name).read_text()
    for forbidden in ['VelocityChessEngine','TranspositionTable','quiescence(','negamax(','analyzeFen(state.fen']:
        assert forbidden not in text, f'{forbidden} leaked into {name}'
protected=(example/'protected/velocity-engine.js').read_text()
assert 'export function analyzeVelocity' in protected
assert len(protected) > 15000
print('Chrome extension engine leak smoke: PASS')
