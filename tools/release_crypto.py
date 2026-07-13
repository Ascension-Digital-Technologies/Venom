#!/usr/bin/env python3
"""Shared Ed25519 release-signing helpers for Venom."""
from __future__ import annotations
import base64, hashlib, subprocess, tempfile
from pathlib import Path

SIGNATURE_HEADER = 'VENOM_RELEASE_SIGNATURE_V2'
DOMAIN = b'VENOM_RELEASE_MANIFEST_ED25519_V1\0'

def signing_message(manifest: bytes) -> bytes:
    return DOMAIN + hashlib.sha256(manifest).digest() + manifest

def key_id_from_public_key(public_key: Path, openssl: str='openssl') -> str:
    der = subprocess.run([openssl,'pkey','-pubin','-in',str(public_key),'-outform','DER'],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30).stdout
    return hashlib.sha256(der).hexdigest()[:32]

def public_key_from_private(private_key: Path, out: Path, openssl: str='openssl') -> None:
    subprocess.run([openssl,'pkey','-in',str(private_key),'-pubout','-out',str(out)],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)

def sign_ed25519(manifest: bytes, private_key: Path, openssl: str='openssl') -> bytes:
    with tempfile.TemporaryDirectory(prefix='venom-sign-') as td:
        msg=Path(td)/'message.bin'; sig=Path(td)/'signature.bin'; msg.write_bytes(signing_message(manifest))
        r=subprocess.run([openssl,'pkeyutl','-sign','-rawin','-inkey',str(private_key),'-in',str(msg),'-out',str(sig)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
        if r.returncode: raise RuntimeError('Ed25519 signing failed: '+r.stdout.strip())
        return sig.read_bytes()

def verify_ed25519(manifest: bytes, signature: bytes, public_key: Path, openssl: str='openssl') -> tuple[bool,str]:
    with tempfile.TemporaryDirectory(prefix='venom-verify-') as td:
        msg=Path(td)/'message.bin'; sig=Path(td)/'signature.bin'; msg.write_bytes(signing_message(manifest)); sig.write_bytes(signature)
        r=subprocess.run([openssl,'pkeyutl','-verify','-rawin','-pubin','-inkey',str(public_key),'-in',str(msg),'-sigfile',str(sig)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=30)
        return r.returncode == 0, (r.stdout.strip() or ('Ed25519 signature ok' if r.returncode == 0 else 'Ed25519 signature rejected'))

def write_signature(path: Path, *, key_id: str, signature: bytes, manifest: bytes) -> None:
    path.write_text('\n'.join([SIGNATURE_HEADER,'algorithm=ed25519',f'key_id={key_id}',f'manifest_sha256={hashlib.sha256(manifest).hexdigest()}',f'signature_base64={base64.b64encode(signature).decode("ascii")}','']) ,encoding='utf-8')

def parse_signature(path: Path) -> dict[str,str]:
    lines=[x.strip() for x in path.read_text(encoding='utf-8').splitlines() if x.strip()]
    if not lines or lines[0] != SIGNATURE_HEADER: raise ValueError('signature header is not '+SIGNATURE_HEADER)
    data={}
    for line in lines[1:]:
        if '=' not in line: raise ValueError('malformed signature metadata')
        k,v=line.split('=',1)
        if k in data: raise ValueError('duplicate signature field: '+k)
        data[k]=v
    for key in ('algorithm','key_id','manifest_sha256','signature_base64'):
        if not data.get(key): raise ValueError('missing signature field: '+key)
    if data['algorithm'] != 'ed25519': raise ValueError('unsupported signature algorithm: '+data['algorithm'])
    return data
