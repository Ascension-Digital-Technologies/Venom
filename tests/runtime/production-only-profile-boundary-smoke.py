#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[2]
profile=(root/'src/core/profile.cpp').read_text()
header=(root/'include/venom/core/profile.hpp').read_text()
cli=(root/'src/cli/cli.cpp').read_text()
assert 'ProfileKind::Dev' not in header
assert 'if (name == "dev") raise_error' in profile
assert '--profile must be prod' in cli
print('production-only profile boundary: PASS')
