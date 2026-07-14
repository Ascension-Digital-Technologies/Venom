#!/usr/bin/env python3
"""Verify a Venom binary release directory or release archive."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path
from release_crypto import parse_signature, verify_ed25519, key_id_from_public_key

MANIFEST_NAME = 'RELEASE_MANIFEST.txt'
SIGNATURE_NAME = 'RELEASE_MANIFEST.sig'


@dataclass(frozen=True)
class ManifestEntry:
    digest: str
    size: int
    relpath: str


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def dev_signature(manifest_bytes: bytes, key: str) -> str:
    # Deliberately labeled dev-insecure: useful for portable smoke tests and local dry runs,
    # but production releases should use --sign openssl with an offline private key.
    h = hashlib.sha256()
    h.update(b'VENOM_DEV_RELEASE_SIGNATURE_V1\0')
    h.update(key.encode('utf-8'))
    h.update(b'\0')
    h.update(manifest_bytes)
    return h.hexdigest()


def parse_manifest(text: str) -> tuple[dict[str, str], list[ManifestEntry]]:
    lines = text.splitlines()
    if not lines or lines[0].strip() != 'VENOM_RELEASE_MANIFEST_V1':
        raise ValueError('manifest header is not VENOM_RELEASE_MANIFEST_V1')
    meta: dict[str, str] = {}
    entries: list[ManifestEntry] = []
    in_files = False
    for raw in lines[1:]:
        line = raw.rstrip('\n')
        if not line:
            continue
        if line == 'files:':
            in_files = True
            continue
        if not in_files:
            if '=' in line:
                k, v = line.split('=', 1)
                meta[k.strip()] = v.strip()
            continue
        parts = line.split(None, 2)
        if len(parts) != 3:
            raise ValueError(f'malformed manifest file row: {line!r}')
        digest, size_text, relpath = parts
        if len(digest) != 64 or any(c not in '0123456789abcdefABCDEF' for c in digest):
            raise ValueError(f'malformed sha256 digest for {relpath!r}')
        try:
            size = int(size_text)
        except ValueError as exc:
            raise ValueError(f'malformed size for {relpath!r}') from exc
        if relpath.startswith('/') or '..' in Path(relpath).parts:
            raise ValueError(f'unsafe manifest path: {relpath!r}')
        entries.append(ManifestEntry(digest.lower(), size, relpath))
    if not entries:
        raise ValueError('manifest contains no file entries')
    return meta, entries


def find_release_root(path: Path) -> Path:
    if (path / MANIFEST_NAME).is_file():
        return path
    matches = [p.parent for p in path.rglob(MANIFEST_NAME)] if path.is_dir() else []
    if len(matches) == 1:
        return matches[0]
    if not matches:
        raise SystemExit(f'could not find {MANIFEST_NAME} under {path}')
    raise SystemExit('multiple release manifests found; pass the exact release directory')


def extract_archive(archive: Path, temp_root: Path) -> Path:
    out = temp_root / 'prod'
    out.mkdir()
    name = archive.name.lower()
    if name.endswith('.zip'):
        with zipfile.ZipFile(archive) as zf:
            for info in zf.infolist():
                target = out / info.filename
                if not target.resolve().is_relative_to(out.resolve()):
                    raise SystemExit(f'archive contains unsafe path: {info.filename!r}')
            zf.extractall(out)
    elif name.endswith('.tar.gz') or name.endswith('.tgz'):
        with tarfile.open(archive, 'r:gz') as tf:
            root_resolved = out.resolve()
            for member in tf.getmembers():
                target = (out / member.name).resolve()
                if not target.is_relative_to(root_resolved):
                    raise SystemExit(f'archive contains unsafe path: {member.name!r}')
            tf.extractall(out)
    else:
        raise SystemExit('unsupported archive format; expected .zip, .tar.gz, or .tgz')
    return find_release_root(out)


def verify_manifest_files(release_root: Path, entries: list[ManifestEntry]) -> list[str]:
    failures: list[str] = []
    expected_paths = {e.relpath for e in entries}
    for entry in entries:
        path = release_root / entry.relpath
        if not path.is_file():
            failures.append(f'missing file listed in manifest: {entry.relpath}')
            continue
        actual_size = path.stat().st_size
        if actual_size != entry.size:
            failures.append(f'size mismatch for {entry.relpath}: expected {entry.size}, got {actual_size}')
        actual_digest = sha256(path)
        if actual_digest != entry.digest:
            failures.append(f'sha256 mismatch for {entry.relpath}: expected {entry.digest}, got {actual_digest}')
    for path in sorted(release_root.rglob('*')):
        if not path.is_file():
            continue
        rel = path.relative_to(release_root).as_posix()
        if rel in {MANIFEST_NAME, SIGNATURE_NAME}:
            continue
        if rel not in expected_paths:
            failures.append(f'unmanifested file present: {rel}')
    return failures


def verify_supply_chain_metadata(release_root: Path, meta: dict[str, str], entries: list[ManifestEntry]) -> list[str]:
    failures: list[str] = []
    required = ('SBOM.cdx.json', 'PROVENANCE.intoto.json', 'toolchains.lock.json')
    manifest_paths = {e.relpath for e in entries}
    for name in required:
        if name not in manifest_paths:
            failures.append(f'supply-chain metadata is not manifested: {name}')
        path = release_root / name
        if not path.is_file():
            failures.append(f'missing supply-chain metadata: {name}')
    try:
        sbom = json.loads((release_root/'SBOM.cdx.json').read_text(encoding='utf-8'))
        if sbom.get('bomFormat') != 'CycloneDX': failures.append('SBOM is not CycloneDX')
        if sbom.get('metadata',{}).get('component',{}).get('version') != meta.get('version'): failures.append('SBOM version does not match release manifest')
    except Exception as exc:
        failures.append(f'invalid SBOM.cdx.json: {exc}')
    try:
        provenance = json.loads((release_root/'PROVENANCE.intoto.json').read_text(encoding='utf-8'))
        if provenance.get('_type') != 'https://in-toto.io/Statement/v1': failures.append('provenance statement type is invalid')
        if provenance.get('predicateType') != 'https://slsa.dev/provenance/v1': failures.append('provenance predicate type is invalid')
        params=provenance.get('predicate',{}).get('buildDefinition',{}).get('externalParameters',{})
        if params.get('version') != meta.get('version'): failures.append('provenance version does not match release manifest')
    except Exception as exc:
        failures.append(f'invalid PROVENANCE.intoto.json: {exc}')
    if 'source_date_epoch' not in meta: failures.append('release manifest is missing source_date_epoch')
    return failures

def verify_dev_signature(release_root: Path, key: str) -> tuple[bool, str]:
    manifest = release_root / MANIFEST_NAME
    sig = release_root / SIGNATURE_NAME
    if not sig.is_file():
        return False, 'missing RELEASE_MANIFEST.sig'
    lines = [line.strip() for line in sig.read_text(encoding='utf-8').splitlines() if line.strip()]
    data = dict(line.split('=', 1) for line in lines if '=' in line)
    if data.get('type') != 'dev-sha256':
        return False, f"signature type is {data.get('type')!r}, expected 'dev-sha256'"
    expected = dev_signature(manifest.read_bytes(), key)
    if data.get('digest') != expected:
        return False, 'dev signature digest mismatch'
    return True, 'dev-sha256 signature ok'


def resolve_trusted_key(key_id: str, public_key: Path | None, trusted_keys: Path | None, openssl: str) -> tuple[Path | None, str]:
    if public_key:
        path = public_key.resolve()
        if not path.is_file(): return None, f'public key not found: {path}'
        actual = key_id_from_public_key(path, openssl)
        if actual != key_id: return None, f'public key fingerprint mismatch: signature key_id={key_id}, supplied key_id={actual}'
        return path, ''
    if trusted_keys:
        base = trusted_keys.resolve()
        for suffix in ('.pem', '.pub.pem', ''):
            candidate = base / (key_id + suffix)
            if candidate.is_file():
                actual = key_id_from_public_key(candidate, openssl)
                if actual != key_id: return None, f'trusted key filename/fingerprint mismatch for {candidate.name}'
                return candidate, ''
        return None, f'unknown release signing key: {key_id}'
    return None, 'no Ed25519 public key or trusted-key directory configured'


def verify_release_signature(release_root: Path, public_key: Path | None, trusted_keys: Path | None, openssl: str = 'openssl') -> tuple[bool, str, str | None]:
    manifest = release_root / MANIFEST_NAME
    sig_path = release_root / SIGNATURE_NAME
    if not sig_path.is_file(): return False, 'missing RELEASE_MANIFEST.sig', None
    try:
        data = parse_signature(sig_path)
    except Exception as exc:
        return False, f'invalid signature metadata: {exc}', None
    raw = manifest.read_bytes()
    actual_digest = hashlib.sha256(raw).hexdigest()
    if data['manifest_sha256'] != actual_digest: return False, 'signature metadata manifest digest mismatch', data.get('key_id')
    key, error = resolve_trusted_key(data['key_id'], public_key, trusted_keys, openssl)
    if not key: return False, error, data['key_id']
    try:
        signature = __import__('base64').b64decode(data['signature_base64'], validate=True)
    except Exception:
        return False, 'signature_base64 is malformed', data['key_id']
    ok, message = verify_ed25519(raw, signature, key, openssl)
    return ok, message, data['key_id']

def verify_release(path: Path, args: argparse.Namespace) -> int:
    temp: tempfile.TemporaryDirectory[str] | None = None
    try:
        release_root = path.resolve()
        if path.is_file():
            temp = tempfile.TemporaryDirectory(prefix='venom-release-verify-')
            release_root = extract_archive(path.resolve(), Path(temp.name))
        else:
            release_root = find_release_root(path.resolve())

        manifest_path = release_root / MANIFEST_NAME
        manifest_text = manifest_path.read_text(encoding='utf-8')
        meta, entries = parse_manifest(manifest_text)
        failures = verify_manifest_files(release_root, entries)
        if args.require_supply_chain_metadata:
            failures.extend(verify_supply_chain_metadata(release_root, meta, entries))

        if args.expect_version and meta.get('version') != args.expect_version:
            failures.append(f"version mismatch: expected {args.expect_version}, got {meta.get('version')!r}")
        try:
            release_sequence = int(meta.get('release_sequence', '-1'))
        except ValueError:
            release_sequence = -1
            failures.append('manifest release_sequence is not an integer')
        if args.min_release_sequence is not None and release_sequence < args.min_release_sequence:
            failures.append(f'rollback rejected: release_sequence {release_sequence} is below minimum {args.min_release_sequence}')
        if args.expect_channel and meta.get('release_channel') != args.expect_channel:
            failures.append(f"release channel mismatch: expected {args.expect_channel}, got {meta.get('release_channel')!r}")

        signature_checked = False
        if args.dev_insecure_key:
            ok, message = verify_dev_signature(release_root, args.dev_insecure_key)
            signature_checked = True
            if not ok:
                failures.append(message)
        signing_key_id = None
        if args.public_key or args.trusted_keys:
            ok, message, signing_key_id = verify_release_signature(release_root, args.public_key, args.trusted_keys, args.openssl)
            signature_checked = True
            if not ok: failures.append(message)
        if args.strict_signature and not signature_checked:
            failures.append('strict signature requested but no signature verifier was configured')
        if args.strict_signature and not (release_root / SIGNATURE_NAME).is_file():
            failures.append('strict signature requested but RELEASE_MANIFEST.sig is missing')

        if failures:
            print('release verification: FAIL', file=sys.stderr)
            for failure in failures:
                print('failure: ' + failure, file=sys.stderr)
            return 1

        print('release verification: PASS')
        print(f'release_root: {release_root}')
        print(f'version: {meta.get("version", "unknown")}')
        print(f'files_verified: {len(entries)}')
        print(f'release_sequence: {release_sequence}')
        print(f'release_channel: {meta.get("release_channel", "unknown")}')
        if signature_checked:
            print('signature: verified')
            if signing_key_id: print(f'signing_key_id: {signing_key_id}')
        elif (release_root / SIGNATURE_NAME).is_file():
            print('signature: present-not-verified')
        else:
            print('signature: absent')
        return 0
    finally:
        if temp is not None:
            temp.cleanup()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('release', type=Path, help='release directory, .zip, or .tar.gz archive')
    ap.add_argument('--expect-version', default=None)
    ap.add_argument('--strict-signature', action='store_true', help='fail unless a configured signature verifier succeeds')
    ap.add_argument('--dev-insecure-key', default=None, help='verify dev-sha256 smoke-test signature; not for production releases')
    ap.add_argument('--public-key', type=Path, default=None, help='specific trusted Ed25519 public key PEM')
    ap.add_argument('--trusted-keys', type=Path, default=None, help='directory of trusted Ed25519 keys named <key-id>.pem')
    ap.add_argument('--min-release-sequence', type=int, default=None, help='reject cryptographically valid rollback releases below this sequence')
    ap.add_argument('--expect-channel', choices=['development','preview','stable'], default=None)
    ap.add_argument('--openssl', default=os.environ.get('OPENSSL', 'openssl'))
    ap.add_argument('--require-supply-chain-metadata', action='store_true', help='require and validate SBOM, provenance, toolchain lock, and reproducible timestamp metadata')
    args = ap.parse_args()
    return verify_release(args.release, args)


if __name__ == '__main__':
    raise SystemExit(main())
