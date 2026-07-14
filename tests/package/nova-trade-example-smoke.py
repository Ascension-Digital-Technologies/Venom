#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parents[2]
site=root/'examples/nova-trade'
required=['index.html','README.md','venom.toml','protected/terminal-engine.js','assets/js/venom-terminal.js']
for rel in required:
 assert (site/rel).is_file(), rel
engine=(site/'protected/terminal-engine.js').read_text(encoding='utf-8')
bridge=(site/'assets/js/venom-terminal.js').read_text(encoding='utf-8')
for name in ('assessOrder','analyzePortfolio','generateSignal'):
 assert f'export function {name}' in engine
 assert name in bridge
assert '// @venom: protected module' in engine
print('nova trade example smoke passed')
