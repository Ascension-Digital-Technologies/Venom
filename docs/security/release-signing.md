# Release Signing

> **Applies to:** Venom 1.1.0

Stable Venom release sets use Ed25519 signatures to bind published metadata to an authorized release key. Checksums alone do not authenticate a manifest when an attacker can replace both the package and its checksum.

## Key material

Maintain:

- a protected Ed25519 private key used only by the release environment;
- the corresponding public key distributed to verification clients;
- a stable key identifier recorded in release metadata.

Never commit the private key. Store it in a protected CI environment or offline signing system with approval controls.

## Signed release set

A stable release includes:

```text
RELEASE_SET.json
RELEASE_SET.sig
```

The manifest identifies the release channel, version, sequence, packages, checksums, provenance, and policy. Verification must authenticate the signature before trusting any package metadata.

## Verification behavior

Reject missing signatures, unknown keys, malformed signatures, altered manifests, mismatched package hashes, and replayed or disallowed release sequences.

## Key rotation

Introduce a new key through an authenticated transition. Keep old public keys available for verifying historical packages, and document the activation sequence for the replacement key.

See [Release packaging](../operations/release-packaging.md) and [Update management](../operations/update-management.md).
