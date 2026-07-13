#!/usr/bin/env python3
"""Verify a Venom release publication set, including optional Ed25519 signature."""
from __future__ import annotations
import argparse, hashlib, json, tempfile, zipfile
from pathlib import Path
from release_crypto import key_id_from_public_key, parse_signature, verify_ed25519

def digest(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def safe_extract(src: Path, root: Path) -> Path:
    with zipfile.ZipFile(src) as z:
        names=set()
        for info in z.infolist():
            if info.filename in names: raise SystemExit(f'duplicate archive entry: {info.filename}')
            names.add(info.filename)
            target=(root/info.filename).resolve()
            if not target.is_relative_to(root.resolve()): raise SystemExit(f'unsafe archive path: {info.filename}')
        z.extractall(root)
    matches=list(root.rglob('RELEASE_SET.json'))
    if len(matches)!=1: raise SystemExit('archive must contain exactly one RELEASE_SET.json')
    return matches[0].parent

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('release_set',type=Path)
    ap.add_argument('--public-key',type=Path)
    ap.add_argument('--require-signature',action='store_true')
    ap.add_argument('--require-evidence',action='store_true')
    ap.add_argument('--require-runtime',action='store_true')
    ap.add_argument('--expect-version')
    ap.add_argument('--openssl',default='openssl')
    args=ap.parse_args(); temp=None
    root=args.release_set.resolve()
    if root.is_file():
        temp=tempfile.TemporaryDirectory(prefix='venom-release-set-'); root=safe_extract(root,Path(temp.name))
    manifest_path=root/'RELEASE_SET.json'
    try: obj=json.loads(manifest_path.read_text(encoding='utf-8'))
    except Exception as e: raise SystemExit(f'invalid RELEASE_SET.json: {e}')
    errors=[]
    if obj.get('schema_version')!=1: errors.append('unsupported release-set schema')
    if obj.get('product')!='Venom - Secure Web Runtime': errors.append('unexpected product identity')
    if args.expect_version and obj.get('version')!=args.expect_version: errors.append('version mismatch')
    rows=list(obj.get('packages') or [])
    if not rows: errors.append('no platform packages listed')
    evidence=obj.get('compatibility_evidence')
    if args.require_evidence and not evidence: errors.append('compatibility evidence is required but absent')
    runtime=obj.get('verified_runtime')
    if args.require_runtime and not runtime: errors.append('verified runtime is required but absent')
    expected=set()
    extra=[]
    if evidence: extra.append(evidence)
    if runtime: extra.append(runtime)
    for row in rows+extra:
        if not isinstance(row,dict): errors.append('malformed artifact row'); continue
        name=row.get('name','')
        if not name or '/' in name or '\\' in name or name in expected: errors.append(f'unsafe or duplicate artifact name: {name!r}'); continue
        expected.add(name); p=root/'artifacts'/name
        if not p.is_file(): errors.append(f'missing artifact: {name}'); continue
        if p.stat().st_size!=row.get('size'): errors.append(f'size mismatch: {name}')
        if digest(p)!=row.get('sha256'): errors.append(f'sha256 mismatch: {name}')
    actual={p.name for p in (root/'artifacts').iterdir() if p.is_file()} if (root/'artifacts').is_dir() else set()
    for name in sorted(actual-expected): errors.append(f'unmanifested artifact: {name}')
    sig=root/'RELEASE_SET.sig'
    if args.require_signature and not sig.is_file(): errors.append('required RELEASE_SET.sig is missing')
    if sig.is_file():
        if not args.public_key: errors.append('signature present but no --public-key supplied')
        else:
            try:
                data=parse_signature(sig); raw=manifest_path.read_bytes()
                if data['manifest_sha256']!=hashlib.sha256(raw).hexdigest(): errors.append('signature manifest digest mismatch')
                actual_id=key_id_from_public_key(args.public_key,args.openssl)
                if data['key_id']!=actual_id: errors.append('signature key id mismatch')
                import base64
                ok,msg=verify_ed25519(raw,base64.b64decode(data['signature_base64'],validate=True),args.public_key,args.openssl)
                if not ok: errors.append(f'signature verification failed: {msg}')
            except Exception as e: errors.append(f'invalid signature: {e}')
    if errors:
        for e in errors: print(f'[venom] release-set verification failed: {e}')
        return 60
    print(f"[venom] release set verified: version={obj.get('version')} packages={len(rows)} evidence={'yes' if evidence else 'no'} runtime={'yes' if runtime else 'no'}")
    return 0
if __name__=='__main__': raise SystemExit(main())
