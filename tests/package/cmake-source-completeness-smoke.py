#!/usr/bin/env python3
"""Verify source/header/script paths named by CMake exist in source packages."""
from __future__ import annotations
import re
import sys
from pathlib import Path
root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
missing: list[str] = []
# Paths in build files are intentionally repository-relative in this project.
path_re = re.compile(r'(?<![A-Za-z0-9_])((?:src|tools|scripts|tests|cmake|contracts|examples)/[A-Za-z0-9_./+\-]+\.(?:cmake|json|cpp|cxx|hpp|mjs|ps1|bat|cc|py|js|sh|in|c|h))(?=[^A-Za-z0-9_.]|$)')
for build_file in [root/'CMakeLists.txt', *sorted((root/'cmake').rglob('*.cmake'))]:
    text = build_file.read_text(encoding='utf-8', errors='replace')
    for rel in path_re.findall(text):
        if '$' in rel or '<' in rel:
            continue
        if not (root/rel).is_file():
            missing.append(f'{build_file.relative_to(root)} -> {rel}')
if missing:
    raise SystemExit('missing CMake-referenced source-package file(s):\n  ' + '\n  '.join(sorted(set(missing))))
print('CMake source completeness smoke: PASS')
