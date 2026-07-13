#!/usr/bin/env python3
"""Compare two Venom release directories or archives byte-for-byte."""
from __future__ import annotations
import argparse, hashlib, json, tarfile, tempfile, zipfile
from pathlib import Path


def digest(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def extract(path: Path, root: Path) -> Path:
    if path.is_dir(): return path.resolve()
    out=root/path.stem.replace('.tar','')
    out.mkdir(parents=True)
    if path.name.lower().endswith('.zip'):
        with zipfile.ZipFile(path) as z:
            base=out.resolve()
            for i in z.infolist():
                if not (out/i.filename).resolve().is_relative_to(base): raise SystemExit(f'unsafe archive path: {i.filename}')
            z.extractall(out)
    elif path.name.lower().endswith(('.tar.gz','.tgz')):
        with tarfile.open(path,'r:gz') as t:
            base=out.resolve()
            for m in t.getmembers():
                if not (out/m.name).resolve().is_relative_to(base): raise SystemExit(f'unsafe archive path: {m.name}')
            t.extractall(out)
    else: raise SystemExit(f'unsupported input: {path}')
    children=[p for p in out.iterdir()]
    return children[0] if len(children)==1 and children[0].is_dir() else out

def inventory(root: Path) -> dict[str,tuple[int,str]]:
    return {p.relative_to(root).as_posix():(p.stat().st_size,digest(p)) for p in sorted(root.rglob('*')) if p.is_file()}

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('first',type=Path); ap.add_argument('second',type=Path)
    ap.add_argument('--format',choices=['text','json'],default='text')
    a=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='venom-repro-') as td:
        tr=Path(td); ar=extract(a.first.resolve(),tr/'a'); br=extract(a.second.resolve(),tr/'b')
        ai,bi=inventory(ar),inventory(br)
        missing=sorted(set(ai)-set(bi)); extra=sorted(set(bi)-set(ai))
        changed=sorted(k for k in set(ai)&set(bi) if ai[k]!=bi[k])
        ok=not missing and not extra and not changed
        result={'reproducible':ok,'first':str(a.first),'second':str(a.second),'file_count':len(ai),'missing':missing,'extra':extra,'changed':changed}
        if a.format=='json': print(json.dumps(result,indent=2,sort_keys=True))
        else:
            print('Venom reproducibility check')
            print(f"status: {'PASS' if ok else 'FAIL'}")
            print(f'files: {len(ai)}')
            for label,items in [('missing',missing),('extra',extra),('changed',changed)]:
                for item in items: print(f'{label}: {item}')
        return 0 if ok else 1
if __name__=='__main__': raise SystemExit(main())
