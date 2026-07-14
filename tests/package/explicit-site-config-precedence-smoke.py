#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
cli = (root / 'src/compiler/commands/cli.cpp').read_text(encoding='utf-8')
config = (root / 'src/compiler/core/config.cpp').read_text(encoding='utf-8')
ps = (root / 'scripts/build-quickjs-wasm.ps1').read_text(encoding='utf-8')
assert 'const auto explicit_input = cmd.build.input;' in cli
assert 'if (!explicit_input.empty()) cmd.build.input = explicit_input;' in cli
assert 'config_dir / options.input' in config
assert '& python $Tool @Arguments | Out-Host' in ps
assert 'return [int]$Code' in ps
print('explicit site/config precedence and PowerShell runtime verifier: PASS')
