#!/usr/bin/env python3
"""Create, sign, and verify canonical browser-distribution release roots."""
from __future__ import annotations
import argparse, base64, hashlib, json, os, tempfile
from pathlib import Path
from release_crypto import (key_id_from_public_key, parse_signature,
                            public_key_from_private, sign_ed25519,
                            verify_ed25519, write_signature)

ROOT_NAME='VENOM_RELEASE_ROOT.json'
SIG_NAME='VENOM_RELEASE_ROOT.sig'
SCHEMA='venom-browser-release-root-v1'

def sha256_bytes(data: bytes) -> str: return hashlib.sha256(data).hexdigest()
def sha256_file(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def canonical_bytes(obj: dict) -> bytes:
    return (json.dumps(obj, sort_keys=True, separators=(',',':'), ensure_ascii=False)+'\n').encode('utf-8')

def locate_build_json(dist: Path) -> Path:
    candidates=[dist/'assets/app/build.json', dist/'assets/build.json']
    found=[p for p in candidates if p.is_file()]
    if len(found)!=1: raise SystemExit('expected exactly one assets/app/build.json or assets/build.json')
    return found[0]

def safe_files(dist: Path):
    for p in sorted(dist.rglob('*')):
        if not p.is_file(): continue
        rel=p.relative_to(dist).as_posix()
        if rel in {ROOT_NAME,SIG_NAME}: continue
        if p.is_symlink(): raise SystemExit(f'symlink is not allowed in signed dist: {rel}')
        yield rel,p

def make_root(dist: Path) -> dict:
    build_path=locate_build_json(dist)
    build=json.loads(build_path.read_text(encoding='utf-8'))
    closure=build.get('protection_closure')
    if not isinstance(closure,dict): raise SystemExit('build.json lacks protection_closure')
    for k in ('requested','resolved','expected_turbojs_records'):
        if not isinstance(closure.get(k),int) or closure[k] < 0: raise SystemExit(f'invalid protection_closure.{k}')
    if closure['requested'] != closure['resolved']: raise SystemExit('unresolved protected intents cannot be signed')
    files=[{'path':rel,'size':p.stat().st_size,'sha256':sha256_file(p)} for rel,p in safe_files(dist)]
    if not files: raise SystemExit('distribution contains no signable files')
    return {'schema':SCHEMA,'version':1,'product':build.get('product','Venom'),
            'venom_version':build.get('venom_version','unknown'),
            'profile':build.get('profile','unknown'),
            'security_target':build.get('security_target','unknown'),
            'build_metadata_path':build_path.relative_to(dist).as_posix(),
            'build_metadata_sha256':sha256_file(build_path),
            'package_sha256':build.get('package_sha256',''),
            'package_asset':build.get('package_asset',''),
            'runtime_abi_version':build.get('runtime_abi_version'),
            'package_format_version':build.get('package_format_version'),
            'protection_closure':closure,'files':files}

def write_root(dist: Path) -> bytes:
    data=canonical_bytes(make_root(dist)); (dist/ROOT_NAME).write_bytes(data); return data

def sign(dist: Path, private: Path, public: Path|None, openssl: str):
    data=write_root(dist)
    with tempfile.TemporaryDirectory(prefix='venom-dist-sign-') as td:
        pub=public or Path(td)/'public.pem'
        if public is None: public_key_from_private(private,pub,openssl)
        key_id=key_id_from_public_key(pub,openssl)
        signature=sign_ed25519(data,private,openssl)
        write_signature(dist/SIG_NAME,key_id=key_id,signature=signature,manifest=data)
    return key_id

def verify(dist: Path, public: Path, openssl: str, require_signature=True) -> list[str]:
    failures=[]
    root_path=dist/ROOT_NAME; sig_path=dist/SIG_NAME
    if not root_path.is_file(): return [f'missing {ROOT_NAME}']
    try:
        raw=root_path.read_bytes(); obj=json.loads(raw)
        if raw != canonical_bytes(obj): failures.append('release root is not canonical JSON')
        if obj.get('schema') != SCHEMA: failures.append('unsupported release-root schema')
        expected=make_root(dist)
        if obj != expected: failures.append('release root does not match current distribution')
    except Exception as exc: return [f'invalid release root: {exc}']
    if not sig_path.is_file():
        if require_signature: failures.append(f'missing {SIG_NAME}')
        return failures
    try:
        meta=parse_signature(sig_path)
        if meta['manifest_sha256'] != sha256_bytes(raw): failures.append('signature metadata root hash mismatch')
        if meta['key_id'] != key_id_from_public_key(public,openssl): failures.append('signature key_id is not trusted key')
        sig=base64.b64decode(meta['signature_base64'],validate=True)
        ok,detail=verify_ed25519(raw,sig,public,openssl)
        if not ok: failures.append('Ed25519 verification failed: '+detail)
    except Exception as exc: failures.append(f'invalid signature: {exc}')
    return failures

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('dist',type=Path)
    ap.add_argument('--sign',action='store_true'); ap.add_argument('--verify',action='store_true')
    ap.add_argument('--private-key',type=Path); ap.add_argument('--public-key',type=Path)
    ap.add_argument('--openssl',default='openssl'); ap.add_argument('--allow-unsigned',action='store_true')
    ns=ap.parse_args(); dist=ns.dist.resolve()
    if not dist.is_dir(): raise SystemExit('dist is not a directory')
    if ns.sign:
        if not ns.private_key: raise SystemExit('--sign requires --private-key')
        kid=sign(dist,ns.private_key,ns.public_key,ns.openssl); print(f'signed browser release root key_id={kid}')
    elif ns.verify:
        if not ns.public_key: raise SystemExit('--verify requires --public-key')
        failures=verify(dist,ns.public_key,ns.openssl,not ns.allow_unsigned)
        if failures:
            for f in failures: print('[FAIL]',f)
            raise SystemExit(1)
        print('[PASS] browser release root and Ed25519 signature verified')
    else:
        write_root(dist); print(f'wrote {ROOT_NAME} (unsigned; not publishable)')
if __name__=='__main__': main()
