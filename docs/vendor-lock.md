# Remote dependency lock — v1.0.1

Venom production builds pin every static remote JavaScript dependency in a source-local `venom.lock`. The cache stores bytes for fast or offline reuse; the lock decides which bytes are allowed.

## Default workflow

The first reviewed online build creates `venom.lock` beside the site's `index.html`:

```bash
venom build site --out dist
```

Subsequent builds enforce the exact normalized URL, SHA-256 digest, byte count, and SRI verification result recorded in that file. A network response that differs from the lock is rejected before it can replace the pinned cache body.

Commit `venom.lock` with the website source. It is build input and is never copied into `dist`.

## Offline builds

Remote dependencies require both the lock and valid cached bytes:

```bash
venom build site --out dist \
  --vendor-cache .venom-cache/remote \
  --offline
```

A missing lock, missing body, corrupt metadata, changed digest, changed dependency set, or changed integrity result fails closed.

## Reviewed updates

Only an explicit refresh may update pins:

```bash
venom build site --out dist \
  --vendor-cache .venom-cache/remote \
  --refresh-vendors
```

Review the dependency changes and the resulting `venom.lock` diff before committing it.

A custom lock location can be selected with:

```bash
venom build site --out dist --vendor-lock path/to/venom.lock
```

## Format

```text
VENOM_VENDOR_LOCK_V1
version=1
entry_count=1
url\tsha256\tbytes\tintegrity
https://cdn.example.com/app.js\t<64 lowercase hex>\t12345\tverified-sha384
```

Entries are normalized, unique, and sorted by URL. Supported integrity states are `not-declared`, `verified-sha256`, `verified-sha384`, and `verified-sha512`.

The protected package contains the canonical lock SHA-256 and lock mode in encrypted `remote-vendors.vrvd` metadata. `release-check` and `verify-runtime` require that contract.
