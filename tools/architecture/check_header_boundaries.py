#!/usr/bin/env python3
"""Validate Venom's centralized public and internal header boundaries."""
from __future__ import annotations

import re
import sys
from pathlib import Path

HEADER_SUFFIXES = {'.h', '.hh', '.hpp', '.hxx'}
NATIVE_SUFFIXES = HEADER_SUFFIXES | {'.c', '.cc', '.cpp', '.cxx'}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
BROAD_INCLUDE_PATTERNS = (
    r'target_include_directories\([^)]*\b(?:PUBLIC|PRIVATE|INTERFACE)\s+src(?:\s|\))',
    r'\$\{CMAKE_CURRENT_SOURCE_DIR\}/src(?:["\s)]|$)',
    r'\$\{CMAKE_SOURCE_DIR\}/src(?:["\s)]|$)',
)


def owner_for_path(path: Path, src: Path, header_root: Path) -> str | None:
    try:
        return path.relative_to(src).parts[0]
    except ValueError:
        pass
    try:
        relative = path.relative_to(header_root)
    except ValueError:
        return None
    if not relative.parts:
        return None
    if relative.parts[0] == 'internal':
        return relative.parts[1] if len(relative.parts) > 1 else None
    return relative.parts[0]


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    src = root / 'src'
    header_root = root / 'include' / 'venom'
    internal_root = header_root / 'internal'
    domains = {path.name for path in src.iterdir() if path.is_dir()}
    failures: list[str] = []
    public_headers: list[Path] = []
    internal_headers: list[Path] = []

    if not header_root.is_dir():
        failures.append('missing central header tree: include/venom')
    if not internal_root.is_dir():
        failures.append('missing centralized internal header tree: include/venom/internal')

    for path in sorted(src.rglob('*')):
        if path.is_file() and path.suffix.lower() in HEADER_SUFFIXES:
            failures.append(
                f'first-party header must be centralized under include/venom: '
                f'{path.relative_to(root).as_posix()}'
            )
        if path.is_dir() and path.name == 'include':
            failures.append(
                f'domain-local include directory is not allowed: {path.relative_to(root).as_posix()}'
            )

    if header_root.is_dir():
        for header in sorted(header_root.rglob('*')):
            if not header.is_file() or header.suffix.lower() not in HEADER_SUFFIXES:
                continue
            relative = header.relative_to(header_root)
            if relative.parts[0] == 'internal':
                if len(relative.parts) < 3:
                    failures.append(
                        f'internal header must live under include/venom/internal/<domain>/: '
                        f'{header.relative_to(root).as_posix()}'
                    )
                    continue
                owner = relative.parts[1]
                if owner not in domains:
                    failures.append(
                        f'internal header has unknown domain {owner}: '
                        f'{header.relative_to(root).as_posix()}'
                    )
                internal_headers.append(header)
                continue

            if len(relative.parts) < 2:
                failures.append(
                    f'public header must live under include/venom/<domain>/: '
                    f'{header.relative_to(root).as_posix()}'
                )
                continue
            owner = relative.parts[0]
            if owner not in domains:
                failures.append(
                    f'public header has unknown domain {owner}: {header.relative_to(root).as_posix()}'
                )
            public_headers.append(header)

    for header in [*public_headers, *internal_headers]:
        text = header.read_text(encoding='utf-8', errors='replace')
        kind = 'public' if header in public_headers else 'internal'
        if '#pragma once' not in text and not re.search(r'^\s*#\s*ifndef\b', text, re.MULTILINE):
            failures.append(f'{kind} header has no include guard: {header.relative_to(root).as_posix()}')

    for header in public_headers:
        for include in INCLUDE_RE.findall(header.read_text(encoding='utf-8', errors='replace')):
            if include.startswith('venom/internal/'):
                failures.append(
                    f'{header.relative_to(root).as_posix()}: public API reaches internal header {include}'
                )
            parts = include.split('/')
            if parts and parts[0] in domains:
                failures.append(
                    f'{header.relative_to(root).as_posix()}: public header uses legacy include {include}'
                )
            if include.startswith('venom/') and not include.startswith('venom/internal/'):
                target = parts[1] if len(parts) > 2 else ''
                if target not in domains:
                    failures.append(
                        f'{header.relative_to(root).as_posix()}: unknown Venom API domain in {include}'
                    )

    native_paths = list(src.rglob('*')) + list(header_root.rglob('*'))
    for path in sorted(native_paths):
        if not path.is_file() or path.suffix.lower() not in NATIVE_SUFFIXES:
            continue
        source_domain = owner_for_path(path, src, header_root)
        if source_domain is None:
            continue
        for include in INCLUDE_RE.findall(path.read_text(encoding='utf-8', errors='replace')):
            if not include.startswith('venom/internal/'):
                continue
            parts = include.split('/')
            if len(parts) < 4:
                failures.append(f'{path.relative_to(root).as_posix()}: malformed internal include {include}')
                continue
            owner = parts[2]
            target = root / 'include' / include
            if not target.is_file():
                failures.append(
                    f'{path.relative_to(root).as_posix()}: internal include does not resolve: {include}'
                )
            if owner != source_domain:
                failures.append(
                    f'{path.relative_to(root).as_posix()}: {source_domain} includes internal '
                    f'{owner} header {include}'
                )

    cmake_text = '\n'.join(
        p.read_text(encoding='utf-8', errors='replace')
        for p in [root / 'CMakeLists.txt', *sorted((root / 'cmake').rglob('*.cmake'))]
        if p.is_file()
    )
    for pattern in BROAD_INCLUDE_PATTERNS:
        if re.search(pattern, cmake_text, re.DOTALL):
            failures.append(f'CMake exposes repository-wide src include path matching {pattern}')

    source_domains = (root / 'cmake' / 'source_domains.cmake').read_text(encoding='utf-8')
    required_markers = (
        'CMAKE_CURRENT_SOURCE_DIR}/include',
        'include/venom/internal/core/pch.hpp',
        'venom_${_dependency}_api',
        'VENOM_DOMAIN_API_TARGETS',
    )
    for marker in required_markers:
        if marker not in source_domains:
            failures.append(f'source-domain CMake is missing header-boundary marker: {marker}')
    if 'src/${ARG_DOMAIN}' in source_domains:
        failures.append('source-domain CMake still exposes domain source directories as include paths')

    tests_cmake = (root / 'cmake' / 'tests.cmake').read_text(encoding='utf-8')
    if 'venom_test_private_headers' in tests_cmake or 'src/${_domain}' in tests_cmake:
        failures.append('tests still depend on source-directory private include paths')

    if failures:
        print('header-boundary check failed:', file=sys.stderr)
        for failure in sorted(set(failures)):
            print(f'  - {failure}', file=sys.stderr)
        return 1

    print(
        f'header-boundary check passed: {len(public_headers)} public, '
        f'{len(internal_headers)} internal headers; no first-party headers under src/'
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
