# Update management

Venom supports explicit stable and preview update channels. Updates are selected from a release-set manifest, verified by SHA-256, optionally verified with Ed25519 metadata, checked against `CONTRACTS.json`, installed atomically, and recorded for rollback.

```powershell
venom update check --channel stable --manifest https://example/releases/RELEASE_SET.json
venom update install --channel stable --manifest https://example/releases/RELEASE_SET.json --require-signature --public-key venom-release.pub --yes
venom update status
venom update rollback
```

The updater rejects HTTP URLs, mismatched package digests, incompatible stable contracts, preview builds on the stable channel, and unsigned metadata when signature enforcement is enabled.
