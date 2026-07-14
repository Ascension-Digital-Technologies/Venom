#!/usr/bin/env python3
"""Check that a new CONTRACTS.json is compatible with a prior release."""
from __future__ import annotations
import argparse, json
from pathlib import Path

def major(v):
    try:return int(v.split('.',1)[0])
    except:return 0

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('previous',type=Path); ap.add_argument('current',type=Path); a=ap.parse_args()
    old=json.loads(a.previous.read_text()); new=json.loads(a.current.read_text())
    same_major=major(old.get('product_version','0'))==major(new.get('product_version','0'))
    errors=[]
    oc=old.get('contracts',{}); nc=new.get('contracts',{})
    if same_major:
      for k,v in oc.items():
        if k not in nc: errors.append(f'removed stable contract: {k}')
        elif isinstance(v,int) and isinstance(nc[k],int) and nc[k] < v: errors.append(f'contract version decreased: {k} {v}->{nc[k]}')
        elif isinstance(v,str) and nc[k] != v: errors.append(f'named contract changed within major release: {k} {v!r}->{nc[k]!r}')
    if errors:
      print('Venom contract upgrade check: FAIL')
      for e in errors: print(' - '+e)
      return 1
    print('Venom contract upgrade check: PASS')
    return 0
if __name__=='__main__': raise SystemExit(main())
