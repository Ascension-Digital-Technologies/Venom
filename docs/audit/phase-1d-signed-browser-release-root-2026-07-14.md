# Phase 1D — Signed Browser Release Root

Date: 2026-07-14

## Result

Venom now provides a canonical Ed25519-signed release root for browser distributions. The root binds every shipped file and explicitly includes the build metadata hash, package hash, runtime/package versions, profile, security target, and protection-closure summary.

## Files

- `tools/dist_release_root.py` creates, signs, and verifies `VENOM_RELEASE_ROOT.json` and `VENOM_RELEASE_ROOT.sig`.
- `tools/publish_dist.py` is a fail-closed publication gate. It refuses unsigned, untrusted, stale, or modified distributions.
- `tests/security/test_dist_release_root.py` verifies valid signing, tamper rejection, signature verification, and unsigned-publication rejection.

## Signing

```powershell
python tools/dist_release_root.py dist --sign `
  --private-key C:\secure\venom-release-private.pem `
  --public-key C:\secure\venom-release-public.pem
```

## Verification

```powershell
python tools/dist_release_root.py dist --verify `
  --public-key C:\secure\venom-release-public.pem
```

## Publication

```powershell
python tools/publish_dist.py dist `
  --public-key C:\secure\venom-release-public.pem `
  --out build\protected-site-signed.zip
```

Publication fails unless the signature is present, trusted, and matches the exact current distribution.

## Security boundary

This authenticates the publisher and detects offline modification when verification is performed against a trusted public key. It does not prevent an attacker who controls the website origin from replacing both the application and an in-browser verifier. Trusted verification must therefore occur in a deployment pipeline, installer, browser extension, native launcher, or another independently trusted channel.
