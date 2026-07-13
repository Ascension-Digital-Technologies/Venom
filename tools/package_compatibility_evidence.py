#!/usr/bin/env python3
"""Package browser compatibility reports and policy output into a deterministic evidence bundle."""
from __future__ import annotations
import argparse, hashlib, json, os, shutil, sys, tarfile, tempfile, zipfile
from pathlib import Path
from release_crypto import key_id_from_public_key, sign_ed25519, write_signature

def sha256(p: Path) -> str:
    h=hashlib.sha256();
    with p.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def canonical(obj)->bytes:
    return (json.dumps(obj,sort_keys=True,separators=(',',':'))+'\n').encode()

def main()->int:
    ap=argparse.ArgumentParser(description='Create a deterministic Venom compatibility-evidence bundle.')
    ap.add_argument('--matrix',type=Path,required=True)
    ap.add_argument('--reports',type=Path,nargs='+',required=True)
    ap.add_argument('--manifests',type=Path,nargs='*',default=[])
    ap.add_argument('--out',type=Path,required=True)
    ap.add_argument('--product-version',default='1.11.0')
    ap.add_argument('--source-revision',default=os.environ.get('GITHUB_SHA','unknown'))
    ap.add_argument('--private-key',type=Path)
    ap.add_argument('--public-key',type=Path)
    ap.add_argument('--key-id')
    ap.add_argument('--openssl',default='openssl')
    a=ap.parse_args()
    matrix=json.loads(a.matrix.read_text(encoding='utf-8'))
    if matrix.get('schema_version') != 2: raise SystemExit('unsupported compatibility matrix schema')
    if not matrix.get('support_policy_passed'): raise SystemExit('compatibility support policy did not pass')
    files=[]
    for kind, paths in [('matrix',[a.matrix]),('report',a.reports),('fixture-manifest',a.manifests)]:
        for p in paths:
            p=p.resolve()
            if not p.is_file(): raise SystemExit(f'missing evidence file: {p}')
            files.append((kind,p))
    seen=set(); entries=[]
    for kind,p in files:
        name=p.name
        if name in seen: name=f'{kind}-{len(entries)}-{name}'
        seen.add(name)
        entries.append({'kind':kind,'name':name,'sha256':sha256(p),'size':p.stat().st_size,'source':str(p)})
    manifest={'schema_version':1,'product':'Venom - Secure Web Runtime','product_version':a.product_version,'source_revision':a.source_revision,'matrix_passed':bool(matrix.get('passed')),'support_policy_passed':True,'required_browsers':matrix.get('required_browsers',[]),'minimum_checks_per_browser':matrix.get('minimum_checks_per_browser'),'files':[{k:v for k,v in e.items() if k!='source'} for e in entries]}
    with tempfile.TemporaryDirectory(prefix='venom-evidence-') as td:
        root=Path(td)/'compatibility-evidence'; root.mkdir()
        for e in entries: shutil.copy2(e['source'],root/e['name'])
        raw=canonical(manifest); (root/'EVIDENCE_MANIFEST.json').write_bytes(raw)
        if a.private_key:
            if not a.public_key: raise SystemExit('--private-key requires --public-key')
            kid=a.key_id or key_id_from_public_key(a.public_key,a.openssl)
            write_signature(root/'EVIDENCE_MANIFEST.sig',key_id=kid,signature=sign_ed25519(raw,a.private_key,a.openssl),manifest=raw)
            shutil.copy2(a.public_key,root/'SIGNING_PUBLIC_KEY.pem')
        out=a.out.resolve(); out.parent.mkdir(parents=True,exist_ok=True)
        epoch=int(os.environ.get('SOURCE_DATE_EPOCH','1704067200'))
        if out.suffix=='.zip':
            with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
                for p in sorted(root.iterdir()):
                    info=zipfile.ZipInfo(f'compatibility-evidence/{p.name}',(2024,1,1,0,0,0)); info.external_attr=0o100644<<16
                    z.writestr(info,p.read_bytes(),compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
        else:
            raise SystemExit('--out must end in .zip')
    print(json.dumps({'bundle':str(out),'sha256':sha256(out),'signed':bool(a.private_key)},indent=2))
    return 0
if __name__=='__main__': raise SystemExit(main())
