#!/usr/bin/env python3
"""Enforce Venom's source-domain dependency and include-boundary rules."""
from __future__ import annotations

import re
import sys
from collections import defaultdict
from pathlib import Path

NATIVE_SUFFIXES = {'.c', '.cc', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx'}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)

# Generated source is a compiled snapshot of APIs owned by pipeline/runtime.
# It may implement declarations from those owners, while first-party code may
# consume generated contracts and blobs. It is excluded from authored-cycle
# analysis but still has an explicit implementation dependency allowlist.
ALLOWED: dict[str, set[str]] = {
    'base': set(),
    'cli': {'base', 'core', 'package', 'pipeline', 'turbojs', 'runtime'},
    'core': {'base', 'package'},
    'frontends': {'base', 'core', 'generated', 'package'},
    'generated': {'pipeline', 'runtime'},
    'graph': {'frontends', 'package'},
    'package': {'base', 'generated'},
    'pipeline': {'base', 'core', 'frontends', 'generated', 'graph', 'package', 'turbojs', 'remote', 'vm'},
    'turbojs': {'base', 'package'},
    'remote': {'base', 'core', 'package'},
    'runtime': {'base', 'core', 'generated'},
    'templates': set(),
    'vm': {'base', 'package'},
}


def source_header_path(src: Path, include: str) -> Path | None:
    parts = include.split('/')
    if len(parts) < 2:
        return None
    return src / include


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
    src = root / 'src'
    actual_domains = {path.name for path in src.iterdir() if path.is_dir()}
    failures: list[str] = []

    if actual_domains != set(ALLOWED):
        failures.append(
            f'domain policy mismatch: expected {sorted(ALLOWED)}, got {sorted(actual_domains)}'
        )

    edges: dict[str, set[str]] = defaultdict(set)
    for path in sorted(src.rglob('*')):
        if not path.is_file() or path.suffix.lower() not in NATIVE_SUFFIXES:
            continue
        relative = path.relative_to(src)
        source_domain = relative.parts[0]
        text = path.read_text(encoding='utf-8', errors='replace')
        for include in INCLUDE_RE.findall(text):
            include_parts = include.split('/')
            target_domain: str | None = None
            if include_parts and include_parts[0] in actual_domains:
                target_domain = include_parts[0]
                header = source_header_path(src, include)
                generated_version = include == 'core/version.hpp'
                if header is not None and not header.is_file() and not generated_version:
                    failures.append(
                        f'{path.relative_to(root).as_posix()}: source-root include does not resolve: {include}'
                    )
            elif include.startswith('venom/'):
                failures.append(
                    f'{path.relative_to(root).as_posix()}: obsolete central include spelling: {include}'
                )

            if target_domain not in actual_domains or target_domain == source_domain:
                continue
            edges[source_domain].add(target_domain)
            if target_domain not in ALLOWED.get(source_domain, set()):
                failures.append(
                    f'{path.relative_to(root).as_posix()}: forbidden {source_domain} -> '
                    f'{target_domain} include ({include})'
                )

    # CLI is an outer adapter. No reusable product domain may include it.
    for source_domain, dependencies in edges.items():
        if source_domain != 'cli' and 'cli' in dependencies:
            failures.append(f'{source_domain} must not depend on cli')

    # Verify the authored dependency graph is acyclic. Generated snapshots are
    # excluded because they intentionally implement owner-domain declarations.
    authored = set(ALLOWED) - {'generated', 'templates'}
    graph = {
        domain: {dep for dep in edges.get(domain, set()) if dep in authored}
        for domain in authored
    }
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(domain: str, trail: list[str]) -> None:
        if domain in visiting:
            begin = trail.index(domain)
            failures.append('authored domain cycle: ' + ' -> '.join(trail[begin:] + [domain]))
            return
        if domain in visited:
            return
        visiting.add(domain)
        for dependency in sorted(graph[domain]):
            visit(dependency, trail + [domain])
        visiting.remove(domain)
        visited.add(domain)

    for domain in sorted(authored):
        visit(domain, [])

    if failures:
        print('source-domain dependency check failed:', file=sys.stderr)
        for failure in sorted(set(failures)):
            print(f'  - {failure}', file=sys.stderr)
        return 1

    print('source-domain dependency check passed')
    for domain in sorted(actual_domains):
        deps = ', '.join(sorted(edges.get(domain, set()))) or '(none)'
        print(f'  {domain}: {deps}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
