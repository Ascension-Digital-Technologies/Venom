#!/usr/bin/env python3
"""Fail-closed publication gate for signed Venom browser distributions."""
from __future__ import annotations
import argparse, shutil, tempfile, zipfile
from pathlib import Path
from dist_release_root import verify

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('dist',type=Path); ap.add_argument('--public-key',type=Path,required=True); ap.add_argument('--out',type=Path,required=True); ap.add_argument('--openssl',default='openssl')
    ns=ap.parse_args(); dist=ns.dist.resolve()
    failures=verify(dist,ns.public_key,ns.openssl,True)
    if failures:
        for f in failures: print('[FAIL]',f)
        raise SystemExit('publication refused: distribution is unsigned, untrusted, or modified')
    ns.out.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(ns.out,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(dist.rglob('*')):
            if p.is_file(): z.write(p,p.relative_to(dist).as_posix())
    print(f'[PASS] published signed distribution: {ns.out}')
if __name__=='__main__': main()
