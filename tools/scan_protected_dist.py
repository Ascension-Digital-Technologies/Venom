#!/usr/bin/env python3
"""Fail-closed structural scanner for Venom protected browser distributions."""
from __future__ import annotations
import argparse, re, sys
from pathlib import Path

HASH = r"[0-9a-f]{12}"
ALLOWED = [
    re.compile(r"index\.html"),
    re.compile(r"assets/app/build\.json"),
    re.compile(rf"assets/app/app\.{HASH}\.vbc"),
    re.compile(rf"assets/style/s\.{HASH}\.css"),
    re.compile(rf"assets/loader/loader\.{HASH}\.js"),
    re.compile(rf"assets/runtime/engine\.{HASH}\.js"),
    re.compile(rf"assets/runtime/runtime\.{HASH}\.wasm"),
    re.compile(rf"assets/runtime/r\.{HASH}\.js"),
    re.compile(rf"assets/runtime/rw\.{HASH}\.wasm"),
    re.compile(r"assets/(?!app/|style/|loader/|runtime/|workers/).+"),  # public site assets
    re.compile(r"assets/workers/worker\.[0-9a-f]{12}\.js"),
]
FORBIDDEN_TEXT = (
    "new Function", "eval(", "sourceMappingURL", "host-js-fallback",
    "legacy-host-js-wrapper", "strict_release=false", "release_fail_closed=false",
)

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('dist', type=Path)
    args=ap.parse_args(); root=args.dist.resolve()
    failures=[]
    if not root.is_dir():
        print(f"[venom] protected dist not found: {root}", file=sys.stderr); return 2
    files=sorted(p for p in root.rglob('*') if p.is_file())
    rels=[p.relative_to(root).as_posix() for p in files]
    for rel in rels:
        if rel.endswith(('.map','.d.ts')) or rel.startswith('.') or '/.' in rel:
            failures.append(f"forbidden debug/hidden artifact: {rel}")
            continue
        if not any(rx.fullmatch(rel) for rx in ALLOWED):
            failures.append(f"unexpected protected artifact: {rel}")
    roots=[r for r in rels if '/' not in r]
    allowed_roots={'index.html'}
    if set(roots) != allowed_roots:
        failures.append('protected root must contain only index.html (found: '+', '.join(roots)+')')
    required=(
        r"assets/app/build\.json", rf"assets/app/app\.{HASH}\.vbc", rf"assets/style/s\.{HASH}\.css",
        rf"assets/loader/loader\.{HASH}\.js", rf"assets/runtime/engine\.{HASH}\.js",
        rf"assets/runtime/runtime\.{HASH}\.wasm", rf"assets/runtime/r\.{HASH}\.js",
        rf"assets/runtime/rw\.{HASH}\.wasm", rf"assets/workers/worker\.{HASH}\.js",
    )
    for pattern in required:
        if not any(re.fullmatch(pattern, rel) for rel in rels):
            failures.append(f"missing required protected artifact matching: {pattern}")
    runtime_gate_found = False
    for p, rel in zip(files, rels):
        if p.suffix in {'.js','.html','.css'}:
            text=p.read_text(encoding='utf-8', errors='replace')
            if re.fullmatch(rf"assets/runtime/r\.{HASH}\.js", rel) and 'unvendored network script denied by production runtime' in text:
                runtime_gate_found = True
            for needle in FORBIDDEN_TEXT:
                if needle in text:
                    failures.append(f"forbidden marker {needle!r} in {rel}")
    if not runtime_gate_found:
        failures.append('runtime bridge is missing the fail-closed unvendored network script gate')
    if failures:
        print('[venom] protected distribution scan: FAIL', file=sys.stderr)
        for f in failures: print('[venom]  - '+f, file=sys.stderr)
        return 1
    print(f'[venom] protected distribution scan: PASS ({len(files)} files)')
    return 0
if __name__=='__main__': raise SystemExit(main())
