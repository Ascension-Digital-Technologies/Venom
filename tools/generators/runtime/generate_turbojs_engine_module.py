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
    ap=argparse.ArgumentParser(description='Generate the C++ TurboJS browser-engine wrapper from authored JS modules.')
    ap.add_argument('--input-dir', required=True, type=Path)
    ap.add_argument('--prefix', required=True, type=Path)
    ap.add_argument('--suffix', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    args=ap.parse_args()
    modules=sorted(p for p in args.input_dir.glob('*.js') if p.is_file())
    if not modules: raise SystemExit('no TurboJS engine modules found')
    js=''.join(p.read_text(encoding='utf-8') for p in modules)
    if ')TJSENGINE"' in js: raise SystemExit('TurboJS engine source contains reserved raw-string delimiter')
    generated=args.prefix.read_text(encoding='utf-8')+js+args.suffix.read_text(encoding='utf-8')
    write_if_changed(args.output, generated)
    return 0
if __name__=='__main__': raise SystemExit(main())
