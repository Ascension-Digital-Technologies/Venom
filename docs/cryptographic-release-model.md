# Cryptographic release model — v0.93.0

Venom production releases use a detached Ed25519 signature over a domain-separated message containing both the SHA-256 digest and exact bytes of `RELEASE_MANIFEST.txt`.

## Trust and rotation

Each signature records a 128-bit printable key identifier derived from the first 32 hexadecimal characters of the SHA-256 digest of the public key's DER encoding. Verification accepts either one explicitly pinned public key or a trusted-key directory containing `<key-id>.pem`. This permits overlap during rotation without trusting an arbitrary key embedded in the release.

## Rollback policy

The manifest records `release_sequence` and `release_channel`. Deployments retain a minimum accepted sequence and invoke verification with `--min-release-sequence`. A correctly signed older release is rejected once the floor advances.

## Offline signing

Generate and retain the Ed25519 private key on an offline signing system:

```sh
openssl genpkey -algorithm ED25519 -out release-private.pem
openssl pkey -in release-private.pem -pubout -out release-public.pem
python tools/sign_release.py dist-release --private-key release-private.pem --public-key release-public.pem
```

The private key is never copied into a release folder. Production packaging can sign directly with `--sign ed25519`, but the preferred operational flow is to generate the unsigned manifest in CI, transfer its digest to the offline signer, and return only `RELEASE_MANIFEST.sig`.

## Verification

```sh
python tools/verify_release.py dist-release \
  --strict-signature \
  --public-key release-public.pem \
  --min-release-sequence 90 \
  --expect-channel stable
```

For key rotation, place trusted public keys in a directory named by fingerprint and use `--trusted-keys`.

The legacy `dev-sha256` mode remains available only for portable tests. It is explicitly not a production signature.
