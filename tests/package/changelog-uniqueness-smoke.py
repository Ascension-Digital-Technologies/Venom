#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2]
text = (root / 'CHANGES.md').read_text(encoding='utf-8')
failures = []
if text.count('# Changelog') != 1:
    failures.append(f'expected one top-level changelog heading, found {text.count("# Changelog")}')
versions = re.findall(r'^##\s+(\d+\.\d+\.\d+)\b', text, re.MULTILINE)
duplicates = sorted({v for v in versions if versions.count(v) > 1})
if duplicates:
    failures.append('duplicate changelog versions: ' + ', '.join(duplicates))
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
m = re.search(r'VERSION\s+(\d+\.\d+\.\d+)', cmake)
if not m:
    failures.append('could not resolve CMake project version')
elif versions.count(m.group(1)) != 1:
    failures.append(f'current version {m.group(1)} must appear exactly once in CHANGES.md')
if failures:
    print('\n'.join(failures), file=sys.stderr)
    raise SystemExit(1)
print('changelog uniqueness smoke: PASS')
