#!/usr/bin/env python3
"""Verify a Venom compatibility-evidence bundle."""
from __future__ import annotations
import argparse, hashlib, json, tempfile, zipfile
from pathlib import Path
from release_crypto import parse_signature, verify_ed25519

def main()->int:
    ap=argparse.ArgumentParser(); ap.add_argument('bundle',type=Path); ap.add_argument('--public-key',type=Path); ap.add_argument('--require-signature',action='store_true'); ap.add_argument('--openssl',default='openssl'); a=ap.parse_args()
    with tempfile.TemporaryDirectory(prefix='venom-evidence-verify-') as td:
        with zipfile.ZipFile(a.bundle) as z:
            names=z.namelist()
            if len(names) != len(set(names)):
                print(json.dumps({'valid':False,'signed':False,'failures':['duplicate archive entries']},indent=2)); return 60
            for name in names:
                pp=Path(name)
                if pp.is_absolute() or '..' in pp.parts or not name.startswith('compatibility-evidence/'):
                    print(json.dumps({'valid':False,'signed':False,'failures':['unsafe archive path: '+name]},indent=2)); return 60
            z.extractall(td)
        root=Path(td)/'compatibility-evidence'; mf=root/'EVIDENCE_MANIFEST.json'
        if not mf.is_file(): print('missing EVIDENCE_MANIFEST.json'); return 60
        raw=mf.read_bytes(); data=json.loads(raw); failures=[]
        for e in data.get('files',[]):
            p=root/e['name']
            if not p.is_file(): failures.append(f"missing {e['name']}"); continue
            digest=hashlib.sha256(p.read_bytes()).hexdigest()
            if digest!=e['sha256'] or p.stat().st_size!=e['size']: failures.append(f"content mismatch {e['name']}")
        sig=root/'EVIDENCE_MANIFEST.sig'
        if sig.is_file():
            key=a.public_key or (root/'SIGNING_PUBLIC_KEY.pem')
            if not key.is_file(): failures.append('signature present but no public key available')
            else:
                meta=parse_signature(sig)
                if meta['manifest_sha256']!=hashlib.sha256(raw).hexdigest(): failures.append('signature manifest digest mismatch')
                ok,msg=verify_ed25519(raw,__import__('base64').b64decode(meta['signature_base64']),key,a.openssl)
                if not ok: failures.append(msg)
        elif a.require_signature: failures.append('required signature missing')
        if not data.get('support_policy_passed'): failures.append('support policy not passed')
        print(json.dumps({'valid':not failures,'signed':sig.is_file(),'failures':failures},indent=2))
        return 0 if not failures else 60
if __name__=='__main__': raise SystemExit(main())
