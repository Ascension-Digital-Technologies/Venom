# Release publication sets

A Venom GitHub release can contain several platform packages plus browser compatibility evidence. Version 1.12.0 binds those files into one canonical **release publication set**.

The set contains:

- `RELEASE_SET.json` — canonical manifest with product version, source revision, policy, sizes, and SHA-256 digests;
- `SHA256SUMS` — human- and tool-friendly checksums;
- `artifacts/` — Windows, Linux, and macOS release packages plus optional compatibility evidence;
- `RELEASE_SET.sig` — optional Ed25519 signature over the exact manifest.

## Create a publication set

```bash
scripts/package-release-set.sh \
  --version 1.12.0 \
  --source-revision "$GIT_COMMIT" \
  --packages venom-v1.12.0-linux-x86_64.zip venom-v1.12.0-windows-amd64.zip venom-v1.12.0-darwin-arm64.zip \
  --compatibility-evidence venom-compatibility-evidence.zip \
  --out release-set \
  --source-date-epoch "$SOURCE_DATE_EPOCH"
```

Add `--private-key` and `--public-key` to produce an Ed25519 signature. Keep the private key outside the repository and ordinary CI artifacts.

## Verify a publication set

```bash
scripts/verify-release-set.sh release-set \
  --expect-version 1.12.0 \
  --require-evidence
```

For a signed stable release:

```bash
scripts/verify-release-set.sh release-set \
  --expect-version 1.12.0 \
  --require-evidence \
  --require-signature \
  --public-key release-public.pem
```

Verification rejects missing, changed, duplicated, unmanifested, or path-unsafe artifacts. A valid signature authenticates the manifest and therefore every digest bound by it. It does not independently prove that CI executed honestly; release trust still depends on the signing-key policy and trusted build identity.
