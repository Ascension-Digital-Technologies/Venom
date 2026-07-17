# Update and Rollback Management

> **Applies to:** Venom 1.0.1

Venom release metadata supports signed update discovery, package verification, installation, and rollback. Stable channels must reject unsigned metadata and packages signed by an untrusted key.

## Update lifecycle

1. fetch the signed release-set metadata;
2. verify the detached Ed25519 signature with the trusted public key;
3. verify channel, sequence, version, checksums, and package identity;
4. download the selected platform package;
5. verify the package before installation;
6. install into a versioned location;
7. switch the active version only after verification;
8. retain the prior version for rollback.

## Rollback

Rollback should select a previously verified installed release. Do not reconstruct an older installation from partial files. Preserve the matching runtime, documentation, tools, and manifest as a complete package.

## Failure cases

Reject:

- unsigned stable metadata;
- incorrect public keys;
- invalid or missing signatures;
- lower or replayed release sequence numbers unless an explicit rollback policy permits them;
- mismatched checksums;
- incomplete packages;
- unexpected release channels.

## Verification testing

Before operating a production update channel, test a valid update, a tampered package, an unsigned manifest, a wrong signing key, and a rollback to the previous release.

See [Release signing](../security/release-signing.md) and [Release packaging](release-packaging.md).
