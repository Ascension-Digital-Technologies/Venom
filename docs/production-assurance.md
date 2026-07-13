# Production assurance

Venom separates four release concerns:

1. **Native correctness** — cross-platform compilation and tests.
2. **Security assurance** — CodeQL, sanitizers, fuzzing, hostile-input tests, and integrity checks.
3. **Browser closure** — a real QuickJS/WASM build and browser-complete runtime tests.
4. **Artifact assurance** — deterministic packaging, manifests, SBOM, provenance, signatures, and byte-for-byte reproducibility checks.

## Reproducibility

Set `SOURCE_DATE_EPOCH` to a fixed Unix timestamp and package the same compiled inputs twice:

```bash
export SOURCE_DATE_EPOCH=1704067200
python tools/package_release.py --build-dir build/release --out build/repro-a --archive zip --sign none
python tools/package_release.py --build-dir build/release --out build/repro-b --archive zip --sign none
python tools/check_reproducibility.py <first.zip> <second.zip>
```

The comparison is strict: missing, extra, size-different, or hash-different files fail the gate.

Per-build polymorphism in protected website output is a security feature and is distinct from deterministic release packaging.

## Required tagged-release gates

- `.github/workflows/build.yml`
- `.github/workflows/security.yml`
- `.github/workflows/release-hardening.yml`
- `.github/workflows/reproducibility.yml`
- `.github/workflows/release.yml`
