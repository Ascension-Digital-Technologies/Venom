#!/usr/bin/env python3
from __future__ import annotations
import json, os, subprocess, sys, tempfile, time
from pathlib import Path

root=Path(__file__).resolve().parents[2]
venom=Path(sys.argv[1]).resolve()
site=root/'examples'/'typescript-showcase'
with tempfile.TemporaryDirectory(prefix='venom-cache-smoke-') as td:
    base=Path(td); dist=base/'dist'; cache=base/'cache'; env=dict(os.environ); env['SOURCE_DATE_EPOCH']='1700000000'
    command=[str(venom),'build',str(site),'--profile','prod','--out',str(dist),'--cache-dir',str(cache),'--quiet']
    t=time.perf_counter(); subprocess.run(command,check=True,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True); cold=time.perf_counter()-t
    t=time.perf_counter(); subprocess.run(command,check=True,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True); warm=time.perf_counter()-t
    report=json.loads((base/'.venom'/'dist'/'build-performance.json').read_text(encoding='utf-8'))
    assert report['phases'], 'performance report must contain phase timings in quiet mode'
    assert report['cache']['typescript']['hits'] >= 5, report
    assert report['cache']['quickjs_bytecode']['hits'] >= 1, report
    assert report['cache']['typescript']['misses'] == 0, report
    assert warm <= cold * 1.25, (cold,warm)
print('incremental build cache smoke passed')
