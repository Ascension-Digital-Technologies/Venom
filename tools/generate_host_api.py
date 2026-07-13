#!/usr/bin/env python3
import argparse, json
from pathlib import Path

def render(data):
    calls=data['calls']
    ids=[c['id'] for c in calls]
    names=[c['name'] for c in calls]
    if len(ids)!=len(set(ids)) or len(names)!=len(set(names)):
        raise SystemExit('duplicate host API id or name')
    h=['#pragma once','/* Generated from contracts/host-api.json. Do not edit. */',f'#define VENOM_HOST_BRIDGE_ABI {data["host_bridge_abi"]}u','typedef enum venom_host_call_id {']
    for c in calls: h.append(f'  VENOM_HOST_CALL_{c["name"].upper().replace(".", "_")} = {c["id"]}u,')
    h += ['} venom_host_call_id;','']
    md=['# Generated Host API Contract','',f'Host bridge ABI: **{data["host_bridge_abi"]}**','', '| ID | Call | Capability | Maximum payload |','|---:|---|---|---:|']
    for c in calls: md.append(f'| {c["id"]} | `{c["name"]}` | `{c["capability"]}` | {c["max_payload"]} bytes |')
    return '\n'.join(h)+'\n','\n'.join(md)+'\n'

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--check',action='store_true'); a=ap.parse_args()
    root=Path(__file__).resolve().parents[1]; data=json.loads((root/'contracts/host-api.json').read_text())
    header,docs=render(data); outputs={root/'src/runtime/host_api_contract.h':header, root/'docs/generated/host-api-contract.md':docs}
    bad=[]
    for p,text in outputs.items():
        if a.check:
            if not p.exists() or p.read_text()!=text: bad.append(str(p.relative_to(root)))
        else: p.parent.mkdir(parents=True,exist_ok=True); p.write_text(text)
    if bad: raise SystemExit('generated host API outputs are stale: '+', '.join(bad))
if __name__=='__main__': main()
