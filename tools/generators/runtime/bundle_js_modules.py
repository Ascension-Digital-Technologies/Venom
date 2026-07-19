#!/usr/bin/env python3
from __future__ import annotations
import argparse
from pathlib import Path

def write_if_changed(path: Path, content: str) -> bool:
    if path.is_file() and path.read_text(encoding="utf-8") == content:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(content, encoding="utf-8")
    temporary.replace(path)
    return True

def main() -> int:
    ap=argparse.ArgumentParser(description='Concatenate ordered authored JavaScript modules deterministically.')
    ap.add_argument('--input-dir', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    ap.add_argument('--header', default='')
    ap.add_argument('--annotate', action='store_true')
    args=ap.parse_args()
    modules=sorted(p for p in args.input_dir.glob('*.js') if p.is_file())
    if not modules:
        raise SystemExit(f'no JavaScript modules found in {args.input_dir}')
    chunks=[]
    if args.header:
        chunks.append(f'/* {args.header} */\n')
    for module in modules:
        text=module.read_text(encoding='utf-8')
        if args.annotate:
            chunks.append(f'/* module: {module.name} */\n')
        chunks.append(text)
        if not text.endswith('\n'):
            chunks.append('\n')
    write_if_changed(args.output, ''.join(chunks))
    return 0
if __name__=='__main__': raise SystemExit(main())
