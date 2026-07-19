#!/usr/bin/env python3
"""Validate domain-local public APIs and private header isolation."""
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


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    src = root / 'src'
    domains = {path.name for path in src.iterdir() if path.is_dir()}
    failures: list[str] = []
    public_headers: list[Path] = []
    private_headers: list[Path] = []

    for domain in sorted(domains - {'templates'}):
        domain_root = src / domain
        include_root = domain_root / 'include' / 'venom' / domain
        for header in sorted(domain_root.rglob('*')):
            if not header.is_file() or header.suffix.lower() not in HEADER_SUFFIXES:
                continue
            if 'include' in header.relative_to(domain_root).parts:
                public_headers.append(header)
                try:
                    header.relative_to(include_root)
                except ValueError:
                    failures.append(
                        f'public header is outside src/{domain}/include/venom/{domain}: '
                        f'{header.relative_to(root).as_posix()}'
                    )
            else:
                private_headers.append(header)

    for header in public_headers:
        text = header.read_text(encoding='utf-8', errors='replace')
        if '#pragma once' not in text and not re.search(r'^\s*#\s*ifndef\b', text, re.MULTILINE):
            failures.append(f'public header has no include guard: {header.relative_to(root).as_posix()}')
        owner = header.relative_to(src).parts[0]
        for include in INCLUDE_RE.findall(text):
            parts = include.split('/')
            if parts and parts[0] in domains:
                failures.append(
                    f'{header.relative_to(root).as_posix()}: public header uses legacy include {include}'
                )
            if include.startswith('venom/'):
                target = include.split('/')[1] if len(include.split('/')) > 2 else ''
                if target not in domains:
                    failures.append(
                        f'{header.relative_to(root).as_posix()}: unknown Venom API domain in {include}'
                    )
                continue
            # Quoted project headers from a public header would expose a private
            # include-directory assumption to consumers.
            if include.endswith(tuple(HEADER_SUFFIXES)) and not include.startswith(('quickjs', 'sys/')):
                candidate = src / owner / include
                if candidate.exists():
                    failures.append(
                        f'{header.relative_to(root).as_posix()}: public API reaches private header {include}'
                    )

    # A private header may only be included from its owning domain. Resolve
    # quoted/private include spellings against each domain root.
    private_by_domain: dict[str, set[str]] = {}
    for header in private_headers:
        domain = header.relative_to(src).parts[0]
        private_by_domain.setdefault(domain, set()).add(header.relative_to(src / domain).as_posix())

    for path in sorted(src.rglob('*')):
        if not path.is_file() or path.suffix.lower() not in NATIVE_SUFFIXES:
            continue
        source_domain = path.relative_to(src).parts[0]
        for include in INCLUDE_RE.findall(path.read_text(encoding='utf-8', errors='replace')):
            for owner, private_names in private_by_domain.items():
                if include in private_names and owner != source_domain:
                    failures.append(
                        f'{path.relative_to(root).as_posix()}: {source_domain} includes private '
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
        'src/${ARG_DOMAIN}/include',
        'src/${ARG_DOMAIN}',
        'venom_${_dependency}_api',
        'VENOM_DOMAIN_API_TARGETS',
    )
    for marker in required_markers:
        if marker not in source_domains:
            failures.append(f'source-domain CMake is missing API-boundary marker: {marker}')

    if failures:
        print('header-boundary check failed:', file=sys.stderr)
        for failure in sorted(set(failures)):
            print(f'  - {failure}', file=sys.stderr)
        return 1

    print(
        f'header-boundary check passed: {len(public_headers)} public, '
        f'{len(private_headers)} private headers'
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
