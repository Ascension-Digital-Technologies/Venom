#!/usr/bin/env python3
"""One-command qualification of the flagship protected-chess release."""
from __future__ import annotations
import argparse, hashlib, os, shutil, subprocess, sys
from pathlib import Path

def run(cmd, *, cwd:Path, env=None):
    print('+',' '.join(map(str,cmd)), flush=True)
    subprocess.run([str(x) for x in cmd],cwd=cwd,env=env,check=True)

def digest_tree(root:Path)->str:
    h=hashlib.sha256()
    for f in sorted(x for x in root.rglob('*') if x.is_file()):
        rel=f.relative_to(root).as_posix().encode(); data=f.read_bytes()
        h.update(len(rel).to_bytes(4,'big')); h.update(rel); h.update(len(data).to_bytes(8,'big')); h.update(data)
    return h.hexdigest()

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--venom',type=Path,required=True)
    ap.add_argument('--site',type=Path,default=Path('examples/protected-chess'))
    ap.add_argument('--dist',type=Path,default=Path('build/qualified-chess-dist'))
    ap.add_argument('--seed',type=int,default=1350001)
    ap.add_argument('--skip-reproducibility',action='store_true')
    ap.add_argument('--browser', choices=('chromium','firefox','webkit','all'))
    ap.add_argument('--browser-report', type=Path, default=Path('build/protected-chess-browser.json'))
    args=ap.parse_args()
    root=Path(__file__).resolve().parents[1]
    venom=args.venom.resolve(); site=(root/args.site).resolve() if not args.site.is_absolute() else args.site.resolve(); dist=(root/args.dist).resolve() if not args.dist.is_absolute() else args.dist.resolve()
    env={**os.environ,'SOURCE_DATE_EPOCH':os.environ.get('SOURCE_DATE_EPOCH','1704067200')}
    if dist.exists(): shutil.rmtree(dist)
    run([venom,'build',site,'--out',dist,'--seed',str(args.seed)],cwd=root,env=env)
    run([venom,'verify-runtime',dist,'--target','browser','--require-real-engine'],cwd=root,env=env)
    run([sys.executable,root/'tools/check_production_leaks.py',dist],cwd=root,env=env)
    run([sys.executable,root/'tools/tamper_gate.py',dist,'--venom',venom],cwd=root,env=env)
    if args.browser:
        report=(root/args.browser_report).resolve() if not args.browser_report.is_absolute() else args.browser_report.resolve()
        run([sys.executable,root/'tools/browser_validation.py',dist,'--browser',args.browser,'--manifest',site/'venom.browser.json','--json-out',report],cwd=root,env=env)
    if not args.skip_reproducibility:
        a=dist.parent/'repro-a'; b=dist.parent/'repro-b'; c=dist.parent/'repro-different-seed'
        for x in (a,b,c):
            if x.exists(): shutil.rmtree(x)
        run([venom,'build',site,'--out',a,'--seed',str(args.seed)],cwd=root,env=env)
        run([venom,'build',site,'--out',b,'--seed',str(args.seed)],cwd=root,env=env)
        run([venom,'build',site,'--out',c,'--seed',str(args.seed+1)],cwd=root,env=env)
        da,db,dc=map(digest_tree,(a,b,c))
        if da!=db: raise SystemExit('qualification failed: identical seeds produced different distributions')
        if da==dc: raise SystemExit('qualification failed: different seeds produced identical distributions')
        print(f'Reproducibility: PASS ({da[:16]}...)')
    print('Venom release qualification: PASS')
    return 0
if __name__=='__main__': raise SystemExit(main())
