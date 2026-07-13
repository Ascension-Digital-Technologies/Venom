# Release packaging — v0.93.0

v0.93.0 keeps the deterministic binary release folder generator from v0.67.0 and adds a final integrity layer: manifest verification, optional manifest signatures, and archive verification.

## Commands

Linux/macOS:

```bash
scripts/package-release.sh --build-dir build --out dist-release
scripts/package-release.sh --build-dir build --out dist-release --archive zip
```

PowerShell:

```powershell
scripts\package-release.ps1 --build-dir build --out dist-release
scripts\package-release.ps1 --build-dir build --out dist-release --archive zip
```

Batch:

```bat
scripts\package-release.bat --build-dir build --out dist-release
package-release.bat --build-dir build --out dist-release
```

If the executable is not in the default build directory, pass it explicitly:

```bash
scripts/package-release.sh --venom /path/to/venom --out dist-release
```

## Signed release examples

Development-only smoke signature:

```bash
scripts/package-release.sh \
  --venom build/venom \
  --out dist-release \
  --archive zip \
  --sign dev-sha256 \
  --dev-insecure-key local-smoke-key
```

Production-style OpenSSL signature:

```bash
scripts/package-release.sh \
  --venom build/venom \
  --out dist-release \
  --archive zip \
  --sign openssl \
  --private-key /secure/offline/venom-release-private.pem \
  --public-key /secure/offline/venom-release-public.pem
```

The packager signs `RELEASE_MANIFEST.txt` when `--sign` is provided, then verifies the release directory and the generated archive before exiting successfully.

## Verify an existing release

```bash
scripts/verify-release.sh dist-release --expect-version 0.88.0
scripts/verify-release.sh venom-v0.93.0-linux-x86_64.zip --expect-version 0.88.0
```

Strict signature verification requires a verifier:

```bash
scripts/verify-release.sh dist-release \
  --expect-version 0.88.0 \
  --strict-signature \
  --public-key /path/to/venom-release-public.pem
```

See `docs/release-signing.md` for the exact signature modes and security boundary.

## Output layout

```txt
dist-release/
  venom[.exe]
  README.md
  VERSION.txt
  RELEASE_MANIFEST.txt
  RELEASE_MANIFEST.sig       # when --sign is used
  docs/
  examples/
  scripts/
  tools/
    verify_release.py
  licenses/
```

`VERSION.txt` records the release version and the verified `venom --version` output from the copied binary.

`RELEASE_MANIFEST.txt` records SHA-256 hashes and byte sizes for every shipped file except the manifest itself and the optional signature file.

## Release boundaries

This packaging pass does not change the QuickJS/WASM truth model:

```txt
verify-runtime --require-real-engine
```

still fails on the checked-in contract scaffold until `scripts/build-quickjs-wasm.*` is run with Emscripten and the verified upstream QuickJS WASM artifact is embedded.

The binary release is suitable for building and inspecting protected website packages, but a release should not claim full upstream browser QuickJS/WASM parity until the strict real-engine gate passes.
