#!/usr/bin/env python3
"""Measure Venom configure/build/incremental performance reproducibly."""
from __future__ import annotations
import argparse, json, os, platform, shutil, subprocess, sys, time
from pathlib import Path


def run(cmd: list[str], cwd: Path, log: Path) -> tuple[int, float]:
    start=time.perf_counter()
    proc=subprocess.run(cmd,cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    elapsed=time.perf_counter()-start
    log.parent.mkdir(parents=True,exist_ok=True)
    log.write_text(proc.stdout,encoding='utf-8',errors='replace')
    return proc.returncode,elapsed


def size(path: Path) -> int | None:
    return path.stat().st_size if path.exists() else None


def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument('--repo-root',type=Path,default=Path(__file__).resolve().parents[1])
    ap.add_argument('--build-dir',type=Path)
    ap.add_argument('--generator',default='Ninja')
    ap.add_argument('--config',default='Release')
    ap.add_argument('--parallel',type=int,default=max(1,min(8,os.cpu_count() or 2)))
    ap.add_argument('--compiler-cache',choices=['auto','none','ccache','sccache'],default='auto')
    ap.add_argument('--with-ipo',action='store_true')
    ap.add_argument('--with-quickjs-ipo',action='store_true')
    ap.add_argument('--skip-clean',action='store_true')
    ap.add_argument('--json-out',type=Path)
    args=ap.parse_args()
    root=args.repo_root.resolve()
    build=(args.build_dir or root/'build'/'performance').resolve()
    if not args.skip_clean and build.exists(): shutil.rmtree(build)
    build.mkdir(parents=True,exist_ok=True)
    logs=build/'performance-logs'
    cmake=['cmake','-S',str(root),'-B',str(build),'-G',args.generator,
           f'-DCMAKE_BUILD_TYPE={args.config}',f'-DVENOM_COMPILER_CACHE={args.compiler_cache}',
           f'-DVENOM_ENABLE_IPO={"ON" if args.with_ipo else "OFF"}',
           f'-DVENOM_ENABLE_QUICKJS_IPO={"ON" if args.with_quickjs_ipo else "OFF"}',
           '-DVENOM_ENABLE_PCH=ON']
    rc_cfg,t_cfg=run(cmake,root,logs/'configure.log')
    steps=[{'name':'configure','seconds':round(t_cfg,3),'passed':rc_cfg==0}]
    if rc_cfg: report={'schema':'venom.build-performance.v1','passed':False,'steps':steps}; print(json.dumps(report,indent=2)); return rc_cfg
    build_cmd=['cmake','--build',str(build),'--parallel',str(args.parallel)]
    if 'Visual Studio' in args.generator: build_cmd += ['--config',args.config]
    rc_full,t_full=run(build_cmd,root,logs/'clean-build.log'); steps.append({'name':'clean_build','seconds':round(t_full,3),'passed':rc_full==0})
    rc_noop,t_noop=run(build_cmd,root,logs/'noop-build.log'); steps.append({'name':'noop_build','seconds':round(t_noop,3),'passed':rc_noop==0})
    touch=root/'src/pipeline/js.cpp'; original=touch.stat().st_mtime_ns
    os.utime(touch,None)
    rc_inc,t_inc=run(build_cmd,root,logs/'incremental-build.log'); steps.append({'name':'incremental_build','seconds':round(t_inc,3),'passed':rc_inc==0})
    os.utime(touch,ns=(touch.stat().st_atime_ns,original))
    exe=build/('Release/venom.exe' if 'Visual Studio' in args.generator else 'venom')
    cache_tool=shutil.which('sccache') or shutil.which('ccache')
    cache_stats=None
    if cache_tool:
        flag='--show-stats' if Path(cache_tool).name.startswith('sccache') else '-s'
        p=subprocess.run([cache_tool,flag],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        cache_stats=p.stdout.strip()
    report={
      'schema':'venom.build-performance.v1','passed':all(s['passed'] for s in steps),
      'environment':{'platform':platform.platform(),'python':platform.python_version(),'cpu_count':os.cpu_count(),'generator':args.generator,'parallel':args.parallel},
      'configuration':{'pch':True,'compiler_cache':args.compiler_cache,'ipo':args.with_ipo,'quickjs_ipo':args.with_quickjs_ipo},
      'steps':steps,'artifacts':{'venom_bytes':size(exe)},'cache_stats':cache_stats,
    }
    text=json.dumps(report,indent=2)
    print(text)
    out=(args.json_out or build/'build-performance-report.json')
    out.parent.mkdir(parents=True,exist_ok=True); out.write_text(text+'\n',encoding='utf-8')
    return 0 if report['passed'] else 1
if __name__=='__main__': raise SystemExit(main())
