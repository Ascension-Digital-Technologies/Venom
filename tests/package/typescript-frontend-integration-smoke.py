#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[2]
cpp = (root / 'src/compiler/pipeline/typescript_frontend.cpp').read_text()
hpp = (root / 'src/compiler/pipeline/typescript_frontend.hpp').read_text()
assert 'typescript_structural_frontend.js' in cpp
assert 'VENOM_TYPESCRIPT_FRONTEND' in cpp
assert 'transpile_structural' in cpp
assert 'source_map' in hpp
assert 'frontend_version' in hpp
assert (root / 'third_party/typescript/LICENSE.txt').is_file()
print('TypeScript frontend integration smoke passed')
