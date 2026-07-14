# Release signing

Stable release sets are signed with Ed25519. Publication requires protected private/public key material, creates `RELEASE_SET.json` and `RELEASE_SET.sig`, and verifies the signature before upload. Unsigned stable metadata is rejected.

Signing keys must never be committed. Store them in protected CI secrets or an external signing system. See the detailed [release-signing reference](../release-signing.md).
