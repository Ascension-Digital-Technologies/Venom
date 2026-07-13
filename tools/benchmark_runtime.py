#!/usr/bin/env python3
"""Measure Venom artifact, command, and browser runtime performance with regression gates."""
import argparse, json, os, statistics, subprocess, sys, tempfile, time
from pathlib import Path


def percentile(values, q):
    if not values: return None
    ordered=sorted(values); pos=(len(ordered)-1)*q; lo=int(pos); hi=min(lo+1,len(ordered)-1)
    return ordered[lo]+(ordered[hi]-ordered[lo])*(pos-lo)

def stats(samples):
    return {'runs':len(samples),'median_ms':statistics.median(samples),'mean_ms':statistics.mean(samples),
            'p95_ms':percentile(samples,.95),'min_ms':min(samples),'max_ms':max(samples)}

def run_timed(command, runs, cwd=None):
    samples=[]
    for _ in range(runs):
        started=time.perf_counter(); cp=subprocess.run(command,cwd=cwd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        elapsed=(time.perf_counter()-started)*1000
        if cp.returncode:
            raise RuntimeError(f"command failed ({cp.returncode}): {' '.join(command)}\n{cp.stderr[-2000:]}")
        samples.append(elapsed)
    return stats(samples)

def artifact_report(item):
    d=Path(item); files=[f for f in d.rglob('*') if f.is_file()]
    return {'path':str(d),'bytes':sum(f.stat().st_size for f in files),'files':len(files),
            'wasm_bytes':sum(f.stat().st_size for f in files if f.suffix=='.wasm'),
            'js_bytes':sum(f.stat().st_size for f in files if f.suffix=='.js'),
            'package_bytes':sum(f.stat().st_size for f in files if f.suffix in ('.vbc','.pax'))}

def browser_samples(url,runs):
    script=r'''const { chromium }=require('playwright');(async()=>{const b=await chromium.launch({headless:true});let out=[];for(let i=0;i<Number(process.argv[2]);i++){const p=await b.newPage();const t=performance.now();await p.goto(process.argv[1],{waitUntil:'networkidle'});const nav=await p.evaluate(()=>{const n=performance.getEntriesByType('navigation')[0];return {dom:n? n.domContentLoadedEventEnd:0,load:n?n.loadEventEnd:0,heap:performance.memory?performance.memory.usedJSHeapSize:null}});out.push({wall:performance.now()-t,...nav});await p.close()}console.log(JSON.stringify(out));await b.close()})().catch(e=>{console.error(e);process.exit(1)});'''
    cp=subprocess.run(['node','-e',script,url,str(runs)],capture_output=True,text=True,check=True)
    rows=json.loads(cp.stdout.strip().splitlines()[-1])
    result={'wall':stats([r['wall'] for r in rows]),'dom_content_loaded':stats([r['dom'] for r in rows]),'load':stats([r['load'] for r in rows])}
    heaps=[r['heap'] for r in rows if r['heap'] is not None]
    if heaps: result['heap_bytes']={'median':statistics.median(heaps),'max':max(heaps)}
    return result

def get_path(obj,path):
    cur=obj
    for part in path.split('.'):
        cur=cur[int(part)] if isinstance(cur, list) else cur[part]
    return cur

p=argparse.ArgumentParser()
p.add_argument('--dist',action='append',default=[])
p.add_argument('--url'); p.add_argument('--runs',type=int,default=10)
p.add_argument('--command',action='append',default=[],help='Command to benchmark; repeatable')
p.add_argument('--command-runs',type=int,default=5)
p.add_argument('--cwd'); p.add_argument('--baseline'); p.add_argument('--max-regression-percent',type=float,default=10.0)
p.add_argument('--budget',action='append',default=[],help='metric=max, e.g. boot.wall.p95_ms=900')
p.add_argument('--json-out'); a=p.parse_args()
report={'schema':'venom.performance.v2','artifacts':[artifact_report(x) for x in a.dist],'commands':[],'boot':None,'gates':[]}
for command in a.command:
    import shlex
    argv=shlex.split(command,posix=os.name!='nt'); report['commands'].append({'command':argv,'timing':run_timed(argv,a.command_runs,a.cwd)})
if a.url: report['boot']=browser_samples(a.url,a.runs)
failed=False
for spec in a.budget:
    path,raw=spec.rsplit('=',1); actual=float(get_path(report,path)); maximum=float(raw); ok=actual<=maximum
    report['gates'].append({'kind':'absolute','metric':path,'actual':actual,'maximum':maximum,'passed':ok}); failed|=not ok
if a.baseline:
    base=json.loads(Path(a.baseline).read_text())
    candidates=[]
    if report['boot'] and base.get('boot'): candidates += ['boot.wall.median_ms','boot.wall.p95_ms']
    if report['artifacts'] and base.get('artifacts'): candidates += ['artifacts.0.bytes','artifacts.0.wasm_bytes','artifacts.0.js_bytes']
    for path in candidates:
        before=float(get_path(base,path)); after=float(get_path(report,path)); regression=0 if before==0 else (after-before)*100/before; ok=regression<=a.max_regression_percent
        report['gates'].append({'kind':'baseline','metric':path,'baseline':before,'actual':after,'regression_percent':regression,'maximum_percent':a.max_regression_percent,'passed':ok}); failed|=not ok
text=json.dumps(report,indent=2); print(text)
if a.json_out: Path(a.json_out).write_text(text+'\n')
sys.exit(2 if failed else 0)
