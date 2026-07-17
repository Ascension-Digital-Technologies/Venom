#!/usr/bin/env python3
"""Summarize Venom browser startup timelines and enforce performance budgets."""
from __future__ import annotations
import argparse, json, math, statistics
from pathlib import Path


def percentile(values, q):
    if not values: return None
    xs=sorted(float(v) for v in values); idx=max(0,min(len(xs)-1,math.ceil(q*len(xs))-1)); return xs[idx]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('reports',nargs='+',type=Path)
    ap.add_argument('--contract',type=Path,default=Path(__file__).resolve().parents[1]/'contracts/runtime-performance.json')
    ap.add_argument('--json-out',type=Path)
    ap.add_argument('--markdown-out',type=Path)
    a=ap.parse_args(); contract=json.loads(a.contract.read_text())
    samples={}
    for path in a.reports:
        data=json.loads(path.read_text())
        results=data.get('results',data if isinstance(data,list) else [])
        for result in results:
            boot=result.get('bootStatus') or result.get('boot') or {}
            for entry in boot.get('timeline',[]):
                if entry.get('state')=='complete': samples.setdefault(entry['phase'],[]).append(float(entry.get('durationMs',0)))
            if boot.get('durationMs') is not None: samples.setdefault('complete',[]).append(float(boot['durationMs']))
    metrics={}; failures=[]
    for phase, values in sorted(samples.items()):
        median=statistics.median(values); p95=percentile(values,.95)
        metrics[phase]={'samples':len(values),'medianMs':round(median,3),'p95Ms':round(p95,3),'minMs':round(min(values),3),'maxMs':round(max(values),3)}
        budget=contract.get('budgetsMs',{}).get(phase)
        if budget:
            if median>budget['median']: failures.append(f'{phase} median {median:.1f}ms exceeds {budget["median"]}ms')
            if p95>budget['p95']: failures.append(f'{phase} p95 {p95:.1f}ms exceeds {budget["p95"]}ms')
    result={'schema':'VENOM_RUNTIME_PERFORMANCE_RESULT_V1','passed':not failures,'metrics':metrics,'failures':failures,'contractVersion':contract['version']}
    payload=json.dumps(result,indent=2,sort_keys=True)+'\n'
    lines=['# Venom Startup Performance','',f"**Result:** {'PASS' if result['passed'] else 'FAIL'}",'', '| Phase | Samples | Median | p95 |','|---|---:|---:|---:|']
    for phase,m in metrics.items(): lines.append(f"| `{phase}` | {m['samples']} | {m['medianMs']} ms | {m['p95Ms']} ms |")
    if failures: lines += ['','## Budget failures','']+[f'- {x}' for x in failures]
    md='\n'.join(lines)+'\n'
    if a.json_out: a.json_out.parent.mkdir(parents=True,exist_ok=True); a.json_out.write_text(payload)
    if a.markdown_out: a.markdown_out.parent.mkdir(parents=True,exist_ok=True); a.markdown_out.write_text(md)
    print(payload,end='')
    return 0 if result['passed'] else 2
if __name__=='__main__': raise SystemExit(main())
