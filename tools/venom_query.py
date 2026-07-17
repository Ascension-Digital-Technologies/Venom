#!/usr/bin/env python3
"""Query a Venom module graph from the command line."""
from __future__ import annotations
import argparse, json, sys
from collections import deque
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import venom_explain as explain

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('site',type=Path); ap.add_argument('command',choices=('health','module','dependents','dependencies','boundaries','path')); ap.add_argument('value',nargs='?'); ap.add_argument('target',nargs='?'); ap.add_argument('--json',action='store_true'); a=ap.parse_args()
    root=Path(__file__).resolve().parents[1]; r=explain.analyze(a.site.resolve(),root,None,None)
    mods={m['file']:m for m in r['modules']}; result: object
    if a.command=='health': result=r['summary']
    elif a.command=='boundaries': result=[e for e in r['edges'] if e.get('targetRuntime')=='protected' and mods.get(e['from'],{}).get('runtime')=='browser']
    elif a.command in ('module','dependents','dependencies'):
        if not a.value or a.value not in mods: raise SystemExit(f'module not found: {a.value}')
        result=mods[a.value] if a.command=='module' else mods[a.value][a.command]
    else:
        if not a.value or not a.target: raise SystemExit('path requires source and target modules')
        graph={m:mods[m]['dependencies'] for m in mods}; q=deque([(a.value,[a.value])]); seen={a.value}; result=[]
        while q:
            cur,p=q.popleft()
            if cur==a.target: result=p; break
            for nxt in graph.get(cur,[]):
                if nxt not in seen: seen.add(nxt); q.append((nxt,p+[nxt]))
    print(json.dumps(result,indent=2,sort_keys=True) if a.json else (json.dumps(result,indent=2) if isinstance(result,(dict,list)) else str(result)))
    return 0
if __name__=='__main__': raise SystemExit(main())
