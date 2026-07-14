#!/usr/bin/env python3
from pathlib import Path
import ast
root=Path(__file__).resolve().parents[1]
text=(root/'tools/package_release.py').read_text()
ast.parse(text)
for token in ["out_dir / 'bin'","out_dir / 'runtime'","CONTRACTS.json","install.ps1","target-triplet"]:
    assert token in text, token
install=(root/'tools/install_release.py').read_text(); ast.parse(install)
assert 'add_argument("release"' in install
assert 'sub.add_parser("verify"' in install
print('release package layout smoke: PASS')
