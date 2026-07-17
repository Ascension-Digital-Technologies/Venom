# Release engineering

Venom 1.5.0 release packages are deterministic and self-describing. Every stable package contains:

- `RELEASE_MANIFEST.txt`: SHA-256 and size for every shipped file.
- `RELEASE_MANIFEST.sig`: optional Ed25519 signature over the manifest.
- `SBOM.cdx.json`: CycloneDX 1.5 inventory with deterministic hashes for all vendored dependency trees.
- `PROVENANCE.intoto.json`: in-toto/SLSA provenance covering the compiler, QuickJS/WASM runtime, product contracts, and toolchain lock.
- `RELEASE_POLICY.json`: machine-readable target, ABI, security, release-channel, anti-rollback sequence, and source-revision policy.
- `CONTRACTS.json`: stable public product contract manifest.

## Reproducibility

Set `SOURCE_DATE_EPOCH` and package with `tools/package_release.py`. ZIP and tar archives normalize timestamps, ownership, permissions, ordering, and compression settings.

## Signing

Production releases use an offline Ed25519 private key. Verification requires the corresponding trusted public key and checks the manifest digest before verifying the signature. Stable releases must include supply-chain metadata and may not enable browser host-source fallback.

## Verification

```bash
python tools/verify_release.py <release-or-archive> \
  --expect-version 1.5.0 \
  --expect-channel stable \
  --require-supply-chain-metadata
```

Add `--public-key <trusted.pem> --strict-signature` for signed production artifacts.
