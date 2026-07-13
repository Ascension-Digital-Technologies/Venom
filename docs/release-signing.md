# Release signing and verification — v0.93.0

v0.88.0 adds an explicit release integrity layer on top of the existing `dist-release/` packager.
The release folder now has a SHA-256 manifest verifier, optional manifest signatures, and archive verification before a release zip/tarball is considered complete.

## Files

```txt
dist-release/
  venom[.exe]
  README.md
  VERSION.txt
  RELEASE_MANIFEST.txt
  RELEASE_MANIFEST.sig       # optional, present when --sign is used
  SOURCE_MANIFEST.txt
  docs/
  examples/
  scripts/
  tools/
    verify_release.py
  licenses/
```

`RELEASE_MANIFEST.txt` lists every shipped file except the manifest itself and the optional signature file. Each row records:

```txt
sha256  byte-size  relative/path
```

The verifier rejects missing files, byte-size mismatches, SHA-256 mismatches, unsafe manifest paths, and unmanifested files.

## Package a signed local smoke release

```bash
scripts/package-release.sh \
  --venom build/venom \
  --out dist-release \
  --archive zip \
  --sign dev-sha256 \
  --dev-insecure-key local-smoke-key
```

The `dev-sha256` mode is deliberately labeled development-only. It is useful for reproducible CI smoke tests without external dependencies, but it is not a production signing scheme.

## Package a production-style OpenSSL signed release

Use an offline private key and publish the matching public key through your normal release channel.

```bash
scripts/package-release.sh \
  --venom build/venom \
  --out dist-release \
  --archive zip \
  --sign openssl \
  --private-key /secure/offline/venom-release-private.pem \
  --public-key /secure/offline/venom-release-public.pem
```

The packager signs `RELEASE_MANIFEST.txt` into `RELEASE_MANIFEST.sig` and then runs the verifier on both the release directory and the generated archive.

## Verify a release directory or archive

```bash
scripts/verify-release.sh dist-release --expect-version 0.88.0
scripts/verify-release.sh venom-v0.88.0-linux-x86_64.zip --expect-version 0.88.0
```

For strict signature verification:

```bash
scripts/verify-release.sh dist-release \
  --expect-version 0.88.0 \
  --strict-signature \
  --public-key /path/to/venom-release-public.pem
```

For development smoke signatures only:

```bash
scripts/verify-release.sh dist-release \
  --expect-version 0.88.0 \
  --strict-signature \
  --dev-insecure-key local-smoke-key
```

## Security boundary

Release signing verifies that the binary release folder/archive matches the manifest that was signed. It does not make browser-side protected packages secret, and it does not flip the QuickJS/WASM real-engine gate. The existing honest runtime status remains unchanged until a verified upstream QuickJS WASM artifact is embedded.


## v0.90 production signature

Production releases use the Ed25519 V2 envelope, explicit key fingerprints, trusted-key rotation, and monotonic release-sequence rollback checks. See `docs/cryptographic-release-model.md`. Generic OpenSSL RSA/ECDSA manifest signing is no longer the production contract.
