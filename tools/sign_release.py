#!/usr/bin/env python3
"""Offline-sign an existing Venom RELEASE_MANIFEST.txt with an Ed25519 key."""
from __future__ import annotations
import argparse
from pathlib import Path
from release_crypto import key_id_from_public_key, public_key_from_private, sign_ed25519, write_signature

def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('prod',type=Path)
    ap.add_argument('--private-key',type=Path,required=True)
    ap.add_argument('--public-key',type=Path)
    ap.add_argument('--key-id')
    ap.add_argument('--openssl',default='openssl')
    a=ap.parse_args(); root=a.release.resolve(); manifest=root/'RELEASE_MANIFEST.txt'
    if not manifest.is_file(): raise SystemExit('missing RELEASE_MANIFEST.txt')
    pub=a.public_key.resolve() if a.public_key else root/'.venom-signing-public.pem'
    generated=not a.public_key
    if generated: public_key_from_private(a.private_key.resolve(),pub,a.openssl)
    key_id=a.key_id or key_id_from_public_key(pub,a.openssl)
    raw=manifest.read_bytes(); write_signature(root/'RELEASE_MANIFEST.sig',key_id=key_id,signature=sign_ed25519(raw,a.private_key.resolve(),a.openssl),manifest=raw)
    if generated: pub.unlink(missing_ok=True)
    print(f'signed release manifest with Ed25519 key_id={key_id}')
    return 0
if __name__=='__main__': raise SystemExit(main())
