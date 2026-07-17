#!/usr/bin/env python3
from pathlib import Path
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print('usage: dom-stack-balance-smoke.py <repo-root>', file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve()
    source = root / 'src/compiler/pipeline/html.cpp'
    if not source.is_file():
        print(f'failure: missing {source}')
        return 1
    text = source.read_text(encoding='utf-8')
    required = (
        'open_elements',
        'open_elements.rbegin()',
        'open_elements.rend()',
        'const auto close_count',
        'open_elements.pop_back()',
        'while (!open_elements.empty())',
        'apply_implied_start_closures',
        'close_through("li")',
        'close_through("p")',
        'close_through("td")',
    )
    missing = [needle for needle in required if needle not in text]
    if missing:
        for needle in missing:
            print(f'failure: html parser missing stack-balance contract {needle!r}')
        return 1
    print('DOM stack balance smoke: PASS')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
