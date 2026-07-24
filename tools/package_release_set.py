#!/usr/bin/env python3
"""Create a canonical Venom release publication set from platform packages and compatibility evidence."""
from __future__ import annotations
import argparse, hashlib, json, os, shutil, sys, zipfile
from datetime import datetime, timezone
from pathlib import Path
from release_crypto import key_id_from_public_key, sign_ed25519, write_signature

SCHEMA = 1

def digest(path: Path) -> str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

def canonical(obj: object) -> bytes:
    return (json.dumps(obj, sort_keys=True, separators=(',', ':'), ensure_ascii=False)+'\n').encode()

def copy_unique(inputs: list[Path], out: Path) -> list[dict]:
    seen=set(); rows=[]
    for src in sorted((p.resolve() for p in inputs), key=lambda p:p.name):
        if not src.is_file(): raise SystemExit(f'input artifact not found: {src}')
        if src.name in seen: raise SystemExit(f'duplicate artifact filename: {src.name}')
        seen.add(src.name)
        dst=out/src.name; shutil.copy2(src,dst)
        rows.append({'name':src.name,'size':dst.stat().st_size,'sha256':digest(dst)})
    return rows

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--version', required=True)
    ap.add_argument('--source-revision', required=True)
    ap.add_argument('--packages', type=Path, nargs='+', required=True)
    ap.add_argument('--compatibility-evidence', type=Path)
    ap.add_argument('--verified-runtime', type=Path, help='canonical verified TurboJS/WASM artifact')
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--archive', type=Path, help='optional deterministic ZIP archive output')
    ap.add_argument('--source-date-epoch', type=int, default=None)
    ap.add_argument('--private-key', type=Path)
    ap.add_argument('--public-key', type=Path)
    ap.add_argument('--key-id')
    ap.add_argument('--openssl', default='openssl')
    ap.add_argument('--require-signature', action='store_true', help='fail unless both signing keys are supplied')
    args=ap.parse_args()
    if args.require_signature and (not args.private_key or not args.public_key):
        raise SystemExit('--require-signature requires --private-key and --public-key')
    if args.source_date_epoch is None:
        raw=os.environ.get('SOURCE_DATE_EPOCH','0')
        try: epoch=int(raw)
        except ValueError: raise SystemExit('SOURCE_DATE_EPOCH must be an integer')
    else: epoch=args.source_date_epoch
    if epoch < 0: raise SystemExit('--source-date-epoch must be non-negative')
    out=args.out.resolve()
    if out.exists(): shutil.rmtree(out)
    (out/'artifacts').mkdir(parents=True)
    packages=copy_unique(args.packages, out/'artifacts')
    evidence=None
    if args.compatibility_evidence:
        src=args.compatibility_evidence.resolve()
        if not src.is_file(): raise SystemExit(f'compatibility evidence not found: {src}')
        dst=out/'artifacts'/src.name
        if any(x['name']==src.name for x in packages): raise SystemExit(f'duplicate artifact filename: {src.name}')
        shutil.copy2(src,dst)
        evidence={'name':src.name,'size':dst.stat().st_size,'sha256':digest(dst)}
    runtime=None
    if args.verified_runtime:
        src=args.verified_runtime.resolve()
        if not src.is_file(): raise SystemExit(f'verified runtime not found: {src}')
        dst=out/'artifacts'/src.name
        if any(x['name']==src.name for x in packages) or (evidence and evidence['name']==src.name): raise SystemExit(f'duplicate artifact filename: {src.name}')
        shutil.copy2(src,dst)
        runtime={'name':src.name,'size':dst.stat().st_size,'sha256':digest(dst)}
    manifest={
      'schema_version':SCHEMA,
      'product':'Venom - Secure Web Runtime',
      'version':args.version,
      'source_revision':args.source_revision,
      'generated_utc':datetime.fromtimestamp(epoch,timezone.utc).replace(microsecond=0).isoformat(),
      'source_date_epoch':epoch,
      'policy':{
        'platform_packages_required':True,
        'compatibility_evidence_required':evidence is not None,
        'signature_required_for_stable':bool(args.require_signature),
        'verified_runtime_required':runtime is not None
      },
      'packages':packages,
      'compatibility_evidence':evidence,
      'verified_runtime':runtime
    }
    raw=canonical(manifest)
    (out/'RELEASE_SET.json').write_bytes(raw)
    checksum_lines=[f"{x['sha256']}  artifacts/{x['name']}" for x in packages]
    if evidence: checksum_lines.append(f"{evidence['sha256']}  artifacts/{evidence['name']}")
    if runtime: checksum_lines.append(f"{runtime['sha256']}  artifacts/{runtime['name']}")
    checksum_lines.append(f"{hashlib.sha256(raw).hexdigest()}  RELEASE_SET.json")
    (out/'SHA256SUMS').write_text('\n'.join(checksum_lines)+'\n',encoding='utf-8')
    if args.private_key or args.public_key:
        if not args.private_key or not args.public_key: raise SystemExit('signing requires both --private-key and --public-key')
        key_id=args.key_id or key_id_from_public_key(args.public_key,args.openssl)
        write_signature(out/'RELEASE_SET.sig', key_id=key_id, signature=sign_ed25519(raw,args.private_key,args.openssl), manifest=raw)
    if args.archive:
        archive=args.archive.resolve(); archive.parent.mkdir(parents=True,exist_ok=True)
        if archive.exists(): archive.unlink()
        dt=datetime.fromtimestamp(max(epoch,315532800),timezone.utc)
        stamp=(dt.year,dt.month,dt.day,dt.hour,dt.minute,dt.second)
        with zipfile.ZipFile(archive,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as zf:
            for path in sorted(out.rglob('*')):
                if not path.is_file(): continue
                rel=(Path('venom-release-set')/path.relative_to(out)).as_posix()
                info=zipfile.ZipInfo(rel,date_time=stamp); info.create_system=3; info.external_attr=(0o644&0xffff)<<16
                zf.writestr(info,path.read_bytes(),compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
        print(f'archive: {archive}')
    print(f'release set: {out}')
    print(f'packages: {len(packages)}')
    print(f'compatibility evidence: {"included" if evidence else "not included"}')
    print(f'verified runtime: {"included" if runtime else "not included"}')
    return 0
if __name__=='__main__': raise SystemExit(main())
