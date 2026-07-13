# Build-time remote script vendoring — v1.0.1

## Windows downloader behavior

Venom resolves `curl.exe` directly on Windows and passes the already validated HTTPS URL as a positional argument. Redirect following is enabled with `--max-redirs 0`, causing every redirect to fail closed. Venom intentionally does not use curl's `--proto =https` restriction because that can reject an HTTP CONNECT proxy even when the requested dependency URL is HTTPS. Response bytes must be written to Venom's unique `.download` file; a successful downloader exit without that file is treated as a hard error and all `.download`/`.stderr` temporary files are removed.

Venom production builds do not leave static CDN `<script src>` elements for the browser to fetch at runtime. Each static remote script is resolved during compilation and stored as a protected JavaScript chunk inside `assets/app/app.<hash>.vbc`.

## Build flow

For each static script source, the compiler:

1. normalizes protocol-relative URLs to HTTPS and strips fragments;
2. rejects HTTP, credentials, IPv6 literals, localhost, private IPv4 ranges, and local/internal hostnames;
3. reads a valid URL-keyed cache entry or downloads with `curl` through a no-shell process API;
4. enforces redirect denial, HTTPS-only protocols, TLS 1.2+, time limits, and an 8 MiB default response ceiling;
5. rejects empty, NUL-containing, invalid UTF-8, or HTML responses;
6. verifies declared `sha256-*`, `sha384-*`, or `sha512-*` Subresource Integrity;
7. checks the bytes against source-local `venom.lock` before cache commit;
8. caches only accepted bytes with their SHA-256 content digest;
9. compiles the bytes through the protected QuickJS/WASM package path.

The cache is build-local and is never copied into `dist`. `venom.lock` belongs with the source and is also never copied into `dist`.

The first online build creates the lock. Later builds enforce its URL, digest, size, and integrity result. `--refresh-vendors` is the only operation that updates pins.

## Commands

Normal online build:

```bash
venom build site --out dist
```

Explicit cache location:

```bash
venom build site --out dist --vendor-cache .venom-cache/remote
```

Offline lock-enforced build:

```bash
venom build site --out dist --vendor-cache .venom-cache/remote --offline
```

Refresh all static remote dependencies:

```bash
venom build site --out dist --vendor-cache .venom-cache/remote --refresh-vendors
```

`VENOM_CURL` may point to a specific curl executable. Venom passes arguments directly and never invokes a command shell.

## Release enforcement

The package contains encrypted `remote-vendors.vrvd` metadata. `release-check` and `verify-runtime` require:

```text
remote_vendor_metadata: yes
remote_vendor_lock: yes
remote_vendor_lock_mode: enforced|generated|refreshed
remote_vendor_lock_sha256: <64 lowercase hex>
unvendored_remote_scripts: 0
runtime_remote_chunks: 0
vendored_remote_chunks == remote_vendor_count
```

The generated production runtime also rejects any network script URL that bypasses the build-time vendor path.

## Scope

This pass vendors static `<script src>` elements discoverable in source HTML. It does not rewrite URLs constructed dynamically by JavaScript, and it does not vendor fetch/XHR traffic, images, stylesheets, fonts, media, analytics beacons, or other application network requests.
