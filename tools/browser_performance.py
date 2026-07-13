#!/usr/bin/env python3
"""Evaluate distribution-bound browser validation timing against regression ceilings."""
from __future__ import annotations
import argparse, json, math, sys
from pathlib import Path

def percentile(values, p):
    if not values: return None
    vals=sorted(float(v) for v in values)
    k=(len(vals)-1)*p
    lo=math.floor(k); hi=math.ceil(k)
    return vals[lo] if lo==hi else vals[lo]*(hi-k)+vals[hi]*(k-lo)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('reports', nargs='+', type=Path)
    ap.add_argument('--budget', type=Path)
    ap.add_argument('--format', choices=('text','json'), default='text')
    ap.add_argument('--json-out', type=Path)
    a=ap.parse_args()
    root=Path(__file__).resolve().parents[1]
    policy=json.loads((a.budget or root/'contracts/browser-performance.json').read_text())
    b=policy['budgets']
    metrics={k:[] for k in ('browser_launch_ms','context_create_ms','navigation_wall_ms','fixture_ready_ms','action_ms','scenario_total_ms','runner_total_ms')}
    sources=[]
    for path in a.reports:
        r=json.loads(path.read_text())
        if r.get('schema_version') != 1: raise SystemExit(f'unsupported browser report schema: {path}')
        sources.append({'path':str(path),'fixture_id':r.get('fixture_id'),'dist_sha256':r.get('dist_sha256'),'manifest_sha256':r.get('manifest_sha256')})
        for item in r.get('results',[]):
            perf=item.get('performance') or {}
            if isinstance(perf.get('browser_launch_ms'),(int,float)): metrics['browser_launch_ms'].append(perf['browser_launch_ms'])
            if isinstance(perf.get('context_create_ms'),(int,float)): metrics['context_create_ms'].append(perf['context_create_ms'])
            for sc in perf.get('scenarios') or []:
                for src,dst in [('navigation_wall_ms','navigation_wall_ms'),('fixture_ready_ms','fixture_ready_ms'),('total_ms','scenario_total_ms')]:
                    if isinstance(sc.get(src),(int,float)): metrics[dst].append(sc[src])
                metrics['action_ms'].extend(x for x in (sc.get('action_ms') or []) if isinstance(x,(int,float)))
            if isinstance(item.get('duration_ms'),(int,float)): metrics['runner_total_ms'].append(item['duration_ms'])
    gates=[]; passed=True; summary={}
    for name,vals in metrics.items():
        observed=max(vals) if vals else None
        summary[name]={'samples':len(vals),'max_ms':observed,'p50_ms':percentile(vals,.5),'p95_ms':percentile(vals,.95),'budget_ms':b[name]}
        ok=observed is not None and observed <= b[name]
        gates.append({'metric':name,'passed':ok,'observed_ms':observed,'budget_ms':b[name]})
        passed &= ok
    out={'schema':'venom.browser-performance.v1','passed':passed,'policy':policy.get('profile'),'sources':sources,'metrics':summary,'gates':gates}
    encoded=json.dumps(out,indent=2,sort_keys=True)
    if a.json_out:
        a.json_out.parent.mkdir(parents=True,exist_ok=True); a.json_out.write_text(encoded+'\n')
    if a.format=='json': print(encoded)
    else:
        print('Venom browser performance report')
        print(f"Result: {'PASS' if passed else 'FAIL'}")
        for g in gates: print(f"[{'PASS' if g['passed'] else 'FAIL'}] {g['metric']}: {g['observed_ms']} ms <= {g['budget_ms']} ms")
    return 0 if passed else 60
if __name__=='__main__': raise SystemExit(main())
