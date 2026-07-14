#!/usr/bin/env python3
"""Verify every repository-local CMake include resolves to a packaged file."""
from __future__ import annotations
import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
missing: list[str] = []
checked: set[str] = set()
pattern = re.compile(r"\binclude\(\s*['\"]?(cmake/[^)'\"\s]+\.cmake)['\"]?\s*\)")
for cmake_file in [root / 'CMakeLists.txt', *root.rglob('CMakeLists.txt'), *root.rglob('*.cmake')]:
    if not cmake_file.is_file():
        continue
    text = cmake_file.read_text(encoding='utf-8', errors='replace')
    for rel in pattern.findall(text):
        checked.add(rel)
        if not (root / rel).is_file():
            missing.append(f'{cmake_file.relative_to(root)} -> {rel}')
if missing:
    raise SystemExit('missing repository-local CMake module(s):\n  ' + '\n  '.join(sorted(set(missing))))
required = {'cmake/build_acceleration.cmake'}
for rel in required:
    if not (root / rel).is_file():
        raise SystemExit(f'missing required packaged CMake module: {rel}')
print(f'CMake module completeness smoke: PASS ({len(checked)} local include(s))')
