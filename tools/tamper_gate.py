#!/usr/bin/env python3
"""Verify that representative production-asset mutations fail closed."""
from __future__ import annotations
import argparse, shutil, subprocess, tempfile
from pathlib import Path

def mutate(path: Path) -> None:
    data = bytearray(path.read_bytes())
    if not data: raise RuntimeError(f"cannot mutate empty file: {path}")
    index = min(len(data)-1, max(0, len(data)//2))
    data[index] ^= 0x5A
    path.write_bytes(data)

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('dist', type=Path)
    ap.add_argument('--venom', type=Path, required=True)
    args=ap.parse_args()
    dist=args.dist.resolve(); venom=args.venom.resolve()
    patterns=['assets/app/*.vbc','assets/loader/*.js','assets/runtime/*.wasm']
    targets=[]
    for pattern in patterns:
        found=sorted(dist.glob(pattern))
        if found: targets.append(found[0].relative_to(dist))
    if len(targets) < 3:
        raise SystemExit('tamper gate failed: expected app, loader, and runtime WASM assets')
    failures=[]
    for rel in targets:
        with tempfile.TemporaryDirectory(prefix='venom-tamper-') as td:
            trial=Path(td)/'dist'; shutil.copytree(dist,trial)
            mutate(trial/rel)
            proc=subprocess.run([str(venom),'verify-runtime',str(trial),'--target','browser','--require-real-engine'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            if proc.returncode == 0: failures.append(f'mutation was accepted: {rel}')
    with tempfile.TemporaryDirectory(prefix='venom-missing-') as td:
        trial=Path(td)/'dist'; shutil.copytree(dist,trial)
        wasm=next(iter(sorted(trial.glob('assets/runtime/*.wasm'))),None)
        if wasm: wasm.unlink()
        proc=subprocess.run([str(venom),'verify-runtime',str(trial),'--target','browser','--require-real-engine'],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        if proc.returncode == 0: failures.append('missing runtime WASM was accepted')
    if failures:
        print('Tamper gate: FAIL'); [print(' - '+x) for x in failures]; return 1
    print('Tamper gate: PASS')
    for rel in targets: print(f' - rejected mutation: {rel}')
    print(' - rejected missing runtime WASM')
    return 0
if __name__=='__main__': raise SystemExit(main())
