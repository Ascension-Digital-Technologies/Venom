#!/usr/bin/env python3
"""Validate Venom's colocated source/header layout."""
from __future__ import annotations

import re
import sys
from pathlib import Path

HEADER_SUFFIXES = {'.h', '.hh', '.hpp', '.hxx'}
NATIVE_SUFFIXES = HEADER_SUFFIXES | {'.c', '.cc', '.cpp', '.cxx'}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    src = root / 'src'
    failures: list[str] = []

    if (root / 'include').exists():
        failures.append('top-level include directory is not allowed; headers belong beside sources')

    headers = [
        path for path in sorted(src.rglob('*'))
        if path.is_file() and path.suffix.lower() in HEADER_SUFFIXES
    ]
    sources = [
        path for path in sorted(src.rglob('*'))
        if path.is_file() and path.suffix.lower() in {'.c', '.cc', '.cpp', '.cxx'}
    ]

    for header in headers:
        text = header.read_text(encoding='utf-8', errors='replace')
        if '#pragma once' not in text and not re.search(r'^\s*#\s*ifndef\b', text, re.MULTILINE):
            failures.append(f'header has no include guard: {header.relative_to(root).as_posix()}')
        if '/include/' in header.as_posix() or header.parent.name == 'include':
            failures.append(f'nested include directory is not allowed: {header.relative_to(root).as_posix()}')

    # Project includes must be source-root relative. This keeps the owning domain
    # visible while allowing headers to sit beside their .cpp/.c implementations.
    for path in headers + sources:
        text = path.read_text(encoding='utf-8', errors='replace')
        for include in INCLUDE_RE.findall(text):
            if include.startswith('venom/'):
                failures.append(
                    f'{path.relative_to(root).as_posix()}: obsolete central include spelling {include}'
                )

    root_cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    if re.search(r'(?m)^\s*PUBLIC\s+include\s*$', root_cmake):
        failures.append('root CMake still exposes the removed top-level include directory')
    runtime_block = re.search(
        r'add_library\(venom_runtime_native\s+STATIC.*?target_include_directories\(venom_runtime_native(.*?)\n\s*\)',
        root_cmake,
        re.DOTALL,
    )
    if runtime_block is None or '${CMAKE_CURRENT_SOURCE_DIR}/src' not in runtime_block.group(1):
        failures.append('venom_runtime_native must publicly expose the src root for runtime/... headers')

    cmake = (root / 'cmake' / 'source_domains.cmake').read_text(encoding='utf-8')
    required = (
        '${CMAKE_CURRENT_SOURCE_DIR}/src',
        '${CMAKE_CURRENT_BINARY_DIR}/generated',
        'venom_${_dependency}_api',
        'VENOM_DOMAIN_API_TARGETS',
    )
    for marker in required:
        if marker not in cmake:
            failures.append(f'source-domain CMake is missing colocated-header marker: {marker}')
    if '${CMAKE_CURRENT_SOURCE_DIR}/include' in cmake:
        failures.append('source-domain CMake still exposes the removed central include tree')

    if failures:
        print('header-layout check failed:', file=sys.stderr)
        for failure in sorted(set(failures)):
            print(f'  - {failure}', file=sys.stderr)
        return 1

    print(f'header-layout check passed: {len(headers)} headers colocated with {len(sources)} sources')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
