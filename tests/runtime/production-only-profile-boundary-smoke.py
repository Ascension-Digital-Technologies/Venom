#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
profile=(root/'src/compiler/core/profile.cpp').read_text()
header=(root/'src/compiler/core/profile.hpp').read_text()
cli=(root/'src/compiler/commands/cli.cpp').read_text()
assert 'ProfileKind::Dev' not in header
assert 'if (name == "dev") throw' in profile
assert '--profile must be prod' in cli
print('production-only profile boundary: PASS')
