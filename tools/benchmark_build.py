#!/usr/bin/env python3
"""Measure a clean and unchanged warm Venom build and enforce optional budgets."""
from __future__ import annotations
import argparse, json, os, shutil, subprocess, tempfile, time
from pathlib import Path

def run(command: list[str], env: dict[str, str]) -> float:
    started=time.perf_counter()
    subprocess.run(command, check=True, env=env)
    return time.perf_counter()-started

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--venom', required=True, type=Path)
    ap.add_argument('--site', required=True, type=Path)
    ap.add_argument('--profile', default='prod')
    ap.add_argument('--cold-budget', type=float, default=0.0)
    ap.add_argument('--warm-budget', type=float, default=0.0)
    ap.add_argument('--output-json', type=Path)
    args=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='venom-build-benchmark-') as td:
        root=Path(td); dist=root/'dist'; cache=root/'cache'
        env=dict(os.environ); env.setdefault('SOURCE_DATE_EPOCH','1700000000')
        command=[str(args.venom),'build',str(args.site),'--profile',args.profile,'--out',str(dist),'--cache-dir',str(cache),'--quiet']
        cold=run(command,env); warm=run(command,env)
        perf=root/'.venom'/dist.name/'build-performance.json'
        details=json.loads(perf.read_text(encoding='utf-8')) if perf.is_file() else {}
        result={'schema_version':1,'cold_seconds':round(cold,4),'warm_seconds':round(warm,4),
                'speedup':round(cold/max(warm,1e-9),3),'warm_report':details}
        text=json.dumps(result,indent=2)+'\n'
        print(text,end='')
        if args.output_json:
            args.output_json.parent.mkdir(parents=True,exist_ok=True); args.output_json.write_text(text,encoding='utf-8')
        if args.cold_budget and cold > args.cold_budget: raise SystemExit(f'cold build exceeded {args.cold_budget}s budget: {cold:.3f}s')
        if args.warm_budget and warm > args.warm_budget: raise SystemExit(f'warm build exceeded {args.warm_budget}s budget: {warm:.3f}s')
    return 0
if __name__=='__main__': raise SystemExit(main())
