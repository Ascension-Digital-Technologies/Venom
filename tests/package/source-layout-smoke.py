#!/usr/bin/env python3
import sys
from pathlib import Path


def line_count(path: Path) -> int:
    return len(path.read_text(encoding='utf-8').splitlines())


def main() -> int:
    if len(sys.argv) != 2:
        print('usage: source-layout-smoke.py <repo-root>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    expected = {
        'src/compiler/js.cpp': 700,
        'src/compiler/worker_runtime_js.cpp': 500,
        'src/compiler/runtime_js.cpp': 4200,
        'src/compiler/wasm_runtime_js.cpp': 700,
        'src/compiler/quickjs_engine_module.cpp': 1400,
    }
    failures = []
    for rel, maximum in expected.items():
        path = root / rel
        if not path.exists():
            failures.append(f'missing {rel}')
            continue
        lines = line_count(path)
        if lines > maximum:
            failures.append(f'{rel} has {lines} lines, expected <= {maximum}')

    cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
    for rel in expected:
        if rel not in cmake:
            failures.append(f'CMakeLists.txt does not include {rel}')

    doc = root / 'docs/source-layout.md'
    if not doc.exists():
        failures.append('missing docs/source-layout.md')
    else:
        text = doc.read_text(encoding='utf-8')
        for needle in ('runtime_js.cpp', 'worker_runtime_js.cpp', 'wasm_runtime_js.cpp', 'quickjs_engine_module.cpp', 'generated blob'):
            if needle not in text:
                failures.append(f'docs/source-layout.md missing {needle!r}')

    if failures:
        for failure in failures:
            print('failure:', failure)
        return 1
    print('source layout smoke passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
