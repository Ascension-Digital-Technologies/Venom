#!/usr/bin/env python3
from pathlib import Path
import argparse

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--input', required=True)
    ap.add_argument('--output', required=True)
    args=ap.parse_args()
    text=Path(args.input).read_text(encoding='utf-8')
    delim='VENOMRT'
    while f'){delim}"' in text:
        delim += '_X'
    out=f'''#include "compiler/services/runtime_js_template.hpp"\n\nnamespace venom::compiler {{\nstd::string runtime_js_template() {{\n  return R"{delim}({text}){delim}";\n}}\n}} // namespace venom::compiler\n'''
    op=Path(args.output); op.parent.mkdir(parents=True, exist_ok=True); op.write_text(out, encoding='utf-8')
if __name__=='__main__': main()
